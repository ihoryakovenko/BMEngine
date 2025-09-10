#include "Render.h"

#include "Engine/Systems/Render/VulkanHelper.h"
#include "RenderResources.h"
#include "TransferSystem.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "Util/Util.h"
#include "Util/Settings.h"
#include "Util/Math.h"

#include <random>
#include <mutex>

namespace Render
{
	static void InitImGuiPipeline(VkDescriptorPool* ImGuiPool, VulkanCoreContext::VulkanCoreContext* CoreContext, GLFWwindow* Wnd)
	{
		ImGui_ImplGlfw_InitForVulkan(Wnd, true);

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = DeferredPass::GetAttachmentData()->ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = DeferredPass::GetAttachmentData()->ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = DeferredPass::GetAttachmentData()->DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = DeferredPass::GetAttachmentData()->DepthAttachmentFormat;

		VkDescriptorPoolSize PoolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};
		VkDescriptorPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		PoolInfo.maxSets = 1;
		PoolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(PoolSizes);
		PoolInfo.pPoolSizes = PoolSizes;
		vkCreateDescriptorPool(CoreContext->LogicalDevice, &PoolInfo, nullptr, ImGuiPool);

		ImGui_ImplVulkan_InitInfo InitInfo = { };
		InitInfo.Instance = CoreContext->VulkanInstance;
		InitInfo.PhysicalDevice = CoreContext->PhysicalDevice;
		InitInfo.Device = CoreContext->LogicalDevice;
		InitInfo.QueueFamily = CoreContext->Indices.GraphicsFamily;
		InitInfo.Queue = CoreContext->GraphicsQueue;
		InitInfo.PipelineCache = nullptr;
		InitInfo.DescriptorPool = *ImGuiPool;
		InitInfo.RenderPass = nullptr;
		InitInfo.UseDynamicRendering = true;
		InitInfo.MinImageCount = 2;
		InitInfo.ImageCount = 3;
		InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		InitInfo.PipelineRenderingCreateInfo = RenderingInfo;
		ImGui_ImplVulkan_Init(&InitInfo);

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	static void DeInitImGuiPipeline(VkDevice Device, VkDescriptorPool ImGuiPool)
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	}

	static void InitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline)
	{
		VkSampler ShadowMapArraySampler = RenderResources::GetSampler("ShadowMap");

		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		MeshPipeline->StaticMeshLightLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		MeshPipeline->EntityLightBufferHandle = FrameManager::ReserveUniformMemory(LightBufferSize);
		MeshPipeline->StaticMeshLightSet = FrameManager::CreateAndBindSet(MeshPipeline->EntityLightBufferHandle, LightBufferSize, MeshPipeline->StaticMeshLightLayout);

		MeshPipeline->ShadowMapArrayLayout = RenderResources::GetSetLayout("ShadowMapArrayLayout");

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VkImageViewCreateInfo ViewCreateInfo = {};
			ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo.image = LightningPass::GetShadowMapArray()[i].Image;
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			ViewCreateInfo.format = DepthFormat;
			ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo.subresourceRange.levelCount = 1;
			ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo.subresourceRange.layerCount = 2;
			ViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &MeshPipeline->ShadowMapArrayImageInterface[i]));

			VkDescriptorImageInfo ShadowMapArrayImageInfo;
			ShadowMapArrayImageInfo.imageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapArrayImageInfo.sampler = ShadowMapArraySampler;
			ShadowMapArrayImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorSetAllocateInfo AllocInfo = {};
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = RenderResources::GetMainPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &MeshPipeline->ShadowMapArrayLayout;
			
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, MeshPipeline->ShadowMapArraySet + i));

			VkWriteDescriptorSet WriteDescriptorSet = {};
			WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet.dstSet = MeshPipeline->ShadowMapArraySet[i];
			WriteDescriptorSet.dstBinding = 0;
			WriteDescriptorSet.dstArrayElement = 0;
			WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSet.descriptorCount = 1;
			WriteDescriptorSet.pBufferInfo = nullptr;
			WriteDescriptorSet.pImageInfo = &ShadowMapArrayImageInfo;

			vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
		}

		VkDescriptorSetLayout StaticMeshDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			RenderResources::GetBindlesTexturesLayout(),
			MeshPipeline->StaticMeshLightLayout,
			RenderResources::GetMaterialLayout(),
			MeshPipeline->ShadowMapArrayLayout
		};

		const u32 StaticMeshDescriptorLayoutCount = sizeof(StaticMeshDescriptorLayouts) / sizeof(StaticMeshDescriptorLayouts[0]);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = StaticMeshDescriptorLayoutCount;
		PipelineLayoutCreateInfo.pSetLayouts = StaticMeshDescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.pPushConstantRanges = &MeshPipeline->PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &MeshPipeline->Pipeline.PipelineLayout));

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/StaticMesh.yaml");

		VulkanHelper::PipelineResourceInfo ResourceInfo = {};
		ResourceInfo.PipelineLayout = MeshPipeline->Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		MeshPipeline->Pipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, MainScreenExtent, MeshPipeline->Pipeline.PipelineLayout, &ResourceInfo);
	}

	static void DeInitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline)
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, MeshPipeline->ShadowMapArrayImageInterface[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, MeshPipeline->StaticMeshLightLayout, nullptr);

		vkDestroyPipeline(Device, MeshPipeline->Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, MeshPipeline->Pipeline.PipelineLayout, nullptr);
	}

	static void DrawStaticMeshes(VkDevice Device, VkCommandBuffer CmdBuffer, StaticMeshPipeline* MeshPipeline, DrawScene* Scene)
	{
		FrameManager::UpdateUniformMemory(MeshPipeline->EntityLightBufferHandle, Scene->LightEntity, sizeof(LightBuffer));

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.Pipeline);

		const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet();
		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			0, 1, &VpSet, 1, &DynamicOffset);

		VkDescriptorSet BindlesTexturesSet = RenderResources::GetBindlesTexturesSet();
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			1, 1, &BindlesTexturesSet, 0, nullptr);

		const u32 LightDynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(LightBuffer);

		std::unique_lock Lock(Scene->TempLock);
		for (u32 i = 0; i < Scene->DrawEntities.Count; ++i)
		{
			DrawEntity* DrawEntity = Scene->DrawEntities.Data + i;
			if (!RenderResources::IsDrawEntityLoaded(DrawEntity))
			{
				continue;
			}

			RenderResources::VertexData* Mesh = RenderResources::GetStaticMesh(DrawEntity->StaticMeshIndex);
			RenderResources::InstanceData* Instance = RenderResources::GetInstanceData(DrawEntity->InstanceDataIndex);

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				MeshPipeline->StaticMeshLightSet,
				RenderResources::GetMaterialSet(),
				MeshPipeline->ShadowMapArraySet[VulkanInterface::TestGetImageIndex()],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			const VkBuffer Buffers[] =
			{
				RenderResources::GetVertexStageBuffer(),
				RenderResources::GetInstanceBuffer()
			};

			const u64 Offsets[] = 
			{
				Mesh->VertexOffset,
				sizeof(RenderResources::InstanceData)* DrawEntity->InstanceDataIndex
			};

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
				2, DescriptorSetGroupCount, DescriptorSetGroup, 1, &LightDynamicOffset);

			vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
			vkCmdBindIndexBuffer(CmdBuffer, RenderResources::GetVertexStageBuffer(), Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, DrawEntity->Instances, 0, 0, 0);
		}
	}

	static void InitDrawState(VkDevice Device, u32 GraphicsFamily, u32 MaxDrawFrames, DrawState* State)
	{
		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = GraphicsFamily;

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, nullptr, &State->GraphicsCommandPool));
		
		VkCommandBufferAllocateInfo GraphicsCommandBufferAllocateInfo = { };
		GraphicsCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		GraphicsCommandBufferAllocateInfo.commandPool = State->GraphicsCommandPool;
		GraphicsCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		GraphicsCommandBufferAllocateInfo.commandBufferCount = MaxDrawFrames;

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &GraphicsCommandBufferAllocateInfo, State->Frames.CommandBuffers));

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (u64 i = 0; i < MaxDrawFrames; i++)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, &State->Frames.Fences[i]));
			VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &State->Frames.ImagesAvailable[i]));
			VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &State->Frames.RenderFinished[i]));
		}
	}

	static void DeInitDrawState(VkDevice Device, u32 MaxDrawFrames, DrawState* State)
	{
		vkDestroyCommandPool(Device, State->GraphicsCommandPool, nullptr);

		for (u64 i = 0; i < MaxDrawFrames; i++)
		{
			vkDestroyFence(Device, State->Frames.Fences[i], nullptr);
			vkDestroySemaphore(Device, State->Frames.ImagesAvailable[i], nullptr);
			vkDestroySemaphore(Device, State->Frames.RenderFinished[i], nullptr);
		}
	}


	

	static RenderState State;

	void TmpInitFrameMemory()
	{
		State.FrameMemory = Memory::CreateFrameMemory(1024 * 1024);
	}

	void Init(GLFWwindow* WindowHandler)
	{		
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		InitDrawState(Device, VulkanInterface::GetQueueGraphicsFamilyIndex(), VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);

		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(Device, &State.MeshPipeline);
		InitImGuiPipeline(&State.DebugUiPool, RenderResources::GetCoreContext(), WindowHandler);
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		DeInitImGuiPipeline(Device, State.DebugUiPool);
		DeInitDrawState(Device, VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);

		//TerrainRender::DeInit();
		DeInitStaticMeshPipeline(Device, &State.MeshPipeline);

		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();

		Memory::DestroyFrameMemory(&State.FrameMemory);
	}

	void* FrameAlloc(u32 Size)
	{
		return Memory::FrameAlloc(&State.FrameMemory, Size);
	}

	void Draw(DrawScene* Scene)
	{
		VulkanCoreContext::VulkanCoreContext* CoreContext = RenderResources::GetCoreContext();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkDevice Device = VulkanInterface::GetDevice();
		const u32 CurrentFrame = State.RenderDrawState.CurrentFrame;
		u32 ImageIndex;
		VkFence FrameFence = State.RenderDrawState.Frames.Fences[CurrentFrame];
		VkSemaphore ImagesAvailable = State.RenderDrawState.Frames.ImagesAvailable[CurrentFrame];

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &FrameFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &FrameFence));
		VULKAN_CHECK_RESULT(vkAcquireNextImageKHR(Device, VulkanInterface::GetSwapchain(), UINT64_MAX, ImagesAvailable, nullptr, &ImageIndex));
		State.RenderDrawState.CurrentImageIndex = ImageIndex;

		FrameManager::UpdateViewProjection(&Scene->ViewProjection);

		VkCommandBuffer DrawCmdBuffer = State.RenderDrawState.Frames.CommandBuffers[ImageIndex];
		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(DrawCmdBuffer, &CommandBufferBeginInfo));

		LightningPass::Draw(Scene);
		MainPass::BeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(Device, DrawCmdBuffer, &State.MeshPipeline, Scene);
		MainPass::EndPass();
		DeferredPass::BeginPass();
		DeferredPass::Draw();
		ImGui::Render();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CmdBuffer);
		DeferredPass::EndPass();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(DrawCmdBuffer));

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};

		VkSemaphore RenderFinished = State.RenderDrawState.Frames.RenderFinished[CurrentFrame];
		VkSwapchainKHR Swapchain = VulkanInterface::GetSwapchain();

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImagesAvailable;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &DrawCmdBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderFinished;

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished;
		PresentInfo.pSwapchains = &Swapchain;
		PresentInfo.pImageIndices = &ImageIndex;

		std::unique_lock Lock(CoreContext->QueueSubmitMutex);
		VULKAN_CHECK_RESULT(vkQueueSubmit(VulkanInterface::GetGraphicsQueue(), 1, &SubmitInfo, FrameFence));
		VULKAN_CHECK_RESULT(vkQueuePresentKHR(VulkanInterface::GetGraphicsQueue(), &PresentInfo));
		Lock.unlock();

		State.RenderDrawState.CurrentFrame = Math::WrapIncrement(CurrentFrame, VulkanHelper::MAX_DRAW_FRAMES);

		Memory::FrameFree(&State.FrameMemory);
	}

	RenderState* GetRenderState()
	{
		return &State;
	}
}








namespace DeferredPass
{
	static VulkanHelper::RenderPipeline Pipeline;

	static VkDescriptorSetLayout DeferredInputLayout;

	static VulkanInterface::UniformImage DeferredInputDepthImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VulkanInterface::UniformImage DeferredInputColorImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView DeferredInputDepthImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView DeferredInputColorImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet DeferredInputSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkSampler ColorSampler;
	static VkSampler DepthSampler;

	static VulkanHelper::AttachmentData AttachmentData;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();

		AttachmentData.ColorAttachmentCount = 1;
		AttachmentData.ColorAttachmentFormats[0] = VulkanInterface::GetSurfaceFormat();
		AttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		AttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		DeferredInputLayout = RenderResources::GetSetLayout("MainPassOutputLayout");

		VkImageCreateInfo DeferredInputDepthUniformCreateInfo = { };
		DeferredInputDepthUniformCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		DeferredInputDepthUniformCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		DeferredInputDepthUniformCreateInfo.extent.depth = 1;
		DeferredInputDepthUniformCreateInfo.mipLevels = 1;
		DeferredInputDepthUniformCreateInfo.arrayLayers = 1;
		DeferredInputDepthUniformCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		DeferredInputDepthUniformCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DeferredInputDepthUniformCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		DeferredInputDepthUniformCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		DeferredInputDepthUniformCreateInfo.flags = 0;
		DeferredInputDepthUniformCreateInfo.extent.width = MainScreenExtent.width;
		DeferredInputDepthUniformCreateInfo.extent.height = MainScreenExtent.height;
		DeferredInputDepthUniformCreateInfo.format = DepthFormat;
		DeferredInputDepthUniformCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo DeferredInputColorUniformCreateInfo = DeferredInputDepthUniformCreateInfo;
		DeferredInputColorUniformCreateInfo.format = ColorFormat;
		DeferredInputColorUniformCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		ColorSampler = RenderResources::GetSampler("ColorAttachment");
		DepthSampler = RenderResources::GetSampler("DepthAttachment");

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			vkCreateImage(Device, &DeferredInputDepthUniformCreateInfo, nullptr, &DeferredInputDepthImage[i].Image);
			VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
				DeferredInputDepthImage[i].Image, VulkanHelper::MemoryPropertyFlag::GPULocal);
			DeferredInputDepthImage[i].Memory = AllocResult.Memory;
			vkBindImageMemory(Device, DeferredInputDepthImage[i].Image, DeferredInputDepthImage[i].Memory, 0);

			vkCreateImage(Device, &DeferredInputColorUniformCreateInfo, nullptr, &DeferredInputColorImage[i].Image);
			AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, DeferredInputColorImage[i].Image, VulkanHelper::MemoryPropertyFlag::GPULocal);
			DeferredInputColorImage[i].Memory = AllocResult.Memory;
			vkBindImageMemory(Device, DeferredInputColorImage[i].Image, DeferredInputColorImage[i].Memory, 0);

			VkImageViewCreateInfo DepthViewCreateInfo = { };
			DepthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			DepthViewCreateInfo.image = DeferredInputDepthImage[i].Image;
			DepthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			DepthViewCreateInfo.format = DepthFormat;
			DepthViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			DepthViewCreateInfo.subresourceRange.baseMipLevel = 0;
			DepthViewCreateInfo.subresourceRange.levelCount = 1;
			DepthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			DepthViewCreateInfo.subresourceRange.layerCount = 1;
			DepthViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &DepthViewCreateInfo, nullptr, &DeferredInputDepthImageInterface[i]));

			VkImageViewCreateInfo ColorViewCreateInfo = { };
			ColorViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ColorViewCreateInfo.image = DeferredInputColorImage[i].Image;
			ColorViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ColorViewCreateInfo.format = ColorFormat;
			ColorViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ColorViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ColorViewCreateInfo.subresourceRange.levelCount = 1;
			ColorViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ColorViewCreateInfo.subresourceRange.layerCount = 1;
			ColorViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ColorViewCreateInfo, nullptr, &DeferredInputColorImageInterface[i]));

			VkDescriptorSetAllocateInfo AllocInfo = { };
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = RenderResources::GetMainPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &DeferredInputLayout;
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, DeferredInputSet + i));

			VkDescriptorImageInfo DeferredInputImageInfo[2];
			DeferredInputImageInfo[0].imageView = DeferredInputColorImageInterface[i];
			DeferredInputImageInfo[0].sampler = ColorSampler;
			DeferredInputImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			DeferredInputImageInfo[1].imageView = DeferredInputDepthImageInterface[i];
			DeferredInputImageInfo[1].sampler = DepthSampler;
			DeferredInputImageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet WriteDescriptorSets[2] = { };

			WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[0].dstSet = DeferredInputSet[i];
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DeferredInputImageInfo[0];

			WriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[1].dstSet = DeferredInputSet[i];
			WriteDescriptorSets[1].dstBinding = 1;
			WriteDescriptorSets[1].dstArrayElement = 0;
			WriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSets[1].descriptorCount = 1;
			WriteDescriptorSets[1].pBufferInfo = nullptr;
			WriteDescriptorSets[1].pImageInfo = &DeferredInputImageInfo[1];

			vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);
		}


		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.pSetLayouts = &DeferredInputLayout;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = AttachmentData;

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/DeferredPipeline.yaml");

		Pipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, MainScreenExtent, Pipeline.PipelineLayout, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, DeferredInputDepthImageInterface[i], nullptr);
			vkDestroyImageView(Device, DeferredInputColorImageInterface[i], nullptr);

			vkDestroyImage(Device, DeferredInputDepthImage[i].Image, nullptr);
			vkFreeMemory(Device, DeferredInputDepthImage[i].Memory, nullptr);

			vkDestroyImage(Device, DeferredInputColorImage[i].Image, nullptr);
			vkFreeMemory(Device, DeferredInputColorImage[i].Memory, nullptr);
		}


		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, 1, &DeferredInputSet[VulkanInterface::TestGetImageIndex()], 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void BeginPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingAttachmentInfo SwapchainColorAttachment = { };
		SwapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		SwapchainColorAttachment.imageView = VulkanInterface::GetSwapchainImageViews()[VulkanInterface::TestGetImageIndex()];
		SwapchainColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SwapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		SwapchainColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingInfo RenderingInfo{ };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.renderArea = RenderArea;
		RenderingInfo.layerCount = 1;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &SwapchainColorAttachment;
		RenderingInfo.pDepthAttachment = nullptr;

		VkImageMemoryBarrier2 ColorBarrier = { };
		ColorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		ColorBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ColorBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.image = DeferredInputColorImage[VulkanInterface::TestGetImageIndex()].Image;
		ColorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ColorBarrier.subresourceRange.baseMipLevel = 0;
		ColorBarrier.subresourceRange.levelCount = 1;
		ColorBarrier.subresourceRange.baseArrayLayer = 0;
		ColorBarrier.subresourceRange.layerCount = 1;
		// RELEASE: wait for all color-attachmet writes to finish
		ColorBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		ColorBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: make image ready for sampling in the fragment shader
		ColorBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		ColorBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkImageMemoryBarrier2 DepthBarrier = { };
		DepthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.image = DeferredInputDepthImage[VulkanInterface::TestGetImageIndex()].Image;
		DepthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthBarrier.subresourceRange.baseMipLevel = 0;
		DepthBarrier.subresourceRange.levelCount = 1;
		DepthBarrier.subresourceRange.baseArrayLayer = 0;
		DepthBarrier.subresourceRange.layerCount = 1;
		DepthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		DepthBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;


		VkImageMemoryBarrier2 SwapchainAcquireBarrier = { };
		SwapchainAcquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainAcquireBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SwapchainAcquireBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainAcquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainAcquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainAcquireBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainAcquireBarrier.subresourceRange.levelCount = 1;
		SwapchainAcquireBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainAcquireBarrier.subresourceRange.layerCount = 1;
		// RELEASE nothing we don't depend on any earlier writes to the swap image
		SwapchainAcquireBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainAcquireBarrier.srcAccessMask = 0;
		// ACQUIRE for our upcoming color-attachment writes
		SwapchainAcquireBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainAcquireBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkImageMemoryBarrier2 Barriers[] = {
			ColorBarrier,
			DepthBarrier,
			SwapchainAcquireBarrier
		};

		VkDependencyInfo DepInfo = { };
		DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfo.imageMemoryBarrierCount = 3;
		DepInfo.pImageMemoryBarriers = Barriers;

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfo);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
	}

	void EndPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdEndRendering(CmdBuffer);

		VkImageMemoryBarrier2 SwapchainPresentBarrier = { };
		SwapchainPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainPresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainPresentBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainPresentBarrier.subresourceRange.levelCount = 1;
		SwapchainPresentBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainPresentBarrier.subresourceRange.layerCount = 1;
		SwapchainPresentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			| VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		SwapchainPresentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			| VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		SwapchainPresentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;  // no further memory dep
		SwapchainPresentBarrier.dstAccessMask = 0;

		VkDependencyInfo PresentDepInfo = { };
		PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		PresentDepInfo.imageMemoryBarrierCount = 1;
		PresentDepInfo.pImageMemoryBarriers = &SwapchainPresentBarrier;

		vkCmdPipelineBarrier2(CmdBuffer, &PresentDepInfo);
	}





	VkImageView* TestDeferredInputColorImageInterface()
	{
		return DeferredInputColorImageInterface;
	}

	VkImageView* TestDeferredInputDepthImageInterface()
	{
		return DeferredInputDepthImageInterface;
	}

	VulkanInterface::UniformImage* TestDeferredInputColorImage()
	{
		return DeferredInputColorImage;
	}

	VulkanInterface::UniformImage* TestDeferredInputDepthImage()
	{
		return DeferredInputDepthImage;
	}

	VulkanHelper::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}

namespace LightningPass
{
	static VulkanInterface::UniformImage ShadowMapArray[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSetLayout LightSpaceMatrixLayout;

	static VulkanInterface::UniformBuffer LightSpaceMatrixBuffer[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet LightSpaceMatrixSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView ShadowMapElement1ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView ShadowMapElement2ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	static VulkanHelper::RenderPipeline Pipeline;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();

		LightSpaceMatrixLayout = RenderResources::GetSetLayout("LightSpaceMatrixLayout");

		VkImageCreateInfo ShadowMapArrayCreateInfo = { };
		ShadowMapArrayCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ShadowMapArrayCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ShadowMapArrayCreateInfo.extent.depth = 1;
		ShadowMapArrayCreateInfo.mipLevels = 1;
		ShadowMapArrayCreateInfo.arrayLayers = 1;
		ShadowMapArrayCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ShadowMapArrayCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ShadowMapArrayCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ShadowMapArrayCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ShadowMapArrayCreateInfo.flags = 0;
		ShadowMapArrayCreateInfo.format = ColorFormat;
		ShadowMapArrayCreateInfo.extent.width = DepthViewportExtent.width;
		ShadowMapArrayCreateInfo.extent.height = DepthViewportExtent.height;
		ShadowMapArrayCreateInfo.format = DepthFormat;
		ShadowMapArrayCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ShadowMapArrayCreateInfo.arrayLayers = 2;

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkCreateImage(Device, &ShadowMapArrayCreateInfo, nullptr, &ShadowMapArray[i].Image);
			VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, ShadowMapArray[i].Image, VulkanHelper::MemoryPropertyFlag::GPULocal);
			ShadowMapArray[i].Memory = AllocResult.Memory;
			VULKAN_CHECK_RESULT(vkBindImageMemory(Device, ShadowMapArray[i].Image, ShadowMapArray[i].Memory, 0));

			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			VkBufferCreateInfo BufferInfo = { };
			BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			BufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			BufferInfo.size = LightSpaceMatrixSize;

			LightSpaceMatrixBuffer[i].Buffer = VulkanHelper::CreateBuffer(Device, LightSpaceMatrixSize, VulkanHelper::BufferUsageFlag::UniformFlag);
			AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, LightSpaceMatrixBuffer[i].Buffer,
				VulkanHelper::MemoryPropertyFlag::HostCompatible);
			LightSpaceMatrixBuffer[i].Memory = AllocResult.Memory;
			vkBindBufferMemory(Device, LightSpaceMatrixBuffer[i].Buffer, LightSpaceMatrixBuffer[i].Memory, 0);

			VkDescriptorSetAllocateInfo AllocInfo = { };
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = RenderResources::GetMainPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &LightSpaceMatrixLayout;
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, LightSpaceMatrixSet + i));

			VkImageViewCreateInfo ViewCreateInfo1 = { };
			ViewCreateInfo1.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo1.image = ShadowMapArray[i].Image;
			ViewCreateInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo1.format = DepthFormat;
			ViewCreateInfo1.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo1.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo1.subresourceRange.levelCount = 1;
			ViewCreateInfo1.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo1.subresourceRange.layerCount = 1;
			ViewCreateInfo1.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo1, nullptr, &ShadowMapElement1ImageInterface[i]));

			VkImageViewCreateInfo ViewCreateInfo2 = { };
			ViewCreateInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo2.image = ShadowMapArray[i].Image;
			ViewCreateInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo2.format = DepthFormat;
			ViewCreateInfo2.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo2.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo2.subresourceRange.levelCount = 1;
			ViewCreateInfo2.subresourceRange.baseArrayLayer = 1;
			ViewCreateInfo2.subresourceRange.layerCount = 1;
			ViewCreateInfo2.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo2, nullptr, &ShadowMapElement2ImageInterface[i]));

			VkDescriptorBufferInfo LightSpaceMatrixBufferInfo;
			LightSpaceMatrixBufferInfo.buffer = LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixBufferInfo.offset = 0;
			LightSpaceMatrixBufferInfo.range = LightSpaceMatrixSize;

			VkWriteDescriptorSet WriteDescriptorSet = { };
			WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet.dstSet = LightSpaceMatrixSet[i];
			WriteDescriptorSet.dstBinding = 0;
			WriteDescriptorSet.dstArrayElement = 0;
			WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			WriteDescriptorSet.descriptorCount = 1;
			WriteDescriptorSet.pBufferInfo = &LightSpaceMatrixBufferInfo;
			WriteDescriptorSet.pImageInfo = nullptr;

			vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
		}

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(glm::mat4);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.pSetLayouts = &LightSpaceMatrixLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 0;
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/DepthPipeline.yaml");

		Pipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, DepthViewportExtent, Pipeline.PipelineLayout, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyBuffer(Device, LightSpaceMatrixBuffer[i].Buffer, nullptr);
			vkFreeMemory(Device, LightSpaceMatrixBuffer[i].Memory, nullptr);

			vkDestroyImageView(Device, ShadowMapElement1ImageInterface[i], nullptr);
			vkDestroyImageView(Device, ShadowMapElement2ImageInterface[i], nullptr);

			vkDestroyImage(Device, ShadowMapArray[i].Image, nullptr);
			vkFreeMemory(Device, ShadowMapArray[i].Memory, nullptr);
		}


		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw(Render::DrawScene* Scene)
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		VkImageView Attachments[2];
		Attachments[0] = ShadowMapElement1ImageInterface[VulkanInterface::TestGetImageIndex()];
		Attachments[1] = ShadowMapElement2ImageInterface[VulkanInterface::TestGetImageIndex()];

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, LightSpaceMatrixBuffer[LightCaster].Memory, sizeof(glm::mat4), 0,
				LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };

			VkRenderingAttachmentInfo DepthAttachment{ };
			DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			DepthAttachment.imageView = Attachments[LightCaster];
			DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

			VkRenderingInfo RenderingInfo{ };
			RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			RenderingInfo.renderArea = RenderArea;
			RenderingInfo.layerCount = 1;
			RenderingInfo.colorAttachmentCount = 0;
			RenderingInfo.pColorAttachments = nullptr;
			RenderingInfo.pDepthAttachment = &DepthAttachment;

			VkImageMemoryBarrier2 DepthAttachmentTransitionBefore = { };
			DepthAttachmentTransitionBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			DepthAttachmentTransitionBefore.srcStageMask = VK_PIPELINE_STAGE_2_NONE; // because we're coming from UNDEFINED
			DepthAttachmentTransitionBefore.srcAccessMask = 0;
			DepthAttachmentTransitionBefore.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			DepthAttachmentTransitionBefore.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			DepthAttachmentTransitionBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			DepthAttachmentTransitionBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			DepthAttachmentTransitionBefore.image = ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
			DepthAttachmentTransitionBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			DepthAttachmentTransitionBefore.subresourceRange.baseMipLevel = 0;
			DepthAttachmentTransitionBefore.subresourceRange.levelCount = 1;
			DepthAttachmentTransitionBefore.subresourceRange.baseArrayLayer = LightCaster;
			DepthAttachmentTransitionBefore.subresourceRange.layerCount = 1;

			VkDependencyInfo DependencyInfoBefore = { };
			DependencyInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			DependencyInfoBefore.imageMemoryBarrierCount = 1;
			DependencyInfoBefore.pImageMemoryBarriers = &DepthAttachmentTransitionBefore,

			vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoBefore);

			vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

			vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

			std::unique_lock Lock(Scene->TempLock);
			for (u32 i = 0; i < Scene->DrawEntities.Count; ++i)
			{
				Render::DrawEntity* DrawEntity = Scene->DrawEntities.Data + i;
				if (!RenderResources::IsDrawEntityLoaded(DrawEntity))
				{
					continue;
				}

				RenderResources::VertexData* Mesh = RenderResources::GetStaticMesh(DrawEntity->StaticMeshIndex);

				const VkBuffer Buffers[] =
				{
					RenderResources::GetVertexStageBuffer(),
					RenderResources::GetInstanceBuffer()
				};

				const u64 Offsets[] =
				{
					Mesh->VertexOffset,
					sizeof(RenderResources::InstanceData) * DrawEntity->InstanceDataIndex
				};

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					LightSpaceMatrixSet[LightCaster],
				};

				const VkPipelineLayout PipelineLayout = Pipeline.PipelineLayout;

				vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr);

				vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
				vkCmdBindIndexBuffer(CmdBuffer, RenderResources::GetVertexStageBuffer(), Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, DrawEntity->Instances, 0, 0, 0);
			}

			vkCmdEndRendering(CmdBuffer);
		}

		// TODO: move to Main pass?
		VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
		DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

		DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthAttachmentTransitionAfter.image = ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
		DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = 0;
		DepthAttachmentTransitionAfter.subresourceRange.layerCount = 2;
		// RELEASE: all depth writes have finished  
		DepthAttachmentTransitionAfter.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthAttachmentTransitionAfter.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: allow sampling in the fragment shader  
		DepthAttachmentTransitionAfter.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		DepthAttachmentTransitionAfter.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		VkDependencyInfo DependencyInfoAfter = { };
		DependencyInfoAfter.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DependencyInfoAfter.imageMemoryBarrierCount = 1;
		DependencyInfoAfter.pImageMemoryBarriers = &DepthAttachmentTransitionAfter;

		vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoAfter);
	}

	VulkanInterface::UniformImage* GetShadowMapArray()
	{
		return ShadowMapArray;
	}
}

namespace MainPass
{
	static VkDescriptorSetLayout SkyBoxLayout;

	static VkDescriptorSet SkyBoxSet;

	static VulkanHelper::RenderPipeline SkyBoxPipeline;

	static VulkanHelper::AttachmentData AttachmentData;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		AttachmentData.ColorAttachmentCount = 1;
		AttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		AttachmentData.DepthAttachmentFormat = DepthFormat;


		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding LayoutBinding = { };
		LayoutBinding.binding = 0;
		LayoutBinding.descriptorType = Type;
		LayoutBinding.descriptorCount = 1;
		LayoutBinding.stageFlags = Flags;
		LayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = 1;
		LayoutCreateInfo.pBindings = &LayoutBinding;
		LayoutCreateInfo.flags = 0;
		LayoutCreateInfo.pNext = nullptr;

		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanInterface::GetDevice(), &LayoutCreateInfo, nullptr, &SkyBoxLayout));

		const VkDescriptorSetLayout VpLayout = FrameManager::GetViewProjectionLayout();

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			VpLayout,
			SkyBoxLayout,
		};

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = SkyBoxDescriptorLayoutCount;
		PipelineLayoutCreateInfo.pSetLayouts = SkyBoxDescriptorLayouts;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &SkyBoxPipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = SkyBoxPipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = AttachmentData;

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/SkyBoxPipeline.yaml");

		SkyBoxPipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, MainScreenExtent, SkyBoxPipeline.PipelineLayout, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyDescriptorSetLayout(VulkanInterface::GetDevice(), SkyBoxLayout, nullptr);
		vkDestroyPipeline(Device, SkyBoxPipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, SkyBoxPipeline.PipelineLayout, nullptr);
	}

	void BeginPass()
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];
		VkRenderingAttachmentInfo ColorAttachment = { };
		ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		ColorAttachment.imageView = DeferredPass::TestDeferredInputColorImageInterface()[VulkanInterface::TestGetImageIndex()];
		ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo DepthAttachment = { };
		DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachment.imageView = DeferredPass::TestDeferredInputDepthImageInterface()[VulkanInterface::TestGetImageIndex()];
		DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.clearValue.depthStencil.depth = 1.0f;

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.flags = 0;
		RenderingInfo.renderArea = RenderArea;
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask = 0;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &ColorAttachment;
		RenderingInfo.pDepthAttachment = &DepthAttachment;
		RenderingInfo.pStencilAttachment = nullptr;

		VkImageMemoryBarrier2 ColorBarrierBefore = { };
		ColorBarrierBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		ColorBarrierBefore.pNext = nullptr;
		ColorBarrierBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorBarrierBefore.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorBarrierBefore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrierBefore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrierBefore.image = DeferredPass::TestDeferredInputColorImage()[VulkanInterface::TestGetImageIndex()].Image;
		ColorBarrierBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ColorBarrierBefore.subresourceRange.baseMipLevel = 0;
		ColorBarrierBefore.subresourceRange.levelCount = 1;
		ColorBarrierBefore.subresourceRange.baseArrayLayer = 0;
		ColorBarrierBefore.subresourceRange.layerCount = 1;
		ColorBarrierBefore.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;        // whatever stage last read it
		ColorBarrierBefore.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;                   // reading it as a sampled image
		ColorBarrierBefore.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; // the clear/write stage
		ColorBarrierBefore.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;         // the clear/write access

		VkImageMemoryBarrier2 DepthBarrierBefore = { };
		DepthBarrierBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthBarrierBefore.pNext = nullptr;
		DepthBarrierBefore.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthBarrierBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthBarrierBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthBarrierBefore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrierBefore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrierBefore.image = DeferredPass::TestDeferredInputDepthImage()[VulkanInterface::TestGetImageIndex()].Image;
		DepthBarrierBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthBarrierBefore.subresourceRange.baseMipLevel = 0;
		DepthBarrierBefore.subresourceRange.levelCount = 1;
		DepthBarrierBefore.subresourceRange.baseArrayLayer = 0;
		DepthBarrierBefore.subresourceRange.layerCount = 1;
		// RELEASE nothing (it was UNDEFINED)
		DepthBarrierBefore.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DepthBarrierBefore.srcAccessMask = VK_PIPELINE_STAGE_NONE;
		// ACQUIRE for depth-writes
		DepthBarrierBefore.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
			| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		VkImageMemoryBarrier2 BarriersBefore[] = { ColorBarrierBefore, DepthBarrierBefore };

		VkDependencyInfo DepInfoBefore = { };
		DepInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfoBefore.imageMemoryBarrierCount = 2;
		DepInfoBefore.pImageMemoryBarriers = BarriersBefore;

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfoBefore);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

		//
		//if (Scene->DrawSkyBox)
		//{
		//	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines[2].Pipeline);

		//	const u32 SkyBoxDescriptorSetGroupCount = 2;
		//	const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
		//		VpSet,
		//		Scene->SkyBox.TextureSet,
		//	};

		//	const VkPipelineLayout PipelineLayout = Pipelines[2].PipelineLayout;

		//	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
		//		0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		//	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &PassSharedResources.VertexBuffer.Buffer, &Scene->SkyBox.VertexOffset);
		//	vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene->SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(CommandBuffer, Scene->SkyBox.IndicesCount, 1, 0, 0, 0);
		//}

		//MainRenderPass::OnDraw();
	}

	void EndPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdEndRendering(CmdBuffer);
	}

	VulkanHelper::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}

namespace TerrainRender
{
	struct TerrainVertex
	{
		f32 Altitude;
	};

	// TMP
	struct PushConstantsData
	{
		glm::mat4 Model;
		s32 matIndex;
	};

	static void LoadTerrain();
	static void GenerateTerrain(std::vector<u32>& Indices);

	static const u32 NumRows = 600;
	static const u32 NumCols = 600;
	static TerrainVertex TerrainVerticesData[NumRows][NumCols];
	static TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
	static u32 IndicesCount;

	static VkDescriptorSet TerrainSet;

	static VulkanHelper::RenderPipeline Pipeline;
	static Render::DrawEntity TerrainDrawObject;

	static VkPushConstantRange PushConstants;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u32 ShaderCount = 2;
		VulkanHelper::Shader Shaders[ShaderCount];

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_frag.spv", FragmentShaderCode, "rb");

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		const Render::RenderState* State = Render::GetRenderState();

		VkDescriptorSetLayout TerrainDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			RenderResources::GetBindlesTexturesLayout(),
			RenderResources::GetMaterialLayout(),
		};

		const u32 LayoutsCount = sizeof(TerrainDescriptorLayouts) / sizeof(TerrainDescriptorLayouts[0]);

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(PushConstantsData);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = LayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = TerrainDescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/TerrainPipeline.yaml");

		Pipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, MainScreenExtent, Pipeline.PipelineLayout, &ResourceInfo);

		LoadTerrain();
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw()
	{
		const Render::RenderState* State = Render::GetRenderState();

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		const VkDescriptorSet Sets[] = {
			FrameManager::GetViewProjectionSet(),
			RenderResources::GetBindlesTexturesSet(),
			RenderResources::GetMaterialSet(),
		};

		const u32 TerrainDescriptorSetGroupCount = sizeof(Sets) / sizeof(Sets[0]);

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, TerrainDescriptorSetGroupCount, Sets, 1, &DynamicOffset);

		PushConstantsData Constants;
		//Constants.Model = TerrainDrawObject.Model;
		//Constants.matIndex = TerrainDrawObject.MaterialIndex;

		const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		vkCmdPushConstants(CmdBuffer, Pipeline.PipelineLayout, Flags, 0, sizeof(PushConstantsData), &Constants);

		//VkBuffer VertexBuffer = RenderResources::GetVertexBuffer().Buffer;
		//VkBuffer IndexBuffer = RenderResources::GetIndexBuffer().Buffer;

		//u32 Count;
		//const u64 VertexOffset = TerrainDrawObject.VertexOffset;
		//const u64 IndexOffset = TerrainDrawObject.IndexOffset;

		//vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &VertexBuffer, &VertexOffset);
		//vkCmdBindIndexBuffer(CmdBuffer, IndexBuffer, IndexOffset, VK_INDEX_TYPE_UINT32);
		//vkCmdDrawIndexed(CmdBuffer, IndicesCount, 1, 0, 0, 0);
	}

	void LoadTerrain()
	{
		std::vector<u32> TerrainIndices;
		GenerateTerrain(TerrainIndices);

		IndicesCount = TerrainIndices.size();

		RenderResources::Material Mat = { };
		//u32 MaterialIndex = Render::CreateMaterial(&Mat);
		//TerrainDrawObject = RenderResources::CreateTerrain(&TerrainVerticesData[0][0], sizeof(TerrainVertex), NumRows * NumCols,
			//TerrainIndices.data(), IndicesCount, MaterialIndex);
	}

	void GenerateTerrain(std::vector<u32>& Indices)
	{
		const f32 MaxAltitude = 0.0f;
		const f32 MinAltitude = -10.0f;
		const f32 SmoothMin = 7.0f;
		const f32 SmoothMax = 3.0f;
		const f32 SmoothFactor = 0.5f;
		const f32 ScaleFactor = 0.2f;

		std::mt19937 Gen(1);
		std::uniform_real_distribution<f32> Dist(MinAltitude, MaxAltitude);

		bool UpFactor = false;
		bool DownFactor = false;

		for (int i = 0; i < NumRows; ++i)
		{
			for (int j = 0; j < NumCols; ++j)
			{
				const f32 RandomAltitude = Dist(Gen);
				const f32 Probability = (RandomAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

				const f32 PreviousCornerAltitude = i > 0 && j > 0 ? TerrainVerticesData[i - 1][j - 1].Altitude : 5.0f;
				const f32 PreviousIAltitude = i > 0 ? TerrainVerticesData[i - 1][j].Altitude : 5.0f;
				const f32 PreviousJAltitude = j > 0 ? TerrainVerticesData[i][j - 1].Altitude : 5.0f;

				const f32 PreviousAverageAltitude = (PreviousCornerAltitude + PreviousIAltitude + PreviousJAltitude) / 3.0f;

				f32 NormalizedAltitude = (PreviousAverageAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

				const f32 Smooth = (PreviousAverageAltitude <= SmoothMin || PreviousAverageAltitude >= SmoothMax) ? SmoothFactor : 1.0f;

				if (UpFactor)
				{
					NormalizedAltitude *= ScaleFactor;
				}
				else if (DownFactor)
				{
					NormalizedAltitude /= ScaleFactor;
				}

				if (NormalizedAltitude > Probability)
				{
					TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude - Probability * Smooth;
					UpFactor = false;
					DownFactor = true;
				}
				else
				{
					TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude + Probability * Smooth;
					UpFactor = true;
					DownFactor = false;
				}
			}
		}

		Indices.reserve(NumRows * NumCols * 6);

		for (int row = 0; row < NumRows - 1; ++row)
		{
			for (int col = 0; col < NumCols - 1; ++col)
			{
				u32 topLeft = row * NumCols + col;
				u32 topRight = topLeft + 1;
				u32 bottomLeft = (row + 1) * NumCols + col;
				u32 bottomRight = bottomLeft + 1;

				// First triangle (Top-left, Bottom-left, Bottom-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomLeft);
				Indices.push_back(bottomRight);

				// Second triangle (Top-left, Bottom-right, Top-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomRight);
				Indices.push_back(topRight);
			}
		}
	}
}
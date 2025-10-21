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

static BmRender_Image ShadowMapArray;

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
		InitInfo.Allocator = BmRender_GetVulkanAllocator();
		ImGui_ImplVulkan_Init(&InitInfo);

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	static void DeInitImGuiPipeline(VkDevice Device, VkDescriptorPool ImGuiPool)
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	}

	static void InitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline, BmRender_GPUBufferEntry* EntityLightRegion, BmRender_DescriptorPool MainPool)
	{
		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		MeshPipeline->EntityLightBufferHandle = EntityLightRegion;

		VkDescriptorSetLayout Layout = RenderResources::GetSetLayout("ShadowMapArrayLayout")->Layout;

		for (u32 i = 0; i < RenderResources::GetCoreContext()->ImagesCount; i++)
		{
			MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i, MAX_LIGHT_SOURCES, VK_IMAGE_ASPECT_DEPTH_BIT);
			
			BmRender_DescriptorSetBinding ShadowMapBinding;
			ShadowMapBinding.ImageBinding.Sampler = "ShadowMap";
			ShadowMapBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapBinding.ImageBinding.ImageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapBinding.BindingCount = 1;
			ShadowMapBinding.DstArrayElement = 0;

			MeshPipeline->ShadowMapArraySet[i] = BmRender_CreateDescriptorSet(RenderResources::GetDescriptorSetLayoutHandle("ShadowMapArrayLayout"), MainPool);
			BmRender_UpdateDescriptorSet(MeshPipeline->ShadowMapArraySet[i], &ShadowMapBinding, 1);
		}

		PipelineResourceInfo ResourceInfo = {};
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/StaticMesh.yaml", MainScreenExtent, ResourceInfo);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = static_cast<u32>(PipelineDesc.DescriptorSetLayouts.size());
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts.data();
		LayoutDesc.PushConstantRangeCount = static_cast<u32>(PipelineDesc.PushConstantRanges.size());
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges.data();
		LayoutDesc.Flags = 0;
		LayoutDesc.Next = nullptr;

		RenderResources::CreatePipelineLayout("StaticMesh", LayoutDesc);
		PipelineDesc.PipelineLayout = RenderResources::GetPipelineLayout("StaticMesh");
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		RenderResources::CreateGraphicsPipeline("StaticMesh", PipelineDesc);
	}

	static void DrawStaticMeshes(VkDevice Device, VkCommandBuffer CmdBuffer, StaticMeshPipeline* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets, BmRender_GPUBuffer VertexStageBuffer, BmRender_GPUBuffer InstanceBuffer)
	{
		u32 CurrentImageIndex = Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
		RenderResources::UpdateBufferRegion(MeshPipeline->EntityLightBufferHandle[CurrentImageIndex], 0,
			Scene->LightEntity, sizeof(LightBuffer));

		VkPipeline Pipeline = RenderResources::GetPipeline("StaticMesh");
		VkPipelineLayout PipelineLayout = RenderResources::GetPipelineLayout("StaticMesh");

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		const VkDescriptorSet DescriptorSetGroup[] =
		{
			GetDescriptorSetData(DescriptorSets.VpSet)->Set,
			GetDescriptorSetData(DescriptorSets.BindlesTexturesSet)->Set,
			GetDescriptorSetData(DescriptorSets.StaticMeshLightSet)->Set,
			GetDescriptorSetData(DescriptorSets.MaterialSet)->Set,
			GetDescriptorSetData(MeshPipeline->ShadowMapArraySet[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Set,
		};

		const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

		const u32 VpDynamicOffset = Render::GetRenderState()->RenderDrawState.CurrentImageIndex * sizeof(ViewProjectionBuffer);
		const u32 LightDynamicOffset = Render::GetRenderState()->RenderDrawState.CurrentImageIndex * sizeof(LightBuffer);
		const u32 DynamicOffsets[] = { VpDynamicOffset, LightDynamicOffset };
		const u32 DynamicOffsetCounts[] = { 1, 0, 0, 0, 1 };

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			0, DescriptorSetGroupCount, DescriptorSetGroup, DynamicOffsetCounts[0] + DynamicOffsetCounts[1] + DynamicOffsetCounts[2] + DynamicOffsetCounts[3] + DynamicOffsetCounts[4], DynamicOffsets);

		vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &Render::GetRenderState()->RenderDrawState.CurrentImageIndex);

		std::unique_lock Lock(Scene->TempLock);
		for (u32 i = 0; i < Scene->DrawEntities.size(); ++i)
		{
			DrawEntity* DrawEntity = Scene->DrawEntities.data() + i;
			for (u32 i = 0; i < DrawEntity->ResourceDependency.size(); ++i)
			{
				if (!RenderResources::IsImageResourceReady(DrawEntity->ImageDependency[i]))
				{
					continue;
				}
			}

			for (u32 i = 0; i < DrawEntity->ResourceDependency.size(); ++i)
			{
				if (!RenderResources::IsBufferResourceReady(DrawEntity->ResourceDependency[i]))
				{
					continue;
				}
			}

			const VkBuffer Buffers[] =
			{
				GetGPUBufferData(VertexStageBuffer)->Buffer,
				GetGPUBufferData(InstanceBuffer)->Buffer
			};

			const u64 Offsets[] = 
			{
				DrawEntity->VertexOffset,
				DrawEntity->InstanceOffset
			};

			vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
			vkCmdBindIndexBuffer(CmdBuffer, GetGPUBufferData(VertexStageBuffer)->Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, DrawEntity->IndicesCount, DrawEntity->Instances, 0, 0, 0);
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

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferEntry* VpRegion, BmRender_GPUBufferEntry* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool, BmRender_GPUBuffer VertexStageBuffer, BmRender_GPUBuffer InstanceBuffer)
	{		
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

		State.VpHandle = VpRegion;
		State.DescriptorSets = DescriptorSets;
		State.MainPool = MainPool;
		State.VertexStageBuffer = VertexStageBuffer;
		State.InstanceBuffer = InstanceBuffer;

		InitDrawState(Device, RenderResources::GetCoreContext()->Indices.GraphicsFamily, VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);

		DeferredPass::Init(State.MainPool);
		MainPass::Init();
		LightningPass::Init(State.MainPool);

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(Device, &State.MeshPipeline, EntityLightRegion, State.MainPool);
		InitImGuiPipeline(&State.DebugUiPool, RenderResources::GetCoreContext(), WindowHandler);
	}

	void DeInit()
	{
		vkDeviceWaitIdle(RenderResources::GetCoreContext()->LogicalDevice);

		BmRender_DeInit();

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

		DeInitImGuiPipeline(Device, State.DebugUiPool);
		DeInitDrawState(Device, VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);

		//TerrainRender::DeInit();

		Memory::DestroyFrameMemory(&State.FrameMemory);
	}

	void* FrameAlloc(u32 Size)
	{
		return Memory::FrameAlloc(&State.FrameMemory, Size);
	}

	void Draw(DrawScene* Scene, u64 WaitSemaphoreValue)
	{
		VulkanCoreContext::VulkanCoreContext* CoreContext = RenderResources::GetCoreContext();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		const u32 CurrentFrame = State.RenderDrawState.CurrentFrame;
		u32 ImageIndex;
		VkFence FrameFence = State.RenderDrawState.Frames.Fences[CurrentFrame];
		VkSemaphore ImagesAvailable = State.RenderDrawState.Frames.ImagesAvailable[CurrentFrame];

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &FrameFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &FrameFence));
		VULKAN_CHECK_RESULT(vkAcquireNextImageKHR(Device, RenderResources::GetCoreContext()->VulkanSwapchain, UINT64_MAX, ImagesAvailable, nullptr, &ImageIndex));
		State.RenderDrawState.CurrentImageIndex = ImageIndex;

		RenderResources::UpdateBufferRegion(State.VpHandle[ImageIndex], 0, &Scene->ViewProjection, sizeof(ViewProjectionBuffer));

		VkCommandBuffer DrawCmdBuffer = State.RenderDrawState.Frames.CommandBuffers[ImageIndex];
		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(DrawCmdBuffer, &CommandBufferBeginInfo));

		LightningPass::Draw(Scene);
		MainPass::BeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(Device, DrawCmdBuffer, &State.MeshPipeline, Scene, State.DescriptorSets, State.VertexStageBuffer, State.InstanceBuffer);
		MainPass::EndPass();
		DeferredPass::BeginPass();
		DeferredPass::Draw();
		ImGui::Render();
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CmdBuffer);
		DeferredPass::EndPass();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(DrawCmdBuffer));

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};

		VkTimelineSemaphoreSubmitInfo* TimelineInfo = nullptr;

		if (WaitSemaphoreValue > 0)
		{
			State.RenderDrawState.WaitSemaphoreValueCount += WaitSemaphoreValue;

			VkTimelineSemaphoreSubmitInfo FrameTimelineInfo;
			FrameTimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			FrameTimelineInfo.waitSemaphoreValueCount = 1;
			FrameTimelineInfo.pWaitSemaphoreValues = &State.RenderDrawState.WaitSemaphoreValueCount;
		}

		VkSemaphore RenderFinished = State.RenderDrawState.Frames.RenderFinished[CurrentFrame];
		VkSwapchainKHR Swapchain = RenderResources::GetCoreContext()->VulkanSwapchain;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImagesAvailable;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &DrawCmdBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderFinished;
		SubmitInfo.pNext = TimelineInfo;

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished;
		PresentInfo.pSwapchains = &Swapchain;
		PresentInfo.pImageIndices = &ImageIndex;

		std::unique_lock Lock(CoreContext->QueueSubmitMutex);
		VULKAN_CHECK_RESULT(vkQueueSubmit(RenderResources::GetCoreContext()->GraphicsQueue, 1, &SubmitInfo, FrameFence));
		VULKAN_CHECK_RESULT(vkQueuePresentKHR(RenderResources::GetCoreContext()->GraphicsQueue, &PresentInfo));
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

	static VkDescriptorSetLayout DeferredInputLayout;

	static BmRender_Image DeferredInputDepthImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static BmRender_Image DeferredInputColorImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static BmRender_ImageView DeferredInputDepthImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static BmRender_ImageView DeferredInputColorImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static BmRender_DescriptorSet DeferredInputSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkSampler ColorSampler;
	static VkSampler DepthSampler;

	static AttachmentData PipelineAttachmentData;

	void Init(BmRender_DescriptorPool MainPool)
	{
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;

		PipelineAttachmentData.ColorAttachmentCount = 1;
		PipelineAttachmentData.ColorAttachmentFormats[0] = RenderResources::GetCoreContext()->SurfaceFormat.format;
		PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		DeferredInputLayout = RenderResources::GetSetLayout("MainPassOutputLayout")->Layout;

		for (u32 i = 0; i < RenderResources::GetCoreContext()->ImagesCount; i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			DeferredInputColorImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, ColorFormat, ImageType::ColorAttachmentSampled);
			DeferredInputDepthImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, DepthFormat, ImageType::DepthSamplad);
			
			DeferredInputColorImageInterface[i] = BmRender_CreateImageView2D(DeferredInputColorImage[i], VK_IMAGE_ASPECT_COLOR_BIT);
			DeferredInputDepthImageInterface[i] = BmRender_CreateImageView2D(DeferredInputDepthImage[i], VK_IMAGE_ASPECT_DEPTH_BIT);
			
			BmRender_DescriptorSetBinding ColorBinding;
			ColorBinding.ImageBinding.Sampler = "ColorAttachment";
			ColorBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColorBinding.ImageBinding.ImageView = DeferredInputColorImageInterface[i];
			ColorBinding.BindingCount = 1;
			ColorBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding DepthBinding;
			DepthBinding.ImageBinding.Sampler = "DepthAttachment";
			DepthBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthBinding.ImageBinding.ImageView = DeferredInputDepthImageInterface[i];
			DepthBinding.BindingCount = 1;
			DepthBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding Bindings[] = { ColorBinding, DepthBinding };

			DeferredInputSet[i] = BmRender_CreateDescriptorSet(RenderResources::GetDescriptorSetLayoutHandle("MainPassOutputLayout"), MainPool);
			BmRender_UpdateDescriptorSet(DeferredInputSet[i], Bindings, 2);
		}


		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData = PipelineAttachmentData;

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/DeferredPipeline.yaml", MainScreenExtent, ResourceInfo);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = static_cast<u32>(PipelineDesc.DescriptorSetLayouts.size());
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts.data();
		LayoutDesc.PushConstantRangeCount = static_cast<u32>(PipelineDesc.PushConstantRanges.size());
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges.data();
		LayoutDesc.Flags = 0;
		LayoutDesc.Next = nullptr;

		RenderResources::CreatePipelineLayout("Deferred", LayoutDesc);
		PipelineDesc.PipelineLayout = RenderResources::GetPipelineLayout("Deferred");
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		RenderResources::CreateGraphicsPipeline("Deferred", PipelineDesc);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];

		VkPipeline Pipeline = RenderResources::GetPipeline("Deferred");
		VkPipelineLayout PipelineLayout = RenderResources::GetPipelineLayout("Deferred");

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			0, 1, &GetDescriptorSetData(DeferredInputSet[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Set, 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void BeginPass()
	{
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingAttachmentInfo SwapchainColorAttachment = { };
		SwapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		SwapchainColorAttachment.imageView = RenderResources::GetCoreContext()->ImageViews[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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
		ColorBarrier.image = GetImageData(DeferredInputColorImage[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Image;
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
		DepthBarrier.image = GetImageData(DeferredInputDepthImage[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Image;
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
		SwapchainAcquireBarrier.image = RenderResources::GetCoreContext()->Images[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
		vkCmdEndRendering(CmdBuffer);

		VkImageMemoryBarrier2 SwapchainPresentBarrier = { };
		SwapchainPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.image = RenderResources::GetCoreContext()->Images[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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





	BmRender_ImageView* TestDeferredInputColorImageInterface()
	{
		return DeferredInputColorImageInterface;
	}

	BmRender_ImageView* TestDeferredInputDepthImageInterface()
	{
		return DeferredInputDepthImageInterface;
	}

	BmRender_Image* TestDeferredInputColorImage()
	{
		return DeferredInputColorImage;
	}

	BmRender_Image* TestDeferredInputDepthImage()
	{
		return DeferredInputDepthImage;
	}

	AttachmentData* GetAttachmentData()
	{
		return &PipelineAttachmentData;
	}
}

namespace LightningPass
{
	static VkDescriptorSetLayout LightSpaceMatrixLayout;

	static BmRender_DescriptorSet LightSpaceMatrixSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static BmRender_GPUBufferEntry LightSpaceMatrixBufferRegion[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	
	// Buffer handles array
	static BmRender_GPUBuffer LightSpaceMatrixBuffers[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static BmRender_ImageView ShadowMapElement1ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static BmRender_ImageView ShadowMapElement2ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	void Init(BmRender_DescriptorPool MainPool)
	{
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;

		LightSpaceMatrixLayout = RenderResources::GetSetLayout("LightSpaceMatrixLayout")->Layout;

		ShadowMapArray = BmRender_CreateImage2DArray(DepthViewportExtent.width, DepthViewportExtent.height, DepthFormat,
			ImageType::DepthSamplad, MAX_LIGHT_SOURCES * RenderResources::GetCoreContext()->ImagesCount);

		for (u32 i = 0; i < RenderResources::GetCoreContext()->ImagesCount; i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			LightSpaceMatrixBuffers[i] = BmRender_CreateUniformBuffer(LightSpaceMatrixSize, BufferUpdateFrequency::PerFrame, PipelineStage::Vertex);
			LightSpaceMatrixBufferRegion[i] = RenderResources::BmRender_CreateGPUBufferEntry(0, LightSpaceMatrixSize, LightSpaceMatrixBuffers[i]);

			LightSpaceMatrixSet[i] = BmRender_CreateDescriptorSet(RenderResources::GetDescriptorSetLayoutHandle("LightSpaceMatrixLayout"), MainPool);

			BmRender_DescriptorSetBinding LightSpaceMatrixBinding;
			LightSpaceMatrixBinding.BufferRegions = &LightSpaceMatrixBufferRegion[i];
			LightSpaceMatrixBinding.BindingCount = 1;
			LightSpaceMatrixBinding.DstArrayElement = 0;

			BmRender_UpdateDescriptorSet(LightSpaceMatrixSet[i], &LightSpaceMatrixBinding, 1);

			ShadowMapElement1ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i, 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
			ShadowMapElement2ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i + 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		}

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(glm::mat4);

		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 0;
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/DepthPipeline.yaml", DepthViewportExtent, ResourceInfo);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = static_cast<u32>(PipelineDesc.DescriptorSetLayouts.size());
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts.data();
		LayoutDesc.PushConstantRangeCount = static_cast<u32>(PipelineDesc.PushConstantRanges.size());
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges.data();
		LayoutDesc.Flags = 0;
		LayoutDesc.Next = nullptr;

		RenderResources::CreatePipelineLayout("Depth", LayoutDesc);
		PipelineDesc.PipelineLayout = RenderResources::GetPipelineLayout("Depth");
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		RenderResources::CreateGraphicsPipeline("Depth", PipelineDesc);
	}

	void Draw(Render::DrawScene* Scene)
	{
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		VkImageView Attachments[2];
		Attachments[0] = GetImageViewData(ShadowMapElement1ImageInterface[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->View;
		Attachments[1] = GetImageViewData(ShadowMapElement2ImageInterface[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->View;

		VkImageMemoryBarrier2 DepthAttachmentTransitionBefore = { };
		DepthAttachmentTransitionBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthAttachmentTransitionBefore.srcStageMask = VK_PIPELINE_STAGE_2_NONE; // because we're coming from UNDEFINED
		DepthAttachmentTransitionBefore.srcAccessMask = 0;
		DepthAttachmentTransitionBefore.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthAttachmentTransitionBefore.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthAttachmentTransitionBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachmentTransitionBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionBefore.image = GetImageData(ShadowMapArray)->Image;
		DepthAttachmentTransitionBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionBefore.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionBefore.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionBefore.subresourceRange.baseArrayLayer = MAX_LIGHT_SOURCES * Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
		DepthAttachmentTransitionBefore.subresourceRange.layerCount = MAX_LIGHT_SOURCES;

		VkDependencyInfo DependencyInfoBefore = { };
		DependencyInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DependencyInfoBefore.imageMemoryBarrierCount = 1;
		DependencyInfoBefore.pImageMemoryBarriers = &DepthAttachmentTransitionBefore,

		vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoBefore);

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			RenderResources::UpdateBufferRegion(LightSpaceMatrixBufferRegion[LightCaster], 0, LightViews[LightCaster], sizeof(glm::mat4));

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

			vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

			VkPipeline Pipeline = RenderResources::GetPipeline("Depth");
			VkPipelineLayout PipelineLayout = RenderResources::GetPipelineLayout("Depth");

			vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

			std::unique_lock Lock(Scene->TempLock);
			for (u32 i = 0; i < Scene->DrawEntities.size(); ++i)
			{
				Render::DrawEntity* DrawEntity = Scene->DrawEntities.data() + i;
				for (u32 i = 0; i < DrawEntity->ResourceDependency.size(); ++i)
				{
					if (!RenderResources::IsImageResourceReady(DrawEntity->ImageDependency[i]))
					{
						continue;
					}
				}

				for (u32 i = 0; i < DrawEntity->ResourceDependency.size(); ++i)
				{
					if (!RenderResources::IsBufferResourceReady(DrawEntity->ResourceDependency[i]))
					{
						continue;
					}
				}

				const VkBuffer Buffers[] =
				{
					GetGPUBufferData(State->VertexStageBuffer)->Buffer,
					GetGPUBufferData(State->InstanceBuffer)->Buffer
				};

				const u64 Offsets[] =
				{
					DrawEntity->VertexOffset,
					DrawEntity->InstanceOffset
				};

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					GetDescriptorSetData(LightSpaceMatrixSet[LightCaster])->Set,
				};

				vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr);

				vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
				vkCmdBindIndexBuffer(CmdBuffer, GetGPUBufferData(State->VertexStageBuffer)->Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CmdBuffer, DrawEntity->IndicesCount, DrawEntity->Instances, 0, 0, 0);
			}

			vkCmdEndRendering(CmdBuffer);
		}

		// TODO: move to Main pass?
		VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
		DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

		DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthAttachmentTransitionAfter.image = GetImageData(ShadowMapArray)->Image;
		DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = MAX_LIGHT_SOURCES * Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
		DepthAttachmentTransitionAfter.subresourceRange.layerCount = MAX_LIGHT_SOURCES;
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
}

namespace MainPass
{
	static VkDescriptorSetLayout SkyBoxLayout;

	static VkDescriptorSet SkyBoxSet;

	static AttachmentData PipelineAttachmentData;

	void Init()
	{
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

		PipelineAttachmentData.ColorAttachmentCount = 1;
		PipelineAttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;


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

		//VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(RenderResources::GetCoreContext()->LogicalDevice, &LayoutCreateInfo, nullptr, &SkyBoxLayout));

		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData = PipelineAttachmentData;

		//RenderResources::BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/SkyBoxPipeline.yaml", MainScreenExtent, ResourceInfo);

		//// Create pipeline layout from parsed descriptor set layouts
		//RenderResources::BmRender_PipelineLayoutDescription LayoutDesc = {};
		//LayoutDesc.SetLayoutCount = static_cast<u32>(PipelineDesc.DescriptorSetLayouts.size());
		//LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts.data();
		//LayoutDesc.PushConstantRangeCount = 0;
		//LayoutDesc.PushConstantRanges = nullptr;
		//LayoutDesc.Flags = 0;
		//LayoutDesc.Next = nullptr;

		//RenderResources::CreatePipelineLayout("SkyBox", LayoutDesc);
		//PipelineDesc.PipelineLayout = RenderResources::GetPipelineLayout("SkyBox");
		//ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		//RenderResources::CreateGraphicsPipeline("SkyBox", PipelineDesc);
	}

	void BeginPass()
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];
		VkRenderingAttachmentInfo ColorAttachment = { };
		ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		ColorAttachment.imageView = GetImageViewData(DeferredPass::TestDeferredInputColorImageInterface()[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->View;
		ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo DepthAttachment = { };
		DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachment.imageView = GetImageViewData(DeferredPass::TestDeferredInputDepthImageInterface()[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->View;
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
		ColorBarrierBefore.image = GetImageData(DeferredPass::TestDeferredInputColorImage()[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Image;
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
		DepthBarrierBefore.image = GetImageData(DeferredPass::TestDeferredInputDepthImage()[Render::GetRenderState()->RenderDrawState.CurrentImageIndex])->Image;
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

		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];

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
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];

		vkCmdEndRendering(CmdBuffer);
	}

	AttachmentData* GetAttachmentData()
	{
		return &PipelineAttachmentData;
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

	static Render::DrawEntity TerrainDrawObject;

	static VkPushConstantRange PushConstants;

	void Init()
	{
		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

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

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(PushConstantsData);

		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/TerrainPipeline.yaml", MainScreenExtent, ResourceInfo);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = static_cast<u32>(PipelineDesc.DescriptorSetLayouts.size());
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts.data();
		LayoutDesc.PushConstantRangeCount = static_cast<u32>(PipelineDesc.PushConstantRanges.size());
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges.data();
		LayoutDesc.Flags = 0;
		LayoutDesc.Next = nullptr;

		RenderResources::CreatePipelineLayout("Terrain", LayoutDesc);
		PipelineDesc.PipelineLayout = RenderResources::GetPipelineLayout("Terrain");
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		RenderResources::CreateGraphicsPipeline("Terrain", PipelineDesc);

		LoadTerrain();
	}

	void Draw()
	{
		const Render::RenderState* State = Render::GetRenderState();

		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];

		const VkDescriptorSet Sets[] = {
			GetDescriptorSetData(State->DescriptorSets.VpSet)->Set,
			GetDescriptorSetData(State->DescriptorSets.BindlesTexturesSet)->Set,
			GetDescriptorSetData(State->DescriptorSets.MaterialSet)->Set,
		};

		const u32 TerrainDescriptorSetGroupCount = sizeof(Sets) / sizeof(Sets[0]);

		VkPipeline Pipeline = RenderResources::GetPipeline("Terrain");
		VkPipelineLayout PipelineLayout = RenderResources::GetPipelineLayout("Terrain");

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		const u32 DynamicOffset = Render::GetRenderState()->RenderDrawState.CurrentImageIndex * sizeof(Render::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			0, TerrainDescriptorSetGroupCount, Sets, 1, &DynamicOffset);

		PushConstantsData Constants;
		//Constants.Model = TerrainDrawObject.Model;
		//Constants.matIndex = TerrainDrawObject.MaterialIndex;

		const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		vkCmdPushConstants(CmdBuffer, PipelineLayout, Flags, 0, sizeof(PushConstantsData), &Constants);

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

		//RenderResources::Material Mat = { };
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
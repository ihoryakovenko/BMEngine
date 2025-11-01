#include "Render.h"

#include "Engine/Systems/Render/VulkanHelper.h"
#include "RenderResources.h"
#include "RenderTypes.h"
#include "TransferSystem.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/Settings.h"
#include "Util/Math.h"

#include <random>
#include <mutex>

// Extern declarations for global resource maps
extern std::unordered_map<std::string, Util::VertexBinding_depr> VBindings;
extern std::unordered_map<std::string, BmRender_Sampler> Samplers;
extern std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
extern std::unordered_map<std::string, BmRender_Shader> Shaders;
extern std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
extern std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;
extern std::unordered_map<std::string, BmRender_PushConstant> PushConstants;

static BmRender_Image2DArray ShadowMapArray;

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
		InitInfo.Allocator = GetVulkanAllocator();
		ImGui_ImplVulkan_Init(&InitInfo);

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	static void DeInitImGuiPipeline(VkDevice Device, VkDescriptorPool ImGuiPool)
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	}

	static void InitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline, BmRender_DescriptorPool MainPool)
	{
		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray.Image, MAX_LIGHT_SOURCES * i, MAX_LIGHT_SOURCES, VK_IMAGE_ASPECT_DEPTH_BIT);
			
			BmRender_DescriptorSetBinding ShadowMapBinding;
			ShadowMapBinding.ImageBinding.Sampler = Samplers["ShadowMap"];
			ShadowMapBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapBinding.ImageBinding.ImageView = MeshPipeline->ShadowMapArrayImageInterface[i].View;
			ShadowMapBinding.BindingCount = 1;
			ShadowMapBinding.DstArrayElement = 0;

			MeshPipeline->ShadowMapArraySet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["ShadowMapArrayLayout"], MainPool);
			BmRender_UpdateDescriptorSet(MeshPipeline->ShadowMapArraySet[i], &ShadowMapBinding, 1);
		}

		PipelineResourceInfo ResourceInfo = {};
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/StaticMesh.yaml", MainScreenExtent, ResourceInfo, 
			shaderStages, vertexBindings, descriptorSetLayouts, pushConstantRanges);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;

		PipelineLayouts["StaticMesh"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["StaticMesh"];
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		Pipelines["StaticMesh"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	static void DrawStaticMeshes(VkDevice Device, VkCommandBuffer CmdBuffer, StaticMeshPipeline* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets)
	{
		u32 CurrentImageIndex = Render::GetRenderState()->RenderDrawState.CurrentFrame;

		const BmRender_DescriptorSet DescriptorSetGroup[] =
		{
			DescriptorSets.VpSet,
			DescriptorSets.BindlesTexturesSet,
			DescriptorSets.StaticMeshLightSet,
			DescriptorSets.MaterialSet,
			MeshPipeline->ShadowMapArraySet[CurrentImageIndex],
		};

		const u32 VpDynamicOffset = CurrentImageIndex * sizeof(ViewProjectionBuffer);
		const u32 LightDynamicOffset = CurrentImageIndex * sizeof(LightBuffer);
		const u32 DynamicOffsets[] = { VpDynamicOffset, LightDynamicOffset };

		DrawEntityBatchConfig Config = {};
		Config.Pipeline = Pipelines["StaticMesh"];
		Config.PipelineLayout = PipelineLayouts["StaticMesh"];
		Config.DescriptorSets = DescriptorSetGroup;
		Config.DescriptorSetCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);
		Config.DynamicOffsetCount = sizeof(DynamicOffsets) / sizeof(DynamicOffsets[0]);
		Config.DynamicOffsets = DynamicOffsets;
		Config.PushConstant = PushConstants["MainConstant"];
		Config.PushConstantData = &CurrentImageIndex;

		DrawEntityBatch(CmdBuffer, Scene, Config);
	}

	void DrawEntityBatch(VkCommandBuffer CmdBuffer, DrawScene* Scene, const DrawEntityBatchConfig& Config)
	{
		VkPipeline Pipeline = GetPipelineData(Config.Pipeline)->VulkanPipeline;
		VkPipelineLayout PipelineLayout = GetPipelineLayoutData(Config.PipelineLayout)->VulkanPipelineLayout;

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		if (Config.PushConstant.Private != 0 && Config.PushConstantData != nullptr)
		{
			VkPushConstantRange PushConstantRange = GetPushConstantData(Config.PushConstant)->PushConstants;
			vkCmdPushConstants(CmdBuffer, PipelineLayout, PushConstantRange.stageFlags, 
				PushConstantRange.offset, PushConstantRange.size, Config.PushConstantData);
		}

		if (Config.DescriptorSetCount > 0)
		{
			VkDescriptorSet* VkDescriptorSets = (VkDescriptorSet*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkDescriptorSet) * Config.DescriptorSetCount);
			for (u32 i = 0; i < Config.DescriptorSetCount; ++i)
			{
				VkDescriptorSets[i] = GetDescriptorSetData(Config.DescriptorSets[i])->Set;
			}

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
				0, Config.DescriptorSetCount, VkDescriptorSets, Config.DynamicOffsetCount, Config.DynamicOffsets);
		}

		std::unique_lock Lock(Scene->TempLock);
		
		for (u32 i = 0; i < Scene->DrawEntities.size(); ++i)
		{
			DrawEntity* Entity = Scene->DrawEntities.data() + i;
		
			bool AreDependenciesReady = true;
			//for (u32 j = 0; j < Entity->ImageDependency.size(); ++j)
			//{
			//	if (TransferSystem::IsImageLocked(Entity->ImageDependency[j]))
			//	{
			//		AreDependenciesReady = false;
			//		break;
			//	}
			//}

			if (!AreDependenciesReady)
			{
				continue;
			}
		
		for (u32 j = 0; j < Entity->ResourceDependency.size(); ++j)
		{
			if (TransferSystem::IsBufferLocked(Entity->ResourceDependency[j].GPUBufferHandle))
			{
				AreDependenciesReady = false;
				break;
			}
		}

			if (!AreDependenciesReady)
			{
				continue;
			}

		const VkBuffer Buffers[] = {
			GetGPUBufferData(Entity->VertexBufferEntry.GPUBufferHandle)->Buffer,
			GetGPUBufferData(Entity->InstanceBufferEntry.GPUBufferHandle)->Buffer
		};
	
		const u64 Offsets[] = {
			Entity->VertexBufferEntry.BufferOffset,
			Entity->InstanceBufferEntry.BufferOffset
		};
	
		vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
		vkCmdBindIndexBuffer(CmdBuffer, GetGPUBufferData(Entity->IndexBufferEntry.GPUBufferHandle)->Buffer, Entity->IndexBufferEntry.BufferOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, Entity->IndicesCount, Entity->Instances, 0, 0, 0);
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

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferBinding* VpRegion, BmRender_GPUBufferBinding* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool)
	{		
		VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;
		VkDevice Device = GetCoreContext()->LogicalDevice;

		State.VpHandle = VpRegion;
		State.EntityLightBufferHandle = EntityLightRegion;
		State.DescriptorSets = DescriptorSets;
		State.MainPool = MainPool;

		InitDrawState(Device, GetCoreContext()->Indices.GraphicsFamily, VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);

		DeferredPass::Init(State.MainPool);
		MainPass::Init();
		LightningPass::Init(State.MainPool);

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(Device, &State.MeshPipeline, State.MainPool);
		InitImGuiPipeline(&State.DebugUiPool, GetCoreContext(), WindowHandler);
	}

	void DeInit()
	{
		vkDeviceWaitIdle(GetCoreContext()->LogicalDevice);


		VkDevice Device = GetCoreContext()->LogicalDevice;

		DeInitImGuiPipeline(Device, State.DebugUiPool);
		DeInitDrawState(Device, VulkanHelper::MAX_DRAW_FRAMES, &State.RenderDrawState);
		
		//TerrainRender::DeInit();

	}

	void Test_FrameFree()
	{
		Memory::FrameFree(GetFrameMemory());
	}

	void Draw(DrawScene* Scene, u64 WaitSemaphoreValue)
	{
		VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkDevice Device = GetCoreContext()->LogicalDevice;
		const u32 CurrentFrame = State.RenderDrawState.CurrentFrame;
		u32 ImageIndex;
		VkFence FrameFence = State.RenderDrawState.Frames.Fences[CurrentFrame];
		VkSemaphore ImagesAvailable = State.RenderDrawState.Frames.ImagesAvailable[CurrentFrame];

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &FrameFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &FrameFence));
		VULKAN_CHECK_RESULT(vkAcquireNextImageKHR(Device, GetCoreContext()->VulkanSwapchain, UINT64_MAX, ImagesAvailable, nullptr, &ImageIndex));
		State.RenderDrawState.CurrentImageIndex = ImageIndex;

		RenderResources::UpdateBufferRegion(State.VpHandle[CurrentFrame], 0, &Scene->ViewProjection, sizeof(ViewProjectionBuffer));
		RenderResources::UpdateBufferRegion(State.EntityLightBufferHandle[CurrentFrame], 0, Scene->LightEntity, sizeof(LightBuffer));

		VkCommandBuffer DrawCmdBuffer = State.RenderDrawState.Frames.CommandBuffers[CurrentFrame];
		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(DrawCmdBuffer, &CommandBufferBeginInfo));

		LightningPass::Draw(Scene);
		MainPass::BeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(Device, DrawCmdBuffer, &State.MeshPipeline, Scene, State.DescriptorSets);
		MainPass::EndPass();
		DeferredPass::BeginPass();
		DeferredPass::Draw();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), DrawCmdBuffer);
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
		VkSwapchainKHR Swapchain = GetCoreContext()->VulkanSwapchain;

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
		VULKAN_CHECK_RESULT(vkQueueSubmit(GetCoreContext()->GraphicsQueue, 1, &SubmitInfo, FrameFence));
		VULKAN_CHECK_RESULT(vkQueuePresentKHR(GetCoreContext()->GraphicsQueue, &PresentInfo));
		Lock.unlock();

		State.RenderDrawState.CurrentFrame = Math::WrapIncrement(CurrentFrame, 3u);

		Test_FrameFree();
	}

	RenderState* GetRenderState()
	{
		return &State;
	}
}








namespace DeferredPass
{

	static VkDescriptorSetLayout DeferredInputLayout;

		static BmRender_Image2D DeferredInputDepthImage[VulkanHelper::MAX_DRAW_FRAMES];
		static BmRender_Image2D DeferredInputColorImage[VulkanHelper::MAX_DRAW_FRAMES];
		
		static BmRender_ImageView2D DeferredInputDepthImageInterface[VulkanHelper::MAX_DRAW_FRAMES];
		static BmRender_ImageView2D DeferredInputColorImageInterface[VulkanHelper::MAX_DRAW_FRAMES];

	static BmRender_DescriptorSet DeferredInputSet[VulkanHelper::MAX_DRAW_FRAMES];

	static VkSampler ColorSampler;
	static VkSampler DepthSampler;

	static AttachmentData PipelineAttachmentData;

	void Init(BmRender_DescriptorPool MainPool)
	{
		VkDevice Device = GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

		PipelineAttachmentData.ColorAttachmentCount = 1;
		PipelineAttachmentData.ColorAttachmentFormats[0] = GetCoreContext()->SurfaceFormat.format;
		PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		DeferredInputLayout = GetDescriptorSetLayoutData(DescriptorSetLayouts["MainPassOutputLayout"])->Layout;

		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			DeferredInputColorImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, ColorFormat, BmRender_ImageType::ColorAttachmentSampled);
			DeferredInputDepthImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, DepthFormat, BmRender_ImageType::DepthSamplad);
			
			DeferredInputColorImageInterface[i] = BmRender_CreateImageView2D(DeferredInputColorImage[i].Image, VK_IMAGE_ASPECT_COLOR_BIT);
			DeferredInputDepthImageInterface[i] = BmRender_CreateImageView2D(DeferredInputDepthImage[i].Image, VK_IMAGE_ASPECT_DEPTH_BIT);
			
			BmRender_DescriptorSetBinding ColorBinding;
			ColorBinding.ImageBinding.Sampler = Samplers["ColorAttachment"];
			ColorBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColorBinding.ImageBinding.ImageView = DeferredInputColorImageInterface[i].View;
			ColorBinding.BindingCount = 1;
			ColorBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding DepthBinding;
			DepthBinding.ImageBinding.Sampler = Samplers["DepthAttachment"];
			DepthBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthBinding.ImageBinding.ImageView = DeferredInputDepthImageInterface[i].View;
			DepthBinding.BindingCount = 1;
			DepthBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding Bindings[] = { ColorBinding, DepthBinding };

			DeferredInputSet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["MainPassOutputLayout"], MainPool);
			BmRender_UpdateDescriptorSet(DeferredInputSet[i], Bindings, 2);
		}


		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData = PipelineAttachmentData;

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/DeferredPipeline.yaml", MainScreenExtent, ResourceInfo, 
			shaderStages, vertexBindings, descriptorSetLayouts, pushConstantRanges);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;

		PipelineLayouts["Deferred"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["Deferred"];
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		Pipelines["Deferred"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];

		VkPipeline Pipeline = GetPipelineData(Pipelines["Deferred"])->VulkanPipeline;
		VkPipelineLayout PipelineLayout = GetPipelineLayoutData(PipelineLayouts["Deferred"])->VulkanPipelineLayout;

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			0, 1, &GetDescriptorSetData(DeferredInputSet[Render::GetRenderState()->RenderDrawState.CurrentFrame])->Set, 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void BeginPass()
	{
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingAttachmentInfo SwapchainColorAttachment = { };
		SwapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		SwapchainColorAttachment.imageView = GetCoreContext()->ImageViews[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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
		ColorBarrier.image = GetImageData(DeferredInputColorImage[Render::GetRenderState()->RenderDrawState.CurrentFrame].Image)->Image;
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
		DepthBarrier.image = GetImageData(DeferredInputDepthImage[Render::GetRenderState()->RenderDrawState.CurrentFrame].Image)->Image;
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
		SwapchainAcquireBarrier.image = GetCoreContext()->Images[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];
		vkCmdEndRendering(CmdBuffer);

		VkImageMemoryBarrier2 SwapchainPresentBarrier = { };
		SwapchainPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.image = GetCoreContext()->Images[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
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





	BmRender_ImageView2D* TestDeferredInputColorImageInterface()
	{
		return DeferredInputColorImageInterface;
	}

	BmRender_ImageView2D* TestDeferredInputDepthImageInterface()
	{
		return DeferredInputDepthImageInterface;
	}

	BmRender_Image2D* TestDeferredInputColorImage()
	{
		return DeferredInputColorImage;
	}

	BmRender_Image2D* TestDeferredInputDepthImage()
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

	static BmRender_DescriptorSet LightSpaceMatrixSet[VulkanHelper::MAX_DRAW_FRAMES];

	static BmRender_GPUBufferBinding LightSpaceMatrixBufferRegion[VulkanHelper::MAX_DRAW_FRAMES];
	
	// Buffer handles array
	static BmRender_UniformBuffer LightSpaceMatrixBuffers[VulkanHelper::MAX_DRAW_FRAMES];

		static BmRender_ImageView2DArray ShadowMapElement1ImageInterface[VulkanHelper::MAX_DRAW_FRAMES];
		static BmRender_ImageView2DArray ShadowMapElement2ImageInterface[VulkanHelper::MAX_DRAW_FRAMES];

	static VkPushConstantRange PushConstants;

	void Init(BmRender_DescriptorPool MainPool)
	{
		VkDevice Device = GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

		LightSpaceMatrixLayout = GetDescriptorSetLayoutData(DescriptorSetLayouts["LightSpaceMatrixLayout"])->Layout;

		BmRender_Image2DArray ShadowMapArrayHandle = BmRender_CreateImage2DArray(DepthViewportExtent.width, DepthViewportExtent.height, DepthFormat,
			BmRender_ImageType::DepthSamplad, MAX_LIGHT_SOURCES * GetCoreContext()->ImagesCount);
		ShadowMapArray = ShadowMapArrayHandle;

		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			LightSpaceMatrixBuffers[i] = BmRender_CreateUniformBuffer(LightSpaceMatrixSize, BmRender_BufferUpdateFrequency::PerFrame, BmRender_PipelineSyncStage::VertexShader);
			LightSpaceMatrixBufferRegion[i] = { LightSpaceMatrixBuffers[i].Buffer, 0, LightSpaceMatrixSize };

			LightSpaceMatrixSet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["LightSpaceMatrixLayout"], MainPool);

			BmRender_DescriptorSetBinding LightSpaceMatrixBinding;
			LightSpaceMatrixBinding.BufferRegions = &LightSpaceMatrixBufferRegion[i];
			LightSpaceMatrixBinding.BindingCount = 1;
			LightSpaceMatrixBinding.DstArrayElement = 0;

			BmRender_UpdateDescriptorSet(LightSpaceMatrixSet[i], &LightSpaceMatrixBinding, 1);

			ShadowMapElement1ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray.Image, MAX_LIGHT_SOURCES * i, 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
			ShadowMapElement2ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray.Image, MAX_LIGHT_SOURCES * i + 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		}

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(glm::mat4);

		PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 0;
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		BmRender_PipelineDescription PipelineDesc = Util::ParsePipelineFromYaml("./Resources/Settings/DepthPipeline.yaml", DepthViewportExtent, ResourceInfo, 
			shaderStages, vertexBindings, descriptorSetLayouts, pushConstantRanges);

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;

		PipelineLayouts["Depth"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["Depth"];
		ResourceInfo.PipelineLayout = PipelineDesc.PipelineLayout;

		Pipelines["Depth"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	void Draw(Render::DrawScene* Scene)
	{
		VkDevice Device = GetCoreContext()->LogicalDevice;
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		VkImageView Attachments[2];
		Attachments[0] = GetImageViewData(ShadowMapElement1ImageInterface[Render::GetRenderState()->RenderDrawState.CurrentFrame].View)->View;
		Attachments[1] = GetImageViewData(ShadowMapElement2ImageInterface[Render::GetRenderState()->RenderDrawState.CurrentFrame].View)->View;

		VkImageMemoryBarrier2 DepthAttachmentTransitionBefore = { };
		DepthAttachmentTransitionBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthAttachmentTransitionBefore.srcStageMask = VK_PIPELINE_STAGE_2_NONE; // because we're coming from UNDEFINED
		DepthAttachmentTransitionBefore.srcAccessMask = 0;
		DepthAttachmentTransitionBefore.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthAttachmentTransitionBefore.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthAttachmentTransitionBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachmentTransitionBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionBefore.image = GetImageData(ShadowMapArray.Image)->Image;
		DepthAttachmentTransitionBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionBefore.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionBefore.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionBefore.subresourceRange.baseArrayLayer = MAX_LIGHT_SOURCES * Render::GetRenderState()->RenderDrawState.CurrentFrame;
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

			const BmRender_DescriptorSet DescriptorSetGroup[] = {
				LightSpaceMatrixSet[LightCaster],
			};

			Render::DrawEntityBatchConfig Config = {};
			Config.Pipeline = Pipelines["Depth"];
			Config.PipelineLayout = PipelineLayouts["Depth"];
			Config.DescriptorSets = DescriptorSetGroup;
			Config.DescriptorSetCount = 1;
			Config.DynamicOffsetCount = 0;
			Config.DynamicOffsets = nullptr;
			Config.PushConstant.Private = 0;
			Config.PushConstantData = nullptr;

			Render::DrawEntityBatch(CmdBuffer, Scene, Config);

			vkCmdEndRendering(CmdBuffer);
		}

		// TODO: move to Main pass?
		VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
		DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

		DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthAttachmentTransitionAfter.image = GetImageData(ShadowMapArray.Image)->Image;
		DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = MAX_LIGHT_SOURCES * Render::GetRenderState()->RenderDrawState.CurrentFrame;
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
	static AttachmentData PipelineAttachmentData;

	void Init()
	{
		PipelineAttachmentData.ColorAttachmentCount = 1;
		PipelineAttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
	}

	void BeginPass()
	{
		VkRenderingAttachmentInfo ColorAttachment = { };
		ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		ColorAttachment.imageView = GetImageViewData(DeferredPass::TestDeferredInputColorImageInterface()[Render::GetRenderState()->RenderDrawState.CurrentFrame].View)->View;
		ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo DepthAttachment = { };
		DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachment.imageView = GetImageViewData(DeferredPass::TestDeferredInputDepthImageInterface()[Render::GetRenderState()->RenderDrawState.CurrentFrame].View)->View;
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
		ColorBarrierBefore.image = GetImageData(DeferredPass::TestDeferredInputColorImage()[Render::GetRenderState()->RenderDrawState.CurrentFrame].Image)->Image;
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
		DepthBarrierBefore.image = GetImageData(DeferredPass::TestDeferredInputDepthImage()[Render::GetRenderState()->RenderDrawState.CurrentFrame].Image)->Image;
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

		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfoBefore);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
	}

	void EndPass()
	{
		VkCommandBuffer CmdBuffer = Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentFrame];

		vkCmdEndRendering(CmdBuffer);
	}

	AttachmentData* GetAttachmentData()
	{
		return &PipelineAttachmentData;
	}
}
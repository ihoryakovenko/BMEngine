#include "Render.h"

#include "Engine/Systems/Render/VulkanHelper.h"
#include "RenderResources.h"
#include "RenderTypes.h"
#include "TransferSystem.h"
#include "Systems.h"
#include "Handles.h"

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
		InitInfo.Queue = GetCommandSystemData()->GraphicsQueue;
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
			MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i, MAX_LIGHT_SOURCES);
			
			BmRender_DescriptorSetBinding ShadowMapBinding;
			ShadowMapBinding.ImageBinding.Sampler = Samplers["ShadowMap"];
			ShadowMapBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapBinding.ImageBinding.ImageView = MeshPipeline->ShadowMapArrayImageInterface[i];
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

	static void DrawStaticMeshes(VkDevice Device, BmRender_CommandBuffer CommandBuffer, StaticMeshPipeline* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets)
	{
		u32 CurrentImageIndex = GetDrawSystemData()->CurrentFrame;

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

		DrawEntityBatch(CommandBuffer, Scene, Config);
	}

	void DrawEntityBatch(BmRender_CommandBuffer CommandBuffer, DrawScene* Scene, const DrawEntityBatchConfig& Config)
	{
		VkCommandBuffer CmdBuffer = (VkCommandBuffer)CommandBuffer;
		VkPipelineLayout PipelineLayout = (VkPipelineLayout)Config.PipelineLayout;

		BmRender_BindPipeline(CommandBuffer, Config.Pipeline);

		if (Config.PushConstant.offset != 0 || Config.PushConstant.size != 0)
		{
			VkPushConstantRange PushConstantRange;
			PushConstantRange.offset = Config.PushConstant.offset;
			PushConstantRange.size = Config.PushConstant.size;
			PushConstantRange.stageFlags = Config.PushConstant.stageFlags;
			vkCmdPushConstants(CmdBuffer, PipelineLayout, PushConstantRange.stageFlags, 
				PushConstantRange.offset, PushConstantRange.size, Config.PushConstantData);
		}

		if (Config.DescriptorSetCount > 0)
		{
			VkDescriptorSet* VkDescriptorSets = (VkDescriptorSet*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkDescriptorSet) * Config.DescriptorSetCount);
			for (u32 i = 0; i < Config.DescriptorSetCount; ++i)
			{
				DescriptorSetData SetData;
				GetDescriptorSetData(Config.DescriptorSets[i], &SetData);
				VkDescriptorSets[i] = SetData.Set;
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
			(VkBuffer)Entity->VertexBufferEntry.GPUBufferHandle,
			(VkBuffer)Entity->InstanceBufferEntry.GPUBufferHandle
		};
	
		const u64 Offsets[] = {
			Entity->VertexBufferEntry.BufferOffset,
			Entity->InstanceBufferEntry.BufferOffset
		};
	
		vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
		vkCmdBindIndexBuffer(CmdBuffer, (VkBuffer)Entity->IndexBufferEntry.GPUBufferHandle, Entity->IndexBufferEntry.BufferOffset, VK_INDEX_TYPE_UINT32);
			BmRender_DrawIndexed(CommandBuffer, Entity->IndicesCount, Entity->Instances, 0, 0, 0);
		}
	}

	static RenderState State;
	static u32 CurrentImageIndex;

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferBinding* VpRegion, BmRender_GPUBufferBinding* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool)
	{		
		InitCommandSystem(3);
		InitDrawSystem(3);

		VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;
		VkDevice Device = GetCoreContext()->LogicalDevice;

		State.VpHandle = VpRegion;
		State.EntityLightBufferHandle = EntityLightRegion;
		State.DescriptorSets = DescriptorSets;
		State.MainPool = MainPool;

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
		
		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			BmRender_DestroyImageView(State.MeshPipeline.ShadowMapArrayImageInterface[i]);
		}

		DeferredPass::DeInit();
		LightningPass::DeInit();

		for (auto& [name, pipeline] : Pipelines)
		{
			BmRender_DestroyPipeline(pipeline);
		}

		for (auto& [name, layout] : PipelineLayouts)
		{
			BmRender_DestroyPipelineLayout(layout);
		}

		for (auto& [name, sampler] : Samplers)
		{
			BmRender_DestroySampler(sampler);
		}

		BmRender_DestroyDescriptorPool(State.MainPool);

		DeInitDrawSystem();
		DeInitCommandSystem();
	}

	void Draw(DrawScene* Scene, u64 WaitSemaphoreValue)
	{
		VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();

		const u32 CurrentFrame = GetCurrentFrameIndex();

		RenderResources::UpdateBufferRegion(State.VpHandle[CurrentFrame], 0, &Scene->ViewProjection, sizeof(ViewProjectionBuffer));
		RenderResources::UpdateBufferRegion(State.EntityLightBufferHandle[CurrentFrame], 0, Scene->LightEntity, sizeof(LightBuffer));

		const u32 ImageIndex = AcquireNextSwapchainImage(CurrentFrame);
		CurrentImageIndex = ImageIndex;

		State.GraphicsCommandWorker = AcquireWorker(ULLONG_MAX);
		StartRecording(State.GraphicsCommandWorker);

		CommandWorkerData* SubmitPool = GetSubmitPoolData(State.GraphicsCommandWorker);
		VkCommandBuffer DrawCmdBuffer = (VkCommandBuffer)SubmitPool->CommandBuffer;

		VkDevice Device = GetCoreContext()->LogicalDevice;

		LightningPass::Draw(Scene);
		MainPass::BeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(Device, SubmitPool->CommandBuffer, &State.MeshPipeline, Scene, State.DescriptorSets);
		MainPass::EndPass();
		DeferredPass::BeginPass();
		DeferredPass::Draw();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), DrawCmdBuffer);
		DeferredPass::EndPass();

		EndRecording(State.GraphicsCommandWorker);

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};

		DrawSystemData* DrawSystem = GetDrawSystemData();

		BmRender_SubmitInfo SubmitInfo = { };
		SubmitInfo.WaitDstStageFlags = WaitStages;
		SubmitInfo.WaitSemaphores = &DrawSystem->ImagesAvailable[CurrentFrame];
		SubmitInfo.WaitSemaphoreCount = 1;
		SubmitInfo.WaitTimelineSemaphores = nullptr;
		SubmitInfo.WaitTimelineSemaphoreCount = 0;
		SubmitInfo.CommandBuffers = &SubmitPool->CommandBuffer;
		SubmitInfo.CommandBufferCount = 1;
		SubmitInfo.SignalSemaphores = &DrawSystem->RenderFinished[CurrentFrame];
		SubmitInfo.SignalSemaphoreCount = 1;
		SubmitInfo.SignalTimelineSemaphores = nullptr;
		SubmitInfo.SignalTimelineSemaphoreCount = 0;

		BmRender_PresentInfo PresentInfo = { };
		PresentInfo.WaitSemaphores = &DrawSystem->RenderFinished[CurrentFrame];
		PresentInfo.WaitSemaphoreCount = 1;
		PresentInfo.ImageIndices = &ImageIndex;

		std::unique_lock Lock(GetCommandSystemData()->QueueSubmitMutex);
		BmRender_QueueSubmit(GetCommandSystemData()->GraphicsQueue, 1, &SubmitInfo, SubmitPool->Fence);
		BmRender_QueuePresent(GetCommandSystemData()->GraphicsQueue, &PresentInfo);
		Lock.unlock();

		GetDrawSystemData()->CurrentFrame = Math::WrapIncrement(CurrentFrame, 3u);

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

	static BmRender_Image DeferredInputDepthImage[VulkanHelper::MAX_DRAW_FRAMES];
	static BmRender_Image DeferredInputColorImage[VulkanHelper::MAX_DRAW_FRAMES];
		
	static BmRender_ImageView DeferredInputDepthImageInterface[VulkanHelper::MAX_DRAW_FRAMES];
	static BmRender_ImageView DeferredInputColorImageInterface[VulkanHelper::MAX_DRAW_FRAMES];

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

		DeferredInputLayout = (VkDescriptorSetLayout)DescriptorSetLayouts["MainPassOutputLayout"];

		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			DeferredInputColorImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, ColorFormat, BmRender_ImageType::ColorAttachmentSampled);
			DeferredInputDepthImage[i] = BmRender_CreateImage2D(MainScreenExtent.width, MainScreenExtent.height, DepthFormat, BmRender_ImageType::DepthSamplad);
			
			DeferredInputColorImageInterface[i] = BmRender_CreateImageView2D(DeferredInputColorImage[i]);
			DeferredInputDepthImageInterface[i] = BmRender_CreateImageView2D(DeferredInputDepthImage[i]);
			
			BmRender_DescriptorSetBinding ColorBinding;
			ColorBinding.ImageBinding.Sampler = Samplers["ColorAttachment"];
			ColorBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColorBinding.ImageBinding.ImageView = DeferredInputColorImageInterface[i];
			ColorBinding.BindingCount = 1;
			ColorBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding DepthBinding;
			DepthBinding.ImageBinding.Sampler = Samplers["DepthAttachment"];
			DepthBinding.ImageBinding.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthBinding.ImageBinding.ImageView = DeferredInputDepthImageInterface[i];
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
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		VkCommandBuffer CmdBuffer = (VkCommandBuffer)SubmitPool->CommandBuffer;

		VkPipelineLayout PipelineLayout = (VkPipelineLayout)PipelineLayouts["Deferred"];

		BmRender_BindPipeline(SubmitPool->CommandBuffer, Pipelines["Deferred"]);

		DescriptorSetData SetData;
		GetDescriptorSetData(DeferredInputSet[GetDrawSystemData()->CurrentFrame], &SetData);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			0, 1, &SetData.Set, 0, nullptr);

		BmRender_Draw(SubmitPool->CommandBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void BeginPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		VkCommandBuffer CmdBuffer = (VkCommandBuffer)SubmitPool->CommandBuffer;

		BmRender_RenderingColorAttachment SwapchainColorAttachment = { };
		SwapchainColorAttachment.ImageView = GetCoreContext()->ImageViews[Render::CurrentImageIndex];
		SwapchainColorAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SwapchainColorAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		SwapchainColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingInfo RenderingInfo{ };
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = MainScreenExtent;
		RenderingInfo.ColorAttachments = &SwapchainColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = nullptr;

		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, DeferredInputColorImage[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, DeferredInputDepthImage[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, GetCoreContext()->Images[Render::CurrentImageIndex]);

		BmRender_BeginRendering(SubmitPool->CommandBuffer, &RenderingInfo);
	}

	void EndPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		BmRender_EndRendering(SubmitPool->CommandBuffer);

		BmRender_TransitionImageForPresentation(SubmitPool->CommandBuffer, GetCoreContext()->Images[Render::CurrentImageIndex]);
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

	void DeInit()
	{
		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			BmRender_DestroyImageView(DeferredInputColorImageInterface[i]);
			BmRender_DestroyImageView(DeferredInputDepthImageInterface[i]);
			BmRender_DestroyImage(DeferredInputColorImage[i]);
			BmRender_DestroyImage(DeferredInputDepthImage[i]);
		}
	}
}

namespace LightningPass
{
	static VkDescriptorSetLayout LightSpaceMatrixLayout;

	static BmRender_DescriptorSet LightSpaceMatrixSet[VulkanHelper::MAX_DRAW_FRAMES];

	static BmRender_GPUBufferBinding LightSpaceMatrixBufferRegion[VulkanHelper::MAX_DRAW_FRAMES];
	
	// Buffer handles array
	static BmRender_GPUBuffer LightSpaceMatrixBuffers[VulkanHelper::MAX_DRAW_FRAMES];

		static BmRender_ImageView ShadowMapElement1ImageInterface[VulkanHelper::MAX_DRAW_FRAMES];
		static BmRender_ImageView ShadowMapElement2ImageInterface[VulkanHelper::MAX_DRAW_FRAMES];

	static VkPushConstantRange PushConstants;

	void Init(BmRender_DescriptorPool MainPool)
	{
		VkDevice Device = GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

		LightSpaceMatrixLayout = (VkDescriptorSetLayout)DescriptorSetLayouts["LightSpaceMatrixLayout"];

		ShadowMapArray = BmRender_CreateImage2DArray(DepthViewportExtent.width, DepthViewportExtent.height, DepthFormat,
			BmRender_ImageType::DepthSamplad, MAX_LIGHT_SOURCES * GetCoreContext()->ImagesCount);

		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			LightSpaceMatrixBuffers[i] = BmRender_CreateUniformBuffer(LightSpaceMatrixSize, MemoryPropertyFlag::HostCompatible, BmRender_PipelineSyncStage::VertexShader);
			LightSpaceMatrixBufferRegion[i] = { LightSpaceMatrixBuffers[i], 0, LightSpaceMatrixSize };

			LightSpaceMatrixSet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["LightSpaceMatrixLayout"], MainPool);

			BmRender_DescriptorSetBinding LightSpaceMatrixBinding;
			LightSpaceMatrixBinding.BufferRegions = &LightSpaceMatrixBufferRegion[i];
			LightSpaceMatrixBinding.BindingCount = 1;
			LightSpaceMatrixBinding.DstArrayElement = 0;

			BmRender_UpdateDescriptorSet(LightSpaceMatrixSet[i], &LightSpaceMatrixBinding, 1);

			ShadowMapElement1ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i, 1);
			ShadowMapElement2ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i + 1, 1);
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
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		VkCommandBuffer CmdBuffer = (VkCommandBuffer)SubmitPool->CommandBuffer;
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, ShadowMapArray, MAX_LIGHT_SOURCES * GetDrawSystemData()->CurrentFrame, MAX_LIGHT_SOURCES);

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			RenderResources::UpdateBufferRegion(LightSpaceMatrixBufferRegion[LightCaster], 0, LightViews[LightCaster], sizeof(glm::mat4));

			BmRender_ImageView DepthImageView = (LightCaster == 0) ? 
				ShadowMapElement1ImageInterface[GetDrawSystemData()->CurrentFrame] : 
				ShadowMapElement2ImageInterface[GetDrawSystemData()->CurrentFrame];

			BmRender_RenderingDepthAttachment DepthAttachment{ };
			DepthAttachment.ImageView = DepthImageView;
			DepthAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			DepthAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			DepthAttachment.ClearValue = { 1.0f, 0 };

			BmRender_RenderingInfo RenderingInfo{ };
			RenderingInfo.Offset = { 0, 0 };
			RenderingInfo.Extent = DepthViewportExtent;
			RenderingInfo.ColorAttachments = nullptr;
			RenderingInfo.ColorAttachmentCount = 0;
			RenderingInfo.DepthAttachment = &DepthAttachment;

			BmRender_BeginRendering(SubmitPool->CommandBuffer, &RenderingInfo);

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
			Config.PushConstant = {};
			Config.PushConstantData = nullptr;

			Render::DrawEntityBatch(SubmitPool->CommandBuffer, Scene, Config);

			BmRender_EndRendering(SubmitPool->CommandBuffer);
		}

		// TODO: move to Main pass?
		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, ShadowMapArray, MAX_LIGHT_SOURCES * GetDrawSystemData()->CurrentFrame, MAX_LIGHT_SOURCES);
	}

	void DeInit()
	{
		for (u32 i = 0; i < GetCoreContext()->ImagesCount; i++)
		{
			BmRender_DestroyGPUBuffer(LightSpaceMatrixBuffers[i]);
			BmRender_DestroyImageView(ShadowMapElement1ImageInterface[i]);
			BmRender_DestroyImageView(ShadowMapElement2ImageInterface[i]);
		}
		BmRender_DestroyImage(ShadowMapArray);
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
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		VkCommandBuffer CmdBuffer = (VkCommandBuffer)SubmitPool->CommandBuffer;

		BmRender_RenderingColorAttachment ColorAttachment = { };
		ColorAttachment.ImageView = DeferredPass::TestDeferredInputColorImageInterface()[GetDrawSystemData()->CurrentFrame];
		ColorAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingDepthAttachment DepthAttachment = { };
		DepthAttachment.ImageView = DeferredPass::TestDeferredInputDepthImageInterface()[GetDrawSystemData()->CurrentFrame];
		DepthAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.ClearValue = { 1.0f, 0 };

		BmRender_RenderingInfo RenderingInfo = { };
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = MainScreenExtent;
		RenderingInfo.ColorAttachments = &ColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = &DepthAttachment;

		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, DeferredPass::TestDeferredInputColorImage()[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, DeferredPass::TestDeferredInputDepthImage()[GetDrawSystemData()->CurrentFrame]);

		BmRender_BeginRendering(SubmitPool->CommandBuffer, &RenderingInfo);
	}

	void EndPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(Render::GetRenderState()->GraphicsCommandWorker);
		BmRender_EndRendering(SubmitPool->CommandBuffer);
	}

	AttachmentData* GetAttachmentData()
	{
		return &PipelineAttachmentData;
	}
}
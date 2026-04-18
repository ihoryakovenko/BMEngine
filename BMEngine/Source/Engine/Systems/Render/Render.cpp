#include "Render.h"

#include "RenderResources.h"
#include "TransferSystem.h"
#include "Systems.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/Settings.h"
#include "Util/Math.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include <RenderHelper.h>

#include <random>
#include <mutex>

#include <SharedLib.h>

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
	static void InitImGuiPipeline(BmRender_DescriptorPool* ImGuiPool, GLFWwindow* Wnd)
	{
		ImGui_ImplGlfw_InitForVulkan(Wnd, true);

		AttachmentData* AttachmentDataPtr = DeferredPassGetAttachmentData();
		VkFormat* ColorAttachmentFormats = (VkFormat*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), AttachmentDataPtr->ColorAttachmentCount * sizeof(VkFormat));
		for (u32 i = 0; i < AttachmentDataPtr->ColorAttachmentCount; ++i)
		{
			BmRender_ImageViewData ImageViewData;
			BmRender_ImageResource ImageResource;
			if (AttachmentDataPtr->ColorAttachments[i] != nullptr && 
				BmRender_GetImageViewData(AttachmentDataPtr->ColorAttachments[i], &ImageViewData) &&
				BmRender_GetImageData(ImageViewData.Image, &ImageResource))
			{
				ColorAttachmentFormats[i] = BmRender_FormatToVk(ImageResource.Format);
			}
			else
			{
				ColorAttachmentFormats[i] = VK_FORMAT_UNDEFINED;
			}
		}

		VkFormat DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		if (AttachmentDataPtr->DepthAttachment != nullptr)
		{
			BmRender_ImageViewData ImageViewData;
			BmRender_ImageResource ImageResource;
			if (BmRender_GetImageViewData(AttachmentDataPtr->DepthAttachment, &ImageViewData) &&
				BmRender_GetImageData(ImageViewData.Image, &ImageResource))
			{
				DepthAttachmentFormat = BmRender_FormatToVk(ImageResource.Format);
			}
		}

		VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
		if (AttachmentDataPtr->StencilAttachment != nullptr)
		{
			BmRender_ImageViewData ImageViewData;
			BmRender_ImageResource ImageResource;
			if (BmRender_GetImageViewData(AttachmentDataPtr->StencilAttachment, &ImageViewData) &&
				BmRender_GetImageData(ImageViewData.Image, &ImageResource))
			{
				StencilAttachmentFormat = BmRender_FormatToVk(ImageResource.Format);
			}
		}

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = AttachmentDataPtr->ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = StencilAttachmentFormat;

		BmRender_DescriptorPoolSize PoolSizes[] =
		{
			{ BmRender_DescriptorType::CombinedImageSampler, 1 },
		};

		*ImGuiPool = BmRender_CreateDescriptorPool(PoolSizes, 1, (u32)IM_ARRAYSIZE(PoolSizes), BmRender_DescriptorPoolType::CreateFree);

		BmRender_Queue GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);
		ImGui_ImplVulkan_InitInfo InitInfo = { };
		InitInfo.Instance = (VkInstance)BmRender_GetVulkanInstance();
		InitInfo.PhysicalDevice = (VkPhysicalDevice)BmRender_GetPhysicalDevice();
		InitInfo.Device = (VkDevice)BmRender_GetLogicalDevice();
		InitInfo.QueueFamily = BmRender_GetQueueFamily(GraphicsQueue);
		InitInfo.Queue = (VkQueue)GraphicsQueue;
		InitInfo.PipelineCache = nullptr;
		InitInfo.DescriptorPool = *((VkDescriptorPool*)ImGuiPool);
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

	static void DeInitImGuiPipeline(BmRender_DescriptorPool ImGuiPool)
	{
		ImGui_ImplVulkan_Shutdown();
		BmRender_DestroyDescriptorPool(ImGuiPool);
	}

	static void InitStaticMeshPipeline(StaticMeshPipeline* MeshPipeline, BmRender_DescriptorPool MainPool)
	{
		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_LIGHT_SOURCES * i, MAX_LIGHT_SOURCES);
			
			BmRender_DescriptorSetBinding ShadowMapBinding;
			ShadowMapBinding.ImageBinding.Sampler = Samplers["ShadowMap"];
			ShadowMapBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			ShadowMapBinding.ImageBinding.ImageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapBinding.BindingCount = 1;
			ShadowMapBinding.DstArrayElement = 0;

			MeshPipeline->ShadowMapArraySet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["ShadowMapArrayLayout"], MainPool);
			BmRender_UpdateDescriptorSet(MeshPipeline->ShadowMapArraySet[i], &ShadowMapBinding, 1);
		}

		AttachmentData ResourceInfo = *MainPassGetAttachmentData();
		BmRender_ImageView ResourceInfoColorAttachments[16];
		ResourceInfo.ColorAttachments = ResourceInfoColorAttachments;
		for (u32 i = 0; i < ResourceInfo.ColorAttachmentCount; ++i)
		{
			ResourceInfoColorAttachments[i] = MainPassGetAttachmentData()->ColorAttachments[i];
		}

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		// Build shader stages
		BmRender_ShaderStageDescription vertexStage = {};
		vertexStage.Shader = Shaders["StaticMeshVertex"];
		vertexStage.EntryPointFunction = "main";
		shaderStages.push_back(vertexStage);

		BmRender_ShaderStageDescription fragmentStage = {};
		fragmentStage.Shader = Shaders["StaticMeshFragment"];
		fragmentStage.EntryPointFunction = "main";
		shaderStages.push_back(fragmentStage);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(DescriptorSetLayouts["FrameDataLayout"]);
		descriptorSetLayouts.push_back(DescriptorSetLayouts["BindlesTexturesLayout"]);
		descriptorSetLayouts.push_back(DescriptorSetLayouts["FrameDataLayout"]);
		descriptorSetLayouts.push_back(DescriptorSetLayouts["MaterialLayout"]);
		descriptorSetLayouts.push_back(DescriptorSetLayouts["ShadowMapArrayLayout"]);

		// Build push constants
		pushConstantRanges.push_back(PushConstants["MainConstant"]);

		// Build vertex bindings from VBindings map
		{
			// StaticMeshVertex binding
			Util::VertexBinding_depr& StaticMeshVertexBinding = VBindings["StaticMeshVertex"];
			BmRender_VertexBinding BmRenderVertexBinding = {};
			BmRenderVertexBinding.Stride = StaticMeshVertexBinding.Stride;
			BmRenderVertexBinding.InputRate = StaticMeshVertexBinding.InputRate;
			BmRenderVertexBinding.AttributesCount = 3; // Position, TextureCoords, Normal
			BmRenderVertexBinding.Attributes = (VertexAttribute*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);
			BmRenderVertexBinding.Attributes[0] = StaticMeshVertexBinding.Attributes["Position"];
			BmRenderVertexBinding.Attributes[1] = StaticMeshVertexBinding.Attributes["TextureCoords"];
			BmRenderVertexBinding.Attributes[2] = StaticMeshVertexBinding.Attributes["Normal"];
			vertexBindings.push_back(BmRenderVertexBinding);
		}

		{
			// StaticMeshInstance binding
			Util::VertexBinding_depr& StaticMeshInstanceBinding = VBindings["StaticMeshInstance"];
			BmRender_VertexBinding BmRenderVertexBinding = {};
			BmRenderVertexBinding.Stride = StaticMeshInstanceBinding.Stride;
			BmRenderVertexBinding.InputRate = StaticMeshInstanceBinding.InputRate;
			BmRenderVertexBinding.AttributesCount = 2; // InstanceModel, InstanceMaterialIndex
			BmRenderVertexBinding.Attributes = (VertexAttribute*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);
			BmRenderVertexBinding.Attributes[0] = StaticMeshInstanceBinding.Attributes["InstanceModel"];
			BmRenderVertexBinding.Attributes[1] = StaticMeshInstanceBinding.Attributes["InstanceMaterialIndex"];
			vertexBindings.push_back(BmRenderVertexBinding);
		}

		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = MainScreenExtent;
		PipelineDesc.Attachment = ResourceInfo;

		// Set shader stages
		PipelineDesc.ShaderStages = shaderStages.data();
		PipelineDesc.ShaderStagesCount = static_cast<u32>(shaderStages.size());

		// Set vertex bindings
		PipelineDesc.VertexBindings = vertexBindings.data();
		PipelineDesc.VertexBindingsCount = static_cast<u32>(vertexBindings.size());

		// Set descriptor set layouts
		PipelineDesc.DescriptorSetLayouts = descriptorSetLayouts.data();
		PipelineDesc.DescriptorSetLayoutsCount = static_cast<u32>(descriptorSetLayouts.size());

		// Set push constants
		PipelineDesc.PushConstantRanges = pushConstantRanges.data();
		PipelineDesc.PushConstantRangesCount = static_cast<u32>(pushConstantRanges.size());

		// Rasterization state
		PipelineDesc.RasterizationState = {};
		PipelineDesc.RasterizationState.DepthClampEnable = false;
		PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
		PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
		PipelineDesc.RasterizationState.LineWidth = 1.0f;
		PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::Back;
		PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
		PipelineDesc.RasterizationState.DepthBiasEnable = false;

		// Color blend state
		PipelineDesc.ColorBlendState = {};
		PipelineDesc.ColorBlendState.LogicOpEnable = false;
		PipelineDesc.ColorBlendState.AttachmentCount = 1;

		// Color blend attachment
		PipelineDesc.ColorBlendAttachment = {};
		PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
		PipelineDesc.ColorBlendAttachment.BlendEnable = true;
		PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
		PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
		PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
		PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
		PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
		PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

		// Depth stencil state
		PipelineDesc.DepthStencilState = {};
		PipelineDesc.DepthStencilState.DepthTestEnable = true;
		PipelineDesc.DepthStencilState.DepthWriteEnable = true;
		PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
		PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
		PipelineDesc.DepthStencilState.StencilTestEnable = false;

		// Multisample state
		PipelineDesc.MultisampleState = {};
		PipelineDesc.MultisampleState.SampleShadingEnable = false;

		// Input assembly state
		PipelineDesc.InputAssemblyState = {};
		PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
		PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

		// Viewport state
		PipelineDesc.ViewportState = {};
		PipelineDesc.ViewportState.ViewportCount = 1;
		PipelineDesc.ViewportState.ScissorCount = 1;

		// Viewport
		PipelineDesc.Viewport = {};
		PipelineDesc.Viewport.MinDepth = 0.0f;
		PipelineDesc.Viewport.MaxDepth = 1.0f;
		PipelineDesc.Viewport.X = 0.0f;
		PipelineDesc.Viewport.Y = 0.0f;
		PipelineDesc.Viewport.Width = static_cast<f32>(MainScreenExtent.Width);
		PipelineDesc.Viewport.Height = static_cast<f32>(MainScreenExtent.Height);

		// Scissor
		PipelineDesc.Scissor = {};
		PipelineDesc.Scissor.Offset.X = 0;
		PipelineDesc.Scissor.Offset.Y = 0;
		PipelineDesc.Scissor.Extent.Width = MainScreenExtent.Width;
		PipelineDesc.Scissor.Extent.Height = MainScreenExtent.Height;

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;
		LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;

		PipelineLayouts["StaticMesh"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["StaticMesh"];

		Pipelines["StaticMesh"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	static void DrawStaticMeshes(BmRender_CommandBuffer CommandBuffer, StaticMeshPipeline* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets)
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

	static RenderState State;
	static u32 CurrentImageIndex;

	// DeferredPass static variables
	static BmRender_Image DeferredInputDepthImage[MAX_DRAW_FRAMES];
	static BmRender_Image DeferredInputColorImage[MAX_DRAW_FRAMES];

	static BmRender_ImageView DeferredInputDepthImageInterface[MAX_DRAW_FRAMES];
	static BmRender_ImageView DeferredInputColorImageInterface[MAX_DRAW_FRAMES];

	static BmRender_DescriptorSet DeferredInputSet[MAX_DRAW_FRAMES];

	static AttachmentData DeferredPassPipelineAttachmentData;
	static BmRender_ImageView DeferredPassColorAttachments[16];

	void DrawEntityBatch(BmRender_CommandBuffer CommandBuffer, DrawScene* Scene, const DrawEntityBatchConfig& Config)
	{
		BmRender_BindPipeline(CommandBuffer, Config.Pipeline);

		if (Config.PushConstant.Offset != 0 || Config.PushConstant.Size != 0)
		{
			BmRender_RecordPushConstants(CommandBuffer, Config.PipelineLayout, Config.PushConstant.StageFlags,
				Config.PushConstant.Offset, Config.PushConstant.Size, Config.PushConstantData);
		}

		if (Config.DescriptorSetCount > 0)
		{
			BmRender_RecordBindDescriptorSets(CommandBuffer, Config.PipelineLayout,
				0, Config.DescriptorSetCount, Config.DescriptorSets, Config.DynamicOffsetCount, Config.DynamicOffsets);
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

			const BmRender_GPUBuffer Buffers[] = {
				Entity->VertexBufferEntry.GPUBufferHandle,
				Entity->InstanceBufferEntry.GPUBufferHandle
			};
	
			const u64 Offsets[] = {
				Entity->VertexBufferEntry.BufferOffset,
				Entity->InstanceBufferEntry.BufferOffset
			};
	
			BmRender_RecordBindVertexBuffers(CommandBuffer, 0, 2, Buffers, Offsets);
			BmRender_RecordBindIndexBuffer(CommandBuffer, Entity->IndexBufferEntry.GPUBufferHandle, Entity->IndexBufferEntry.BufferOffset, BmRender_IndexType::Uint32);
			BmRender_DrawIndexed(CommandBuffer, Entity->IndicesCount, Entity->Instances, 0, 0, 0);
		}
	}

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferBinding* VpRegion, BmRender_GPUBufferBinding* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool)
	{		
		InitCommandSystem(3);
		InitDrawSystem(3);

		State.VpHandle = VpRegion;
		State.EntityLightBufferHandle = EntityLightRegion;
		State.DescriptorSets = DescriptorSets;
		State.MainPool = MainPool;

		DeferredPassInit(State.MainPool);
		MainPassInit();
		LightningPassInit(State.MainPool);

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(&State.MeshPipeline, State.MainPool);
		InitImGuiPipeline(&State.DebugUiPool, WindowHandler);
	}

	void DeInit()
	{
		BmRender_DeviceWaitIdle();

		DeInitImGuiPipeline(State.DebugUiPool);
		
		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			BmRender_DestroyImageView(State.MeshPipeline.ShadowMapArrayImageInterface[i]);
		}

		DeferredPassDeInit();
		LightningPassDeInit();

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
		const u32 CurrentFrame = GetCurrentFrameIndex();

		RenderResources::UpdateBufferRegion(State.VpHandle[CurrentFrame], 0, &Scene->ViewProjection, sizeof(ViewProjectionBuffer));
		RenderResources::UpdateBufferRegion(State.EntityLightBufferHandle[CurrentFrame], 0, Scene->LightEntity, sizeof(LightBuffer));

		const u32 ImageIndex = AcquireNextSwapchainImage(CurrentFrame);
		CurrentImageIndex = ImageIndex;

		State.GraphicsCommandWorker = AcquireWorker(ULLONG_MAX);
		StartRecording(State.GraphicsCommandWorker);

		CommandWorkerData* SubmitPool = GetSubmitPoolData(State.GraphicsCommandWorker);

		LightningPassDraw(Scene);
		MainPassBeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(SubmitPool->CommandBuffer, &State.MeshPipeline, Scene, State.DescriptorSets);
		MainPassEndPass();
		DeferredPassBeginPass();
		DeferredPassDraw();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)SubmitPool->CommandBuffer);
		DeferredPassEndPass();

		EndRecording(State.GraphicsCommandWorker);

		BmRender_PipelineSyncStage WaitStages[] = {
			BmRender_PipelineSyncStage::ColorAttachmentOutput,
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

		BmRender_FrameFree();
	}

	RenderState* GetRenderState()
	{
		return &State;
	}

	void DeferredPassInit(BmRender_DescriptorPool MainPool)
	{
		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			DeferredInputColorImage[i] = BmRender_CreateImage2D(MainScreenExtent.Width, MainScreenExtent.Height, ColorFormat, BmRender_ImageType::ColorAttachmentSampled, BmRender_SampleCount::Count1);
			DeferredInputDepthImage[i] = BmRender_CreateImage2D(MainScreenExtent.Width, MainScreenExtent.Height, DepthFormat, BmRender_ImageType::DepthSamplad, BmRender_SampleCount::Count1);
			
			DeferredInputColorImageInterface[i] = BmRender_CreateImageView2D(DeferredInputColorImage[i]);
			DeferredInputDepthImageInterface[i] = BmRender_CreateImageView2D(DeferredInputDepthImage[i]);
		}

		DeferredPassPipelineAttachmentData.ColorAttachmentCount = 1;
		DeferredPassPipelineAttachmentData.ColorAttachments = DeferredPassColorAttachments;
		DeferredPassColorAttachments[0] = DeferredInputColorImageInterface[0];
		DeferredPassPipelineAttachmentData.DepthAttachment = DeferredInputDepthImageInterface[0];
		DeferredPassPipelineAttachmentData.StencilAttachment = nullptr;

		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			BmRender_DescriptorSetBinding ColorBinding;
			ColorBinding.ImageBinding.Sampler = Samplers["ColorAttachment"];
			ColorBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			ColorBinding.ImageBinding.ImageView = DeferredInputColorImageInterface[i];
			ColorBinding.BindingCount = 1;
			ColorBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding DepthBinding;
			DepthBinding.ImageBinding.Sampler = Samplers["DepthAttachment"];
			DepthBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			DepthBinding.ImageBinding.ImageView = DeferredInputDepthImageInterface[i];
			DepthBinding.BindingCount = 1;
			DepthBinding.DstArrayElement = 0;

			BmRender_DescriptorSetBinding Bindings[] = { ColorBinding, DepthBinding };

			DeferredInputSet[i] = BmRender_CreateDescriptorSet(DescriptorSetLayouts["MainPassOutputLayout"], MainPool);
			BmRender_UpdateDescriptorSet(DeferredInputSet[i], Bindings, 2);
		}

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		// Build shader stages
		BmRender_ShaderStageDescription vertexStage = {};
		vertexStage.Shader = Shaders["DeferredVertex"];
		vertexStage.EntryPointFunction = "main";
		shaderStages.push_back(vertexStage);

		BmRender_ShaderStageDescription fragmentStage = {};
		fragmentStage.Shader = Shaders["DeferredFragment"];
		fragmentStage.EntryPointFunction = "main";
		shaderStages.push_back(fragmentStage);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(DescriptorSetLayouts["MainPassOutputLayout"]);

		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = MainScreenExtent;
		PipelineDesc.Attachment = DeferredPassPipelineAttachmentData;

		// Set shader stages
		PipelineDesc.ShaderStages = shaderStages.data();
		PipelineDesc.ShaderStagesCount = static_cast<u32>(shaderStages.size());

		// Set vertex bindings (empty for deferred pass - fullscreen quad)
		PipelineDesc.VertexBindings = nullptr;
		PipelineDesc.VertexBindingsCount = 0;

		// Set descriptor set layouts
		PipelineDesc.DescriptorSetLayouts = descriptorSetLayouts.data();
		PipelineDesc.DescriptorSetLayoutsCount = static_cast<u32>(descriptorSetLayouts.size());

		// Set push constants (none)
		PipelineDesc.PushConstantRanges = nullptr;
		PipelineDesc.PushConstantRangesCount = 0;

		// Rasterization state
		PipelineDesc.RasterizationState = {};
		PipelineDesc.RasterizationState.DepthClampEnable = false;
		PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
		PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
		PipelineDesc.RasterizationState.LineWidth = 1.0f;
		PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::None;
		PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
		PipelineDesc.RasterizationState.DepthBiasEnable = false;

		// Color blend state
		PipelineDesc.ColorBlendState = {};
		PipelineDesc.ColorBlendState.LogicOpEnable = false;
		PipelineDesc.ColorBlendState.AttachmentCount = 1;

		// Color blend attachment
		PipelineDesc.ColorBlendAttachment = {};
		PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
		PipelineDesc.ColorBlendAttachment.BlendEnable = false;
		PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
		PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
		PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
		PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
		PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
		PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

		// Depth stencil state
		PipelineDesc.DepthStencilState = {};
		PipelineDesc.DepthStencilState.DepthTestEnable = false;
		PipelineDesc.DepthStencilState.DepthWriteEnable = false;
		PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
		PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
		PipelineDesc.DepthStencilState.StencilTestEnable = false;

		// Multisample state
		PipelineDesc.MultisampleState = {};
		PipelineDesc.MultisampleState.SampleShadingEnable = false;

		// Input assembly state
		PipelineDesc.InputAssemblyState = {};
		PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
		PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

		// Viewport state
		PipelineDesc.ViewportState = {};
		PipelineDesc.ViewportState.ViewportCount = 1;
		PipelineDesc.ViewportState.ScissorCount = 1;

		// Viewport
		PipelineDesc.Viewport = {};
		PipelineDesc.Viewport.MinDepth = 0.0f;
		PipelineDesc.Viewport.MaxDepth = 1.0f;
		PipelineDesc.Viewport.X = 0.0f;
		PipelineDesc.Viewport.Y = 0.0f;
		PipelineDesc.Viewport.Width = static_cast<f32>(MainScreenExtent.Width);
		PipelineDesc.Viewport.Height = static_cast<f32>(MainScreenExtent.Height);

		// Scissor
		PipelineDesc.Scissor = {};
		PipelineDesc.Scissor.Offset.X = 0;
		PipelineDesc.Scissor.Offset.Y = 0;
		PipelineDesc.Scissor.Extent.Width = MainScreenExtent.Width;
		PipelineDesc.Scissor.Extent.Height = MainScreenExtent.Height;

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;
		LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;

		PipelineLayouts["Deferred"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["Deferred"];

		Pipelines["Deferred"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	void DeferredPassDraw()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);
		BmRender_BindPipeline(SubmitPool->CommandBuffer, Pipelines["Deferred"]);

		const BmRender_DescriptorSet DescriptorSet = DeferredInputSet[GetDrawSystemData()->CurrentFrame];
		BmRender_RecordBindDescriptorSets(SubmitPool->CommandBuffer, PipelineLayouts["Deferred"],
			0, 1, &DescriptorSet, 0, nullptr);

		BmRender_Draw(SubmitPool->CommandBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void DeferredPassBeginPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);

		BmRender_RenderingColorAttachment SwapchainColorAttachment = { };
		SwapchainColorAttachment.ImageView = BmRender_GetSwapchainImageView(CurrentImageIndex);
		SwapchainColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		SwapchainColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		SwapchainColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingInfo RenderingInfo{ };
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = MainScreenExtent;
		RenderingInfo.ColorAttachments = &SwapchainColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = nullptr;

		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, DeferredInputColorImage[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, DeferredInputDepthImage[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, BmRender_GetSwapchainImage(CurrentImageIndex));

		BmRender_BeginRendering(SubmitPool->CommandBuffer, &RenderingInfo);
	}

	void DeferredPassEndPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);
		BmRender_EndRendering(SubmitPool->CommandBuffer);

		BmRender_TransitionImageForPresentation(SubmitPool->CommandBuffer, BmRender_GetSwapchainImage(CurrentImageIndex));
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

	AttachmentData* DeferredPassGetAttachmentData()
	{
		return &DeferredPassPipelineAttachmentData;
	}

	void DeferredPassDeInit()
	{
		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			BmRender_DestroyImageView(DeferredInputColorImageInterface[i]);
			BmRender_DestroyImageView(DeferredInputDepthImageInterface[i]);
			BmRender_DestroyImage(DeferredInputColorImage[i]);
			BmRender_DestroyImage(DeferredInputDepthImage[i]);
		}
	}

	// LightningPass static variables
	static BmRender_DescriptorSet LightSpaceMatrixSet[MAX_DRAW_FRAMES];

	static BmRender_GPUBufferBinding LightSpaceMatrixBufferRegion[MAX_DRAW_FRAMES];
	
	// Buffer handles array
	static BmRender_GPUBuffer LightSpaceMatrixBuffers[MAX_DRAW_FRAMES];

	static BmRender_ImageView ShadowMapElement1ImageInterface[MAX_DRAW_FRAMES];
	static BmRender_ImageView ShadowMapElement2ImageInterface[MAX_DRAW_FRAMES];

	void LightningPassInit(BmRender_DescriptorPool MainPool)
	{
		ShadowMapArray = BmRender_CreateImage2DArray(DepthViewportExtent.Width, DepthViewportExtent.Height, DepthFormat,
			BmRender_ImageType::DepthSamplad, MAX_LIGHT_SOURCES * BmRender_GetSwapchainImageCount(), BmRender_SampleCount::Count1);

		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			const u64 LightSpaceMatrixSize = sizeof(glm::mat4);

			LightSpaceMatrixBuffers[i] = BmRender_CreateUniformBuffer(LightSpaceMatrixSize, MemoryPropertyFlag::HostCompatible);
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

		AttachmentData ResourceInfo;
		ResourceInfo.ColorAttachmentCount = 0;
		ResourceInfo.ColorAttachments = nullptr;
		ResourceInfo.DepthAttachment = ShadowMapElement1ImageInterface[0];
		ResourceInfo.StencilAttachment = nullptr;

		// Create vectors to hold pipeline data
		std::vector<BmRender_ShaderStageDescription> shaderStages;
		std::vector<BmRender_VertexBinding> vertexBindings;
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		// Build shader stages
		BmRender_ShaderStageDescription vertexStage = {};
		vertexStage.Shader = Shaders["DepthVertex"];
		vertexStage.EntryPointFunction = "main";
		shaderStages.push_back(vertexStage);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(DescriptorSetLayouts["LightSpaceMatrixLayout"]);

		// Build vertex bindings from VBindings map
		{
			// StaticMeshVertex binding (only Position attribute)
			Util::VertexBinding_depr& StaticMeshVertexBinding = VBindings["StaticMeshVertex"];
			BmRender_VertexBinding BmRenderVertexBinding = {};
			BmRenderVertexBinding.Stride = StaticMeshVertexBinding.Stride;
			BmRenderVertexBinding.InputRate = StaticMeshVertexBinding.InputRate;
			BmRenderVertexBinding.AttributesCount = 1; // Position only
			BmRenderVertexBinding.Attributes = (VertexAttribute*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);
			BmRenderVertexBinding.Attributes[0] = StaticMeshVertexBinding.Attributes["Position"];
			vertexBindings.push_back(BmRenderVertexBinding);
		}

		{
			// StaticMeshInstance binding (only InstanceModel attribute)
			Util::VertexBinding_depr& StaticMeshInstanceBinding = VBindings["StaticMeshInstance"];
			BmRender_VertexBinding BmRenderVertexBinding = {};
			BmRenderVertexBinding.Stride = StaticMeshInstanceBinding.Stride;
			BmRenderVertexBinding.InputRate = StaticMeshInstanceBinding.InputRate;
			BmRenderVertexBinding.AttributesCount = 1; // InstanceModel only
			BmRenderVertexBinding.Attributes = (VertexAttribute*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);
			BmRenderVertexBinding.Attributes[0] = StaticMeshInstanceBinding.Attributes["InstanceModel"];
			vertexBindings.push_back(BmRenderVertexBinding);
		}

		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = DepthViewportExtent;
		PipelineDesc.Attachment = ResourceInfo;

		// Set shader stages
		PipelineDesc.ShaderStages = shaderStages.data();
		PipelineDesc.ShaderStagesCount = static_cast<u32>(shaderStages.size());

		// Set vertex bindings
		PipelineDesc.VertexBindings = vertexBindings.data();
		PipelineDesc.VertexBindingsCount = static_cast<u32>(vertexBindings.size());

		// Set descriptor set layouts
		PipelineDesc.DescriptorSetLayouts = descriptorSetLayouts.data();
		PipelineDesc.DescriptorSetLayoutsCount = static_cast<u32>(descriptorSetLayouts.size());

		// Set push constants (none)
		PipelineDesc.PushConstantRanges = nullptr;
		PipelineDesc.PushConstantRangesCount = 0;

		// Rasterization state
		PipelineDesc.RasterizationState = {};
		PipelineDesc.RasterizationState.DepthClampEnable = false;
		PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
		PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
		PipelineDesc.RasterizationState.LineWidth = 1.0f;
		PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::Back;
		PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
		PipelineDesc.RasterizationState.DepthBiasEnable = false;

		// Color blend state
		PipelineDesc.ColorBlendState = {};
		PipelineDesc.ColorBlendState.LogicOpEnable = false;
		PipelineDesc.ColorBlendState.AttachmentCount = 1;

		// Color blend attachment
		PipelineDesc.ColorBlendAttachment = {};
		PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
		PipelineDesc.ColorBlendAttachment.BlendEnable = true;
		PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
		PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
		PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
		PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
		PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
		PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

		// Depth stencil state
		PipelineDesc.DepthStencilState = {};
		PipelineDesc.DepthStencilState.DepthTestEnable = true;
		PipelineDesc.DepthStencilState.DepthWriteEnable = true;
		PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
		PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
		PipelineDesc.DepthStencilState.StencilTestEnable = false;

		// Multisample state
		PipelineDesc.MultisampleState = {};
		PipelineDesc.MultisampleState.SampleShadingEnable = false;

		// Input assembly state
		PipelineDesc.InputAssemblyState = {};
		PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
		PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

		// Viewport state
		PipelineDesc.ViewportState = {};
		PipelineDesc.ViewportState.ViewportCount = 1;
		PipelineDesc.ViewportState.ScissorCount = 1;

		// Viewport
		PipelineDesc.Viewport = {};
		PipelineDesc.Viewport.MinDepth = 0.0f;
		PipelineDesc.Viewport.MaxDepth = 1.0f;
		PipelineDesc.Viewport.X = 0.0f;
		PipelineDesc.Viewport.Y = 0.0f;
		PipelineDesc.Viewport.Width = static_cast<f32>(DepthViewportExtent.Width);
		PipelineDesc.Viewport.Height = static_cast<f32>(DepthViewportExtent.Height);

		// Scissor
		PipelineDesc.Scissor = {};
		PipelineDesc.Scissor.Offset.X = 0;
		PipelineDesc.Scissor.Offset.Y = 0;
		PipelineDesc.Scissor.Extent.Width = DepthViewportExtent.Width;
		PipelineDesc.Scissor.Extent.Height = DepthViewportExtent.Height;

		// Create pipeline layout from parsed descriptor set layouts
		BmRender_PipelineLayoutDescription LayoutDesc = {};
		LayoutDesc.SetLayoutCount = PipelineDesc.DescriptorSetLayoutsCount;
		LayoutDesc.SetLayouts = PipelineDesc.DescriptorSetLayouts;
		LayoutDesc.PushConstantRangeCount = PipelineDesc.PushConstantRangesCount;
		LayoutDesc.PushConstantRanges = PipelineDesc.PushConstantRanges;
		LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;

		PipelineLayouts["Depth"] = BmRender_CreatePipelineLayout(&LayoutDesc);
		PipelineDesc.PipelineLayout = PipelineLayouts["Depth"];

		Pipelines["Depth"] = BmRender_CreatePipeline(&PipelineDesc);
	}

	void LightningPassDraw(DrawScene* Scene)
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);

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
			DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
			DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
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

			DrawEntityBatchConfig Config = {};
			Config.Pipeline = Pipelines["Depth"];
			Config.PipelineLayout = PipelineLayouts["Depth"];
			Config.DescriptorSets = DescriptorSetGroup;
			Config.DescriptorSetCount = 1;
			Config.DynamicOffsetCount = 0;
			Config.DynamicOffsets = nullptr;
			Config.PushConstant = {};
			Config.PushConstantData = nullptr;

			DrawEntityBatch(SubmitPool->CommandBuffer, Scene, Config);

			BmRender_EndRendering(SubmitPool->CommandBuffer);
		}

		// TODO: move to Main pass?
		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, ShadowMapArray, MAX_LIGHT_SOURCES * GetDrawSystemData()->CurrentFrame, MAX_LIGHT_SOURCES);
	}

	void LightningPassDeInit()
	{
		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			BmRender_DestroyGPUBuffer(LightSpaceMatrixBuffers[i]);
			BmRender_DestroyImageView(ShadowMapElement1ImageInterface[i]);
			BmRender_DestroyImageView(ShadowMapElement2ImageInterface[i]);
		}
		BmRender_DestroyImage(ShadowMapArray);
	}

	// MainPass static variables
	static AttachmentData MainPassPipelineAttachmentData;
	static BmRender_ImageView MainPassColorAttachments[16];

	void MainPassInit()
	{
		MainPassPipelineAttachmentData.ColorAttachmentCount = 1;
		MainPassPipelineAttachmentData.ColorAttachments = MainPassColorAttachments;
		MainPassColorAttachments[0] = TestDeferredInputColorImageInterface()[0];
		MainPassPipelineAttachmentData.DepthAttachment = TestDeferredInputDepthImageInterface()[0];
		MainPassPipelineAttachmentData.StencilAttachment = nullptr;
	}

	void MainPassBeginPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);

		BmRender_RenderingColorAttachment ColorAttachment = { };
		ColorAttachment.ImageView = TestDeferredInputColorImageInterface()[GetDrawSystemData()->CurrentFrame];
		ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingDepthAttachment DepthAttachment = { };
		DepthAttachment.ImageView = TestDeferredInputDepthImageInterface()[GetDrawSystemData()->CurrentFrame];
		DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		DepthAttachment.ClearValue = { 1.0f, 0 };

		BmRender_RenderingInfo RenderingInfo = { };
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = MainScreenExtent;
		RenderingInfo.ColorAttachments = &ColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = &DepthAttachment;

		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, TestDeferredInputColorImage()[GetDrawSystemData()->CurrentFrame]);
		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, TestDeferredInputDepthImage()[GetDrawSystemData()->CurrentFrame]);

		BmRender_BeginRendering(SubmitPool->CommandBuffer, &RenderingInfo);
	}

	void MainPassEndPass()
	{
		CommandWorkerData* SubmitPool = GetSubmitPoolData(GetRenderState()->GraphicsCommandWorker);
		BmRender_EndRendering(SubmitPool->CommandBuffer);
	}

	AttachmentData* MainPassGetAttachmentData()
	{
		return &MainPassPipelineAttachmentData;
	}
}
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

#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

#include <Engine/Generated/Deferred.generated.h>
#include <Engine/Generated/Entity.generated.h>
#include <Engine/Generated/Depth_vert.generated.h>

#include <RenderHelper.h>
#include <PipelineMetadata.h>

#include <random>
#include <mutex>

#include <SharedLib.h>

// Extern declarations for global resource maps
extern std::unordered_map<std::string, BmRender_Sampler> Samplers;
extern std::unordered_map<std::string, BmRender_Shader> Shaders;
extern std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
extern std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;

static BmRender_Image ShadowMapArray;

static BmRender_GPUBuffer FrameDataBuffer;
static BmRender_GPUBuffer VertexBuffer;
static BmRender_GPUBuffer IndexBuffer;
static BmRender_GPUBuffer InstanceBuffer;
static BmRender_GPUBuffer MaterialBuffer;

static Render::DescriptorSetHandles DescriptorSets;

static BmRender_GPUBufferUpdateData FrameBufferBinding[1];

static BmRender_DescriptorSetLayout FrameDataLayout;
static BmRender_DescriptorSetLayout ShadowMapArrayLayout;
static BmRender_DescriptorSetLayout LightSpaceMatrixLayout;
static BmRender_DescriptorSetLayout MainPassOutputLayout;

namespace Render
{
	void FillStageDescriptionsFromMetadata(const Metadata_Pipeline* Metadata, BmRender_ShaderStageDescription* OutStageDescriptions)
	{
		for (u32 i = 0; i < Metadata->StageCount; ++i)
		{
			BmRender_ShaderStageDescription* Stage = OutStageDescriptions + i;
			Stage->Shader = Shaders[Metadata->ModuleName];
			Stage->EntryPointFunction = Metadata->Stages[i].EntryPoint;
			Stage->Stage = Metadata->Stages[i].Stage;
		}
	}

	static void GenericDraw(BmRender_CommandBuffer CommandBuffer, DrawScene* Scene, BmRender_PipelineLayout Layout, BmRender_Pipeline Pipeline,
		const BmRender_DescriptorSet* Sets, u32 SetsCount, const u32* DynamicOffsets, u32 DynamicOffsetsCount)
	{
		const u32 CurrentFrame = GetDrawSystemData()->CurrentFrame;
		const u32 FrameDynamicOffset = CurrentFrame * sizeof(Shader_FrameData);

		BmRender_BindPipeline(CommandBuffer, Pipeline);

		BmRender_RecordBindDescriptorSets(CommandBuffer, Layout, 0, 1, &DescriptorSets.FrameBufferSet, 1, &FrameDynamicOffset);
		BmRender_RecordBindDescriptorSets(CommandBuffer, Layout, 1, SetsCount, Sets, DynamicOffsetsCount, DynamicOffsets);

		std::unique_lock Lock(Scene->TempLock);

		for (u32 i = 0; i < Scene->DrawEntities.size(); ++i)
		{
			DrawEntity* Entity = Scene->DrawEntities.data() + i;

			BmRender_RecordBindIndexBuffer(CommandBuffer, Entity->IndexBufferEntry.GPUBufferHandle, Entity->IndexBufferEntry.BufferOffset, BmRender_IndexType::Uint32);
			BmRender_DrawIndexed(CommandBuffer, Entity->IndicesCount, Entity->Instances, 0, 0, i);
		}
	}

	static void InitImGuiPipeline(BmRender_DescriptorPool* ImGuiPool, GLFWwindow* Wnd)
	{
		ImGui_ImplGlfw_InitForVulkan(Wnd, true);

		AttachmentData* AttachmentDataPtr = DeferredPassGetAttachmentData();
		VkFormat* ColorAttachmentFormats = (VkFormat*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), AttachmentDataPtr->ColorAttachmentCount * sizeof(VkFormat));
		for (u32 i = 0; i < AttachmentDataPtr->ColorAttachmentCount; ++i)
		{
			const BmRender_Format Format = BmRender_GetOwningImageFormat(AttachmentDataPtr->ColorAttachments[i]);
			if (Format != BmRender_Format::Undefined)
			{
				ColorAttachmentFormats[i] = BmRender_FormatToVk(Format);
			}
			else
			{
				ColorAttachmentFormats[i] = VK_FORMAT_UNDEFINED;
			}
		}

		VkFormat DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		if (AttachmentDataPtr->DepthAttachment != nullptr)
		{
			const BmRender_Format Format = BmRender_GetOwningImageFormat(AttachmentDataPtr->DepthAttachment);
			if (Format != BmRender_Format::Undefined)
			{
				DepthAttachmentFormat = BmRender_FormatToVk(Format);
			}
		}

		VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
		if (AttachmentDataPtr->StencilAttachment != nullptr)
		{
			const BmRender_Format Format = BmRender_GetOwningImageFormat(AttachmentDataPtr->StencilAttachment);
			if (Format != BmRender_Format::Undefined)
			{
				StencilAttachmentFormat = BmRender_FormatToVk(Format);
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

	static void InitStaticMeshPipeline(StaticMeshPipelineDepr* MeshPipeline, BmRender_DescriptorPool MainPool)
	{
		BmRender_DescriptorSetLayoutBinding LayoutBindings[1];
		LayoutBindings[0].DescriptorCount = 1;
		LayoutBindings[0].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
		LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Fragment;

		ShadowMapArrayLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, 1);

		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_SHADOW_TEXTURES * i, MAX_SHADOW_TEXTURES);
			
			BmRender_DescriptorSetUpdateData ShadowMapBinding;
			ShadowMapBinding.ImageBinding.Sampler = Samplers["ShadowMap"];
			ShadowMapBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			ShadowMapBinding.ImageBinding.ImageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapBinding.BindingCount = 1;
			ShadowMapBinding.DstArrayElement = 0;
			ShadowMapBinding.DstBinding = 0;

			MeshPipeline->ShadowMapArraySet[i] = BmRender_CreateDescriptorSet(ShadowMapArrayLayout, MainPool);
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
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;
		
		BmRender_ShaderStageDescription StageDescriptions[Metadata_EntityPipeline.StageCount];
		FillStageDescriptionsFromMetadata(&Metadata_EntityPipeline, StageDescriptions);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(FrameDataLayout);
		descriptorSetLayouts.push_back(ShadowMapArrayLayout);
			
		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = MainScreenExtent;
		PipelineDesc.Attachment = ResourceInfo;

		// Set shader stages
		PipelineDesc.ShaderStages = StageDescriptions;
		PipelineDesc.ShaderStagesCount = Metadata_EntityPipeline.StageCount;

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

	static void DrawStaticMeshes(BmRender_CommandBuffer CommandBuffer, StaticMeshPipelineDepr* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets)
	{
		u32 CurrentFrame = GetDrawSystemData()->CurrentFrame;

		const BmRender_DescriptorSet DescriptorSetGroup[] =
		{
			MeshPipeline->ShadowMapArraySet[CurrentFrame],
		};

		const u32 SetsCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

		GenericDraw(CommandBuffer, Scene, PipelineLayouts["StaticMesh"], Pipelines["StaticMesh"], DescriptorSetGroup, SetsCount, nullptr, 0);
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

	void Init(GLFWwindow* WindowHandler)
	{
		// Create MainPool using stack array
		const u32 PoolSizeCount = 11;
		BmRender_DescriptorPoolSize TotalPassPoolSizes[PoolSizeCount];
		u32 TotalDescriptorLayouts = 21;
		TotalPassPoolSizes[0] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[1] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[2] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[3] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[4] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[5] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[6] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[7] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[8] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[9] = { BmRender_DescriptorType::CombinedImageSampler, 256 };
		TotalPassPoolSizes[10] = { BmRender_DescriptorType::UniformBufferDynamic, 3 };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * 3;
		TotalDescriptorCount += 256;

		BmRender_DescriptorPool MainPool = BmRender_CreateDescriptorPool(TotalPassPoolSizes, TotalDescriptorCount, PoolSizeCount, BmRender_DescriptorPoolType::UpdateAfterBind);

		FrameDataBuffer = BmRender_CreateUniformBuffer(65536, MemoryPropertyFlag::HostCompatible);
		VertexBuffer = BmRender_CreateStorageBuffer(MB4, MemoryPropertyFlag::GPULocal);
		IndexBuffer = BmRender_CreateVertexStageBuffer(MB4, MemoryPropertyFlag::GPULocal);
		InstanceBuffer = BmRender_CreateStorageBuffer(MB4, MemoryPropertyFlag::GPULocal);
		MaterialBuffer = BmRender_CreateStorageBuffer(MB4, MemoryPropertyFlag::GPULocal);

		InitCommandSystem(3);
		InitDrawSystem(3);

		

		DescriptorSets = Render::DescriptorSetHandles();

		{
			FrameBufferBinding[0] = { FrameDataBuffer, 0, sizeof(Shader_FrameData) };
			BmRender_GPUBufferUpdateData VertexBufferRegion = { VertexBuffer, 0, VK_WHOLE_SIZE };
			BmRender_GPUBufferUpdateData InstanceBufferRegion = { InstanceBuffer, 0, VK_WHOLE_SIZE };
			BmRender_GPUBufferUpdateData MaterialBufferRegion = { MaterialBuffer, 0, VK_WHOLE_SIZE };

			const u32 DescriptorCount = 5;
			BmRender_DescriptorSetLayoutBinding LayoutBindings[DescriptorCount];

			LayoutBindings[0].DescriptorCount = 1;
			LayoutBindings[0].DescriptorType = BmRender_DescriptorType::UniformBufferDynamic;
			LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment;

			LayoutBindings[1].DescriptorCount = 1;
			LayoutBindings[1].DescriptorType = BmRender_DescriptorType::StorageBuffer;
			LayoutBindings[1].StageFlags = BmRender_DescriptorShaderStage::Vertex;

			LayoutBindings[2].DescriptorCount = 1;
			LayoutBindings[2].DescriptorType = BmRender_DescriptorType::StorageBuffer;
			LayoutBindings[2].StageFlags = BmRender_DescriptorShaderStage::Vertex;

			LayoutBindings[3].DescriptorCount = 64;
			LayoutBindings[3].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
			LayoutBindings[3].StageFlags = BmRender_DescriptorShaderStage::Fragment;

			LayoutBindings[4].DescriptorCount = 1;
			LayoutBindings[4].DescriptorType = BmRender_DescriptorType::StorageBuffer;
			LayoutBindings[4].StageFlags = BmRender_DescriptorShaderStage::Fragment;

			const u32 UpdatesCount = 4;
			BmRender_DescriptorSetUpdateData Updates[UpdatesCount];
			Updates[0].BufferRegions = FrameBufferBinding;
			Updates[0].BindingCount = 1;
			Updates[0].DstArrayElement = 0;
			Updates[0].DstBinding = 0;

			Updates[1].BufferRegions = &VertexBufferRegion;
			Updates[1].BindingCount = 1;
			Updates[1].DstArrayElement = 0;
			Updates[1].DstBinding = 1;

			Updates[2].BufferRegions = &InstanceBufferRegion;
			Updates[2].BindingCount = 1;
			Updates[2].DstArrayElement = 0;
			Updates[2].DstBinding = 2;

			Updates[3].BufferRegions = &MaterialBufferRegion;
			Updates[3].BindingCount = 1;
			Updates[3].DstArrayElement = 0;
			Updates[3].DstBinding = 4;

			FrameDataLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, DescriptorCount);
			DescriptorSets.FrameBufferSet = BmRender_CreateDescriptorSet(FrameDataLayout, MainPool);
			BmRender_UpdateDescriptorSet(DescriptorSets.FrameBufferSet, Updates, UpdatesCount);
		}

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

		BmRender_DestroyDescriptorSetLayout(FrameDataLayout);
		BmRender_DestroyDescriptorSetLayout(ShadowMapArrayLayout);
		BmRender_DestroyDescriptorSetLayout(MainPassOutputLayout);
		BmRender_DestroyDescriptorSetLayout(LightSpaceMatrixLayout);

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

		// Destroy GPUBuffers
		BmRender_DestroyGPUBuffer(VertexBuffer);
		BmRender_DestroyGPUBuffer(IndexBuffer);
		BmRender_DestroyGPUBuffer(InstanceBuffer);
		BmRender_DestroyGPUBuffer(MaterialBuffer);
		BmRender_DestroyGPUBuffer(FrameDataBuffer);
	}

	void Draw(DrawScene* Scene, u64 WaitSemaphoreValue)
	{
		const u32 CurrentFrame = GetCurrentFrameIndex();

		RenderResources::UpdateBuffer(FrameDataBuffer, sizeof(Shader_FrameData) * CurrentFrame, &Scene->FrameDataBuffer, sizeof(Shader_FrameData));

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

		{
			const u32 BindingsCover = 2;
			BmRender_DescriptorSetLayoutBinding LayoutBindings[BindingsCover];
			LayoutBindings[0].DescriptorCount = 1;
			LayoutBindings[0].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
			LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Fragment;

			LayoutBindings[1].DescriptorCount = 1;
			LayoutBindings[1].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
			LayoutBindings[1].StageFlags = BmRender_DescriptorShaderStage::Fragment;

			MainPassOutputLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, BindingsCover);

			for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
			{
				BmRender_DescriptorSetUpdateData ColorBinding;
				ColorBinding.ImageBinding.Sampler = Samplers["ColorAttachment"];
				ColorBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
				ColorBinding.ImageBinding.ImageView = DeferredInputColorImageInterface[i];
				ColorBinding.BindingCount = 1;
				ColorBinding.DstArrayElement = 0;
				ColorBinding.DstBinding = 0;

				BmRender_DescriptorSetUpdateData DepthBinding;
				DepthBinding.ImageBinding.Sampler = Samplers["DepthAttachment"];
				DepthBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
				DepthBinding.ImageBinding.ImageView = DeferredInputDepthImageInterface[i];
				DepthBinding.BindingCount = 1;
				DepthBinding.DstArrayElement = 0;
				DepthBinding.DstBinding = 1;

				BmRender_DescriptorSetUpdateData Bindings[] = { ColorBinding, DepthBinding };

				DeferredInputSet[i] = BmRender_CreateDescriptorSet(MainPassOutputLayout, MainPool);
				BmRender_UpdateDescriptorSet(DeferredInputSet[i], Bindings, 2);
			}
		}

		// Create vectors to hold pipeline data
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		BmRender_ShaderStageDescription StageDescriptions[Metadata_DeferredPipeline.StageCount];
		FillStageDescriptionsFromMetadata(&Metadata_DeferredPipeline, StageDescriptions);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(FrameDataLayout);
		descriptorSetLayouts.push_back(MainPassOutputLayout);

		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = MainScreenExtent;
		PipelineDesc.Attachment = DeferredPassPipelineAttachmentData;

		// Set shader stages
		PipelineDesc.ShaderStages = StageDescriptions;
		PipelineDesc.ShaderStagesCount = Metadata_DeferredPipeline.StageCount;

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

		const u32 CurrentFrame = GetDrawSystemData()->CurrentFrame;

		const BmRender_DescriptorSet Sets[2] = {
			DescriptorSets.FrameBufferSet,
			DeferredInputSet[CurrentFrame],
		};

		const u32 FrameDynamicOffset = CurrentFrame * sizeof(Shader_FrameData);
		const u32 DynamicOffsets[] = { FrameDynamicOffset };

		BmRender_RecordBindDescriptorSets(SubmitPool->CommandBuffer, PipelineLayouts["Deferred"],
			0, 2, Sets, 1, DynamicOffsets);

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

	static BmRender_GPUBufferUpdateData LightSpaceMatrixBufferRegion[MAX_DRAW_FRAMES];
	
	// Buffer handles array
	static BmRender_GPUBuffer LightSpaceMatrixBuffers[MAX_DRAW_FRAMES];

	static BmRender_ImageView ShadowMapElement1ImageInterface[MAX_DRAW_FRAMES];
	static BmRender_ImageView ShadowMapElement2ImageInterface[MAX_DRAW_FRAMES];

	void LightningPassInit(BmRender_DescriptorPool MainPool)
	{
		ShadowMapArray = BmRender_CreateImage2DArray(DepthViewportExtent.Width, DepthViewportExtent.Height, DepthFormat,
			BmRender_ImageType::DepthSamplad, MAX_SHADOW_TEXTURES * BmRender_GetSwapchainImageCount(), BmRender_SampleCount::Count1);

		{
			BmRender_DescriptorSetLayoutBinding LayoutBindings[1];
			LayoutBindings[0].DescriptorCount = 1;
			LayoutBindings[0].DescriptorType = BmRender_DescriptorType::UniformBuffer;
			LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Vertex;

			LightSpaceMatrixLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, 1);

			for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
			{
				const u64 LightSpaceMatrixSize = sizeof(glm::mat4);

				LightSpaceMatrixBuffers[i] = BmRender_CreateUniformBuffer(LightSpaceMatrixSize, MemoryPropertyFlag::HostCompatible);
				LightSpaceMatrixBufferRegion[i] = { LightSpaceMatrixBuffers[i], 0, LightSpaceMatrixSize };

				LightSpaceMatrixSet[i] = BmRender_CreateDescriptorSet(LightSpaceMatrixLayout, MainPool);

				BmRender_DescriptorSetUpdateData LightSpaceMatrixBinding;
				LightSpaceMatrixBinding.BufferRegions = &LightSpaceMatrixBufferRegion[i];
				LightSpaceMatrixBinding.BindingCount = 1;
				LightSpaceMatrixBinding.DstArrayElement = 0;
				LightSpaceMatrixBinding.DstBinding = 0;

				BmRender_UpdateDescriptorSet(LightSpaceMatrixSet[i], &LightSpaceMatrixBinding, 1);

				ShadowMapElement1ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_SHADOW_TEXTURES * i, 1);
				ShadowMapElement2ImageInterface[i] = BmRender_CreateImageView2DArray(ShadowMapArray, MAX_SHADOW_TEXTURES * i + 1, 1);
			}
		}

		AttachmentData ResourceInfo;
		ResourceInfo.ColorAttachmentCount = 0;
		ResourceInfo.ColorAttachments = nullptr;
		ResourceInfo.DepthAttachment = ShadowMapElement1ImageInterface[0];
		ResourceInfo.StencilAttachment = nullptr;

		// Create vectors to hold pipeline data
		std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
		std::vector<BmRender_PushConstant> pushConstantRanges;

		BmRender_ShaderStageDescription StageDescriptions[Metadata_Depth_vertPipeline.StageCount];
		FillStageDescriptionsFromMetadata(&Metadata_Depth_vertPipeline, StageDescriptions);

		// Build descriptor set layouts
		descriptorSetLayouts.push_back(FrameDataLayout);
		descriptorSetLayouts.push_back(LightSpaceMatrixLayout);

		// Build pipeline description
		BmRender_PipelineDescription PipelineDesc = {};
		PipelineDesc.Extent = DepthViewportExtent;
		PipelineDesc.Attachment = ResourceInfo;

		// Set shader stages
		PipelineDesc.ShaderStages = StageDescriptions;
		PipelineDesc.ShaderStagesCount = Metadata_Depth_vertPipeline.StageCount;

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
			&Scene->FrameDataBuffer.directionLight.LightSpaceMatrix,
			&Scene->FrameDataBuffer.spotlight.LightSpaceMatrix,
		};

		BmRender_TransitionImageForRendering(SubmitPool->CommandBuffer, ShadowMapArray, MAX_SHADOW_TEXTURES * GetDrawSystemData()->CurrentFrame, MAX_SHADOW_TEXTURES);

		for (u32 LightCaster = 0; LightCaster < MAX_SHADOW_TEXTURES; ++LightCaster)
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

			const u32 SetsCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			GenericDraw(SubmitPool->CommandBuffer, Scene, PipelineLayouts["Depth"], Pipelines["Depth"], DescriptorSetGroup, SetsCount, nullptr, 0);

			BmRender_EndRendering(SubmitPool->CommandBuffer);
		}

		// TODO: move to Main pass?
		BmRender_TransitionImageForSampling(SubmitPool->CommandBuffer, ShadowMapArray, MAX_SHADOW_TEXTURES * GetDrawSystemData()->CurrentFrame, MAX_SHADOW_TEXTURES);
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

	Render::DescriptorSetHandles* GetHandles()
	{
		return &DescriptorSets;
	}

	BmRender_GPUBuffer GetVertexBuffer()
	{
		return VertexBuffer;
	}

	BmRender_GPUBuffer GetIndexBuffer()
	{
		return IndexBuffer;
	}

	BmRender_GPUBuffer GetInstanceBuffer()
	{
		return InstanceBuffer;
	}

	BmRender_GPUBuffer GetMaterialBuffer()
	{
		return MaterialBuffer;
	}
}
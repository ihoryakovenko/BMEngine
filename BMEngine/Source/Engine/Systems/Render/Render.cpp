#include "Render.h"

#include "RenderResources.h"
#include "TransferSystem.h"

//#include "imgui.h"
//#include "imgui_impl_vulkan.h"
//#include "imgui_impl_glfw.h"

#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/Settings.h"
#include "Util/Math.h"

#include "PipelineSettings.h"
#include "RenderResourceManager.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

#include <Engine/Generated/ShaderRegistry.generated.h>


#include "PipelineMetadata.h"

#include <random>
#include <mutex>

#include <SharedLib.h>

#define DEFINE_ENUM_OR(EnumType) constexpr inline EnumType operator| (EnumType lhs, EnumType rhs) { return (EnumType)((u64)(lhs) | (u64)(rhs)); }
DEFINE_ENUM_OR(BmRender_DescriptorShaderStage);

static BmRender_Semaphore ImageAvailable[MAX_DRAW_FRAMES];
static BmRender_Semaphore RenderFinished[MAX_DRAW_FRAMES];
static BmRender_Fence InFlightFence[MAX_DRAW_FRAMES];
static BmRender_CommandBuffer RenderCommandBuffers[MAX_DRAW_FRAMES];
static BmRender_CommandPool RenderCommandPool;
BmRender_Queue GraphicsQueue;
static u32 CurrentFrame;

BmRender_Sampler ShadowMapSampler;
BmRender_Sampler DiffuseTextureSampler;
BmRender_Sampler ColorAttachmentSampler;
BmRender_Sampler DepthAttachmentSampler;

static BmRender_Image ShadowMapArray;

static BmRender_GPUBuffer FrameDataBuffer;
static BmRender_GPUBuffer VertexBuffer;
static BmRender_GPUBuffer IndexBuffer;
static BmRender_GPUBuffer InstanceBuffer;
static BmRender_GPUBuffer MaterialBuffer;

static DescriptorSetHandles DescriptorSets;

static BmRender_GPUBufferUpdateData FrameBufferBinding[1];

static BmRender_DescriptorSetLayout FrameDataLayout;
static BmRender_DescriptorSetLayout ShadowMapArrayLayout;
static BmRender_DescriptorSetLayout LightSpaceMatrixLayout;
static BmRender_DescriptorSetLayout MainPassOutputLayout;

static RenderState State;
static u32 CurrentImageIndex;

// DeferredPass static variables
static BmRender_Image DeferredInputDepthImage[MAX_DRAW_FRAMES];
static BmRender_Image DeferredInputColorImage[MAX_DRAW_FRAMES];

static BmRender_ImageView DeferredInputDepthImageInterface[MAX_DRAW_FRAMES];
static BmRender_ImageView DeferredInputColorImageInterface[MAX_DRAW_FRAMES];

static BmRender_DescriptorSet DeferredInputSet[MAX_DRAW_FRAMES];

static AttachmentData DeferredPassPipelineAttachmentData;
// LightningPass static variables
static BmRender_DescriptorSet LightSpaceMatrixSet[MAX_DRAW_FRAMES];

static BmRender_GPUBufferUpdateData LightSpaceMatrixBufferRegion[MAX_DRAW_FRAMES];

// Buffer handles array
static BmRender_GPUBuffer LightSpaceMatrixBuffers[MAX_DRAW_FRAMES];

static BmRender_ImageView ShadowMapElement1ImageInterface[MAX_DRAW_FRAMES];
static BmRender_ImageView ShadowMapElement2ImageInterface[MAX_DRAW_FRAMES];

// MainPass static variables
static AttachmentData MainPassPipelineAttachmentData;

static void GenericDraw(BmRender_CommandBuffer CommandBuffer, DrawScene* Scene, PipelineNames Name,
	const BmRender_DescriptorSet* Sets, u32 SetsCount, const u32* DynamicOffsets, u32 DynamicOffsetsCount)
{
	const u32 FrameDynamicOffset = CurrentFrame * sizeof(Shader_FrameData);

	RenderResourceManager_BindPipeline(CommandBuffer, Name);

	RenderResourceManager_RecordBindDescriptorSets(CommandBuffer, Name, 0, 1, &DescriptorSets.FrameBufferSet, 1, &FrameDynamicOffset);
	RenderResourceManager_RecordBindDescriptorSets(CommandBuffer, Name, 1, SetsCount, Sets, DynamicOffsetsCount, DynamicOffsets);

	for (u32 i = 0; i < Scene->DrawEntities.size(); ++i)
	{
		DrawEntity* Entity = Scene->DrawEntities.data() + i;

		u64 FrameDataAddr = BmRender_GetBufferDeviceAddress(&FrameDataBuffer) + sizeof(Shader_FrameData) * CurrentFrame;
		//BmRender_RecordPushData(CommandBuffer, &FrameDataAddr, sizeof(FrameDataAddr));

		BmRender_DrawIndexed(CommandBuffer, Entity->IndicesCount, Entity->Instances, Entity->IndexBufferEntry.BufferOffset / 4, 0, i);
	}
}

static void InitImGuiPipeline(BmRender_DescriptorPool* ImGuiPool, GLFWwindow* Wnd)
{
	//ImGui_ImplGlfw_InitForVulkan(Wnd, true);

	//AttachmentData* AttachmentDataPtr = &DeferredPassPipelineAttachmentData;
	//VkFormat* ColorAttachmentFormats = Memory_LinearAllocator_AllocTC(Memory::GetGeneralFrameMemory(), VkFormat, AttachmentDataPtr->ColorAttachmentCount);
	//for (u32 i = 0; i < AttachmentDataPtr->ColorAttachmentCount; ++i)
	//{
	//	const BmRender_Format Format = AttachmentDataPtr->ColorAttachments[i].Format;
	//	if (Format != BmRender_Format::Undefined)
	//	{
	//		ColorAttachmentFormats[i] = BmRender_FormatToVk(Format);
	//	}
	//	else
	//	{
	//		ColorAttachmentFormats[i] = VK_FORMAT_UNDEFINED;
	//	}
	//}

	//VkFormat DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	//if (AttachmentDataPtr->DepthAttachment)
	//{
	//	const BmRender_Format Format = AttachmentDataPtr->DepthAttachment->Format;
	//	if (Format != BmRender_Format::Undefined)
	//	{
	//		DepthAttachmentFormat = BmRender_FormatToVk(Format);
	//	}
	//}

	//VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	//if (AttachmentDataPtr->StencilAttachment)
	//{
	//	const BmRender_Format Format = AttachmentDataPtr->StencilAttachment->Format;
	//	if (Format != BmRender_Format::Undefined)
	//	{
	//		StencilAttachmentFormat = BmRender_FormatToVk(Format);
	//	}
	//}

	//VkPipelineRenderingCreateInfo RenderingInfo = { };
	//RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	//RenderingInfo.pNext = nullptr;
	//RenderingInfo.colorAttachmentCount = AttachmentDataPtr->ColorAttachmentCount;
	//RenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats;
	//RenderingInfo.depthAttachmentFormat = DepthAttachmentFormat;
	//RenderingInfo.stencilAttachmentFormat = StencilAttachmentFormat;

	//BmRender_DescriptorPoolSize PoolSizes[] =
	//{
	//	{ BmRender_DescriptorType::CombinedImageSampler, 1 },
	//};

	//*ImGuiPool = BmRender_CreateDescriptorPool(PoolSizes, 1, (u32)IM_ARRAYSIZE(PoolSizes), BmRender_DescriptorPoolType::CreateFree);

	//BmRender_Queue ImbuiGraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);
	//ImGui_ImplVulkan_InitInfo InitInfo = { };
	//InitInfo.Instance = (VkInstance)BmRender_GetVulkanInstance();
	//InitInfo.PhysicalDevice = (VkPhysicalDevice)BmRender_GetPhysicalDevice();
	//InitInfo.Device = (VkDevice)BmRender_GetLogicalDevice();
	//InitInfo.QueueFamily = BmRender_GetQueueFamily(ImbuiGraphicsQueue);
	//InitInfo.Queue = ImbuiGraphicsQueue.InternalQueue;
	//InitInfo.PipelineCache = nullptr;
	//InitInfo.DescriptorPool = *((VkDescriptorPool*)ImGuiPool);
	//InitInfo.RenderPass = nullptr;
	//InitInfo.UseDynamicRendering = true;
	//InitInfo.MinImageCount = 2;
	//InitInfo.ImageCount = 3;
	//InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	//InitInfo.PipelineRenderingCreateInfo = RenderingInfo;
	//InitInfo.Allocator = BmRender_GetVulkanAllocator();
	//ImGui_ImplVulkan_Init(&InitInfo);

	//ImGui_ImplVulkan_CreateFontsTexture();
}

static void DeInitImGuiPipeline(BmRender_DescriptorPool ImGuiPool)
{
	//ImGui_ImplVulkan_Shutdown();
	BmRender_DestroyDescriptorPool(ImGuiPool);
}

static void InitStaticMeshPipeline(StaticMeshPipelineDepr* MeshPipeline, BmRender_DescriptorPool* MainPool)
{
	BmRender_DescriptorSetLayoutBinding LayoutBindings[1];
	LayoutBindings[0].DescriptorCount = 1;
	LayoutBindings[0].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
	LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Fragment;

	ShadowMapArrayLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, 1);

	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
	{
		MeshPipeline->ShadowMapArrayImageInterface[i] = BmRender_CreateImageView2DArray(&ShadowMapArray, MAX_SHADOW_TEXTURES * i, MAX_SHADOW_TEXTURES);
			
		BmRender_DescriptorSetUpdateData ShadowMapBinding;
		ShadowMapBinding.ImageBinding.Sampler = &ShadowMapSampler;
		ShadowMapBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
		ShadowMapBinding.ImageBinding.ImageView = &MeshPipeline->ShadowMapArrayImageInterface[i];
		ShadowMapBinding.BindingCount = 1;
		ShadowMapBinding.DstArrayElement = 0;
		ShadowMapBinding.DstBinding = 0;

		MeshPipeline->ShadowMapArraySet[i] = BmRender_CreateDescriptorSet(&ShadowMapArrayLayout, MainPool);
		BmRender_UpdateDescriptorSet(MeshPipeline->ShadowMapArraySet + i, &ShadowMapBinding, 1);
	}

	AttachmentData ResourceInfo = MainPassPipelineAttachmentData;
	for (u32 i = 0; i < ResourceInfo.ColorAttachmentCount; ++i)
	{
		ResourceInfo.ColorAttachments[i] = MainPassPipelineAttachmentData.ColorAttachments[i];
	}

	std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;

	descriptorSetLayouts.push_back(FrameDataLayout);
	descriptorSetLayouts.push_back(ShadowMapArrayLayout);

	BmRender_PipelineSettings Settings = GetStaticPipelineDescription();

	RenderResourceManager_CreatePipelineLayout(PipelineNames::Entity, descriptorSetLayouts.data(), descriptorSetLayouts.size(), nullptr, 0);
	RenderResourceManager_CreateGraphicsPipeline(PipelineNames::Entity, &Settings, &ResourceInfo);
}

static void DrawStaticMeshes(BmRender_CommandBuffer CommandBuffer, StaticMeshPipelineDepr* MeshPipeline, DrawScene* Scene, const DescriptorSetHandles& DescriptorSets)
{
	const BmRender_DescriptorSet DescriptorSetGroup[] =
	{
		MeshPipeline->ShadowMapArraySet[CurrentFrame],
	};

	const u32 SetsCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

	GenericDraw(CommandBuffer, Scene, PipelineNames::Entity, DescriptorSetGroup, SetsCount, nullptr, 0);
}

static void DeferredPassInit(BmRender_DescriptorPool* MainPool)
{
	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
	{
		DeferredInputColorImage[i] = BmRender_CreateImage2D(MainScreenExtent.Width, MainScreenExtent.Height, ColorFormat, BmRender_ImageType::ColorAttachmentSampled, BmRender_SampleCount::Count1, "DeferredInputColorImage");
		DeferredInputDepthImage[i] = BmRender_CreateImage2D(MainScreenExtent.Width, MainScreenExtent.Height, DepthFormat, BmRender_ImageType::DepthSamplad, BmRender_SampleCount::Count1, "DeferredInputDepthImage");

		DeferredInputColorImageInterface[i] = BmRender_CreateImageView2D(DeferredInputColorImage + i);
		DeferredInputDepthImageInterface[i] = BmRender_CreateImageView2D(DeferredInputDepthImage + i);
	}

	DeferredPassPipelineAttachmentData.ColorAttachmentCount = 1;
	DeferredPassPipelineAttachmentData.ColorAttachments = DeferredInputColorImageInterface;
	DeferredPassPipelineAttachmentData.DepthAttachment = &DeferredInputDepthImageInterface[0];
	DeferredPassPipelineAttachmentData.StencilAttachment = {};

	{
		const u32 BindingsCount = 3;
		BmRender_DescriptorSetLayoutBinding LayoutBindings[BindingsCount];
		LayoutBindings[0].DescriptorCount = 1;
		LayoutBindings[0].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
		LayoutBindings[0].StageFlags = BmRender_DescriptorShaderStage::Compute;

		LayoutBindings[1].DescriptorCount = 1;
		LayoutBindings[1].DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
		LayoutBindings[1].StageFlags = BmRender_DescriptorShaderStage::Compute;

		LayoutBindings[2].DescriptorCount = 1;
		LayoutBindings[2].DescriptorType = BmRender_DescriptorType::StorageImage;
		LayoutBindings[2].StageFlags = BmRender_DescriptorShaderStage::Compute;

		MainPassOutputLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, BindingsCount);

		for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
		{
			BmRender_DescriptorSetUpdateData ColorBinding;
			ColorBinding.ImageBinding.Sampler = &ColorAttachmentSampler;
			ColorBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			ColorBinding.ImageBinding.ImageView = &DeferredInputColorImageInterface[i];
			ColorBinding.BindingCount = 1;
			ColorBinding.DstArrayElement = 0;
			ColorBinding.DstBinding = 0;

			BmRender_DescriptorSetUpdateData DepthBinding;
			DepthBinding.ImageBinding.Sampler = &DepthAttachmentSampler;
			DepthBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
			DepthBinding.ImageBinding.ImageView = &DeferredInputDepthImageInterface[i];
			DepthBinding.BindingCount = 1;
			DepthBinding.DstArrayElement = 0;
			DepthBinding.DstBinding = 1;

			BmRender_DescriptorSetUpdateData OutputTextureBinding = {};
			OutputTextureBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::General;
			OutputTextureBinding.ImageBinding.ImageView = BmRender_GetSwapchainImageView(i);
			OutputTextureBinding.BindingCount = 1;
			OutputTextureBinding.DstArrayElement = 0;
			OutputTextureBinding.DstBinding = 2;

			BmRender_DescriptorSetUpdateData Bindings[] = { ColorBinding, DepthBinding, OutputTextureBinding };

			DeferredInputSet[i] = BmRender_CreateDescriptorSet(&MainPassOutputLayout, MainPool);
			BmRender_UpdateDescriptorSet(DeferredInputSet + i, Bindings, BindingsCount);
		}
	}

	std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
	descriptorSetLayouts.push_back(FrameDataLayout);
	descriptorSetLayouts.push_back(MainPassOutputLayout);

	RenderResourceManager_CreatePipelineLayout(PipelineNames::Deferred, descriptorSetLayouts.data(), descriptorSetLayouts.size(), nullptr, 0);
	RenderResourceManager_CreateComputePipeline(PipelineNames::Deferred);
}

static void DeferredPassDispatch()
{
	RenderResourceManager_BindPipeline(RenderCommandBuffers[CurrentFrame], PipelineNames::Deferred);

	const BmRender_DescriptorSet Sets[2] = {
		DescriptorSets.FrameBufferSet,
		DeferredInputSet[CurrentFrame],
	};

	const u32 FrameDynamicOffset = CurrentFrame * sizeof(Shader_FrameData);
	const u32 DynamicOffsets[] = { FrameDynamicOffset };

	RenderResourceManager_RecordBindDescriptorSets(RenderCommandBuffers[CurrentFrame], PipelineNames::Deferred, 0, 2, Sets, 1, DynamicOffsets);

	u32 groupX = (MainScreenExtent.Width + 7) / 8;
	u32 groupY = (MainScreenExtent.Height + 7) / 8;

	BmRender_RecordDispatch(RenderCommandBuffers[CurrentFrame], groupX, groupY, 1);
}

static void DeferredPassDeInit()
{
	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
	{
		BmRender_DestroyImageView(DeferredInputColorImageInterface[i]);
		BmRender_DestroyImageView(DeferredInputDepthImageInterface[i]);
		BmRender_DestroyImage(DeferredInputColorImage + i);
		BmRender_DestroyImage(DeferredInputDepthImage + i);
	}
}

static void LightningPassInit(BmRender_DescriptorPool* MainPool)
{
	ShadowMapArray = BmRender_CreateImage2DArray(DepthViewportExtent.Width, DepthViewportExtent.Height, DepthFormat,
		BmRender_ImageType::DepthSamplad, MAX_SHADOW_TEXTURES * BmRender_GetSwapchainImageCount(), BmRender_SampleCount::Count1, "ShadowMapArray");

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
			LightSpaceMatrixBufferRegion[i] = { LightSpaceMatrixBuffers + i, 0, LightSpaceMatrixSize };

			LightSpaceMatrixSet[i] = BmRender_CreateDescriptorSet(&LightSpaceMatrixLayout, MainPool);

			BmRender_DescriptorSetUpdateData LightSpaceMatrixBinding;
			LightSpaceMatrixBinding.BufferRegions = &LightSpaceMatrixBufferRegion[i];
			LightSpaceMatrixBinding.BindingCount = 1;
			LightSpaceMatrixBinding.DstArrayElement = 0;
			LightSpaceMatrixBinding.DstBinding = 0;

			BmRender_UpdateDescriptorSet(LightSpaceMatrixSet + i, &LightSpaceMatrixBinding, 1);

			ShadowMapElement1ImageInterface[i] = BmRender_CreateImageView2DArray(&ShadowMapArray, MAX_SHADOW_TEXTURES * i, 1);
			ShadowMapElement2ImageInterface[i] = BmRender_CreateImageView2DArray(&ShadowMapArray, MAX_SHADOW_TEXTURES * i + 1, 1);
		}
	}

	AttachmentData ResourceInfo;
	ResourceInfo.ColorAttachmentCount = 0;
	ResourceInfo.DepthAttachment = &ShadowMapElement1ImageInterface[0];
	ResourceInfo.StencilAttachment = {};

	std::vector<BmRender_DescriptorSetLayout> descriptorSetLayouts;
	descriptorSetLayouts.push_back(FrameDataLayout);
	descriptorSetLayouts.push_back(LightSpaceMatrixLayout);

	BmRender_PipelineSettings PipelineDesc = GetDepthPipelineDescription();

	RenderResourceManager_CreatePipelineLayout(PipelineNames::Depth_vert, descriptorSetLayouts.data(), descriptorSetLayouts.size(), nullptr, 0);
	RenderResourceManager_CreateGraphicsPipeline(PipelineNames::Depth_vert, &PipelineDesc, &ResourceInfo);
}

static void LightningPassDraw(DrawScene* Scene)
{
	const glm::mat4* LightViews[] =
	{
		&Scene->FrameDataBuffer.directionLight.LightSpaceMatrix,
		&Scene->FrameDataBuffer.spotlight.LightSpaceMatrix,
	};

	BmRender_TransitionImageForRendering(RenderCommandBuffers[CurrentFrame], &ShadowMapArray, MAX_SHADOW_TEXTURES * CurrentFrame, MAX_SHADOW_TEXTURES);

	for (u32 LightCaster = 0; LightCaster < MAX_SHADOW_TEXTURES; ++LightCaster)
	{
		RenderResources::UpdateBufferRegion(LightSpaceMatrixBufferRegion[LightCaster], 0, LightViews[LightCaster], sizeof(glm::mat4));

		BmRender_ImageView DepthImageView = (LightCaster == 0) ?
			ShadowMapElement1ImageInterface[CurrentFrame] :
			ShadowMapElement2ImageInterface[CurrentFrame];

		BmRender_RenderingDepthAttachment DepthAttachment{ };
		DepthAttachment.ImageView = &DepthImageView;
		DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		DepthAttachment.ClearValue = { 1.0f, 0 };

		BmRender_RenderingInfo RenderingInfo{ };
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = DepthViewportExtent;
		RenderingInfo.ColorAttachments = nullptr;
		RenderingInfo.ColorAttachmentCount = 0;
		RenderingInfo.DepthAttachment = &DepthAttachment;

		BmRender_BeginRendering(RenderCommandBuffers[CurrentFrame], &RenderingInfo);

		const BmRender_DescriptorSet DescriptorSetGroup[] = {
			LightSpaceMatrixSet[LightCaster],
		};

		const u32 SetsCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

		u64 FrameDataAddr = BmRender_GetBufferDeviceAddress(&FrameDataBuffer) + sizeof(Shader_FrameData) * CurrentFrame;
		//BmRender_RecordPushData(RenderCommandBuffers[CurrentFrame], &FrameDataAddr, sizeof(FrameDataAddr));

		GenericDraw(RenderCommandBuffers[CurrentFrame], Scene, PipelineNames::Depth_vert, DescriptorSetGroup, SetsCount, nullptr, 0);

		BmRender_EndRendering(RenderCommandBuffers[CurrentFrame]);
	}

	// TODO: move to Main pass?
	BmRender_TransitionImageForSampling(RenderCommandBuffers[CurrentFrame], &ShadowMapArray, MAX_SHADOW_TEXTURES * CurrentFrame, MAX_SHADOW_TEXTURES);
}

static void LightningPassDeInit()
{
	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
	{
		BmRender_DestroyGPUBuffer(LightSpaceMatrixBuffers + i);
		BmRender_DestroyImageView(ShadowMapElement1ImageInterface[i]);
		BmRender_DestroyImageView(ShadowMapElement2ImageInterface[i]);
	}
	BmRender_DestroyImage(&ShadowMapArray);
}

static void MainPassInit()
{
	MainPassPipelineAttachmentData.ColorAttachmentCount = 1;
	MainPassPipelineAttachmentData.ColorAttachments = TestDeferredInputColorImageInterface();
	MainPassPipelineAttachmentData.DepthAttachment = &TestDeferredInputDepthImageInterface()[0];
	MainPassPipelineAttachmentData.StencilAttachment = {};
}

static void MainPassBeginPass()
{
	BmRender_RenderingColorAttachment ColorAttachment = { };
	ColorAttachment.ImageView = &TestDeferredInputColorImageInterface()[CurrentFrame];
	ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
	ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
	ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	BmRender_RenderingDepthAttachment DepthAttachment = { };
	DepthAttachment.ImageView = &TestDeferredInputDepthImageInterface()[CurrentFrame];
	DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
	DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
	DepthAttachment.ClearValue = { 1.0f, 0 };

	BmRender_RenderingInfo RenderingInfo = { };
	RenderingInfo.Offset = { 0, 0 };
	RenderingInfo.Extent = MainScreenExtent;
	RenderingInfo.ColorAttachments = &ColorAttachment;
	RenderingInfo.ColorAttachmentCount = 1;
	RenderingInfo.DepthAttachment = &DepthAttachment;

	BmRender_TransitionImageForRendering(RenderCommandBuffers[CurrentFrame], TestDeferredInputColorImage() + CurrentFrame);
	BmRender_TransitionImageForRendering(RenderCommandBuffers[CurrentFrame], TestDeferredInputDepthImage() + CurrentFrame);

	BmRender_BeginRendering(RenderCommandBuffers[CurrentFrame], &RenderingInfo);
}

static void MainPassEndPass()
{
	BmRender_EndRendering(RenderCommandBuffers[CurrentFrame]);
}

void Render_Init(GLFWwindow* WindowHandle)
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

	GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);

	BmRHI_SamplerDescription ShadowMapSamplerDescription = GetShadowMapSamplerDescription();
	BmRHI_SamplerDescription DiffuseTextureSamplerDescription = GetDiffuseTextureSamplerDescription();
	BmRHI_SamplerDescription ColorAttachmentSamplerDescription = GetColorAttachmentSamplerDescription();
	BmRHI_SamplerDescription DepthAttachmentSamplerDescription = GetDepthAttachmentSamplerDescription();

	ShadowMapSampler = BmRender_CreateSampler(&ShadowMapSamplerDescription);
	DiffuseTextureSampler = BmRender_CreateSampler(&DiffuseTextureSamplerDescription);
	ColorAttachmentSampler = BmRender_CreateSampler(&ColorAttachmentSamplerDescription);
	DepthAttachmentSampler = BmRender_CreateSampler(&DepthAttachmentSamplerDescription);

	BmRender_CreateQueue(BmRender_QueueType::Graphic);
	RenderCommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);

	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); ++i)
	{
		RenderCommandBuffers[i] = BmRender_AllocateCommandBuffer(&RenderCommandPool);
		InFlightFence[i] = BmRender_CreateFence();
		ImageAvailable[i] = BmRender_CreateSemaphore();
		RenderFinished[i] = BmRender_CreateSemaphore();
	}

	DescriptorSets = DescriptorSetHandles();

	{
		FrameBufferBinding[0] = { &FrameDataBuffer, 0, sizeof(Shader_FrameData) };
		BmRender_GPUBufferUpdateData VertexBufferRegion = { &VertexBuffer, 0, VK_WHOLE_SIZE };
		BmRender_GPUBufferUpdateData InstanceBufferRegion = { &InstanceBuffer, 0, VK_WHOLE_SIZE };
		BmRender_GPUBufferUpdateData MaterialBufferRegion = { &MaterialBuffer, 0, VK_WHOLE_SIZE };

		const u32 DescriptorCount = 6;
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
		LayoutBindings[3].DescriptorType = BmRender_DescriptorType::SampledImage;
		LayoutBindings[3].StageFlags = BmRender_DescriptorShaderStage::Fragment;

		LayoutBindings[4].DescriptorCount = 1;
		LayoutBindings[4].DescriptorType = BmRender_DescriptorType::Sampler;
		LayoutBindings[4].StageFlags = BmRender_DescriptorShaderStage::Fragment;

		LayoutBindings[5].DescriptorCount = 1;
		LayoutBindings[5].DescriptorType = BmRender_DescriptorType::StorageBuffer;
		LayoutBindings[5].StageFlags = BmRender_DescriptorShaderStage::Fragment;

		const u32 UpdatesCount = 5;
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

		Updates[3].ImageBinding.Sampler = &DiffuseTextureSampler;
		Updates[3].ImageBinding.ImageLayout = BmRender_ImageLayout::Undefined;
		Updates[3].ImageBinding.ImageView = nullptr;
		Updates[3].BindingCount = 1;
		Updates[3].DstArrayElement = 0;
		Updates[3].DstBinding = 4;

		Updates[4].BufferRegions = &MaterialBufferRegion;
		Updates[4].BindingCount = 1;
		Updates[4].DstArrayElement = 0;
		Updates[4].DstBinding = 5;

		FrameDataLayout = BmRender_CreateDescriptorSetLayout(LayoutBindings, DescriptorCount);
		DescriptorSets.FrameBufferSet = BmRender_CreateDescriptorSet(&FrameDataLayout, &MainPool);
		BmRender_UpdateDescriptorSet(&DescriptorSets.FrameBufferSet, Updates, UpdatesCount);
	}

	State.DescriptorSets = DescriptorSets;
	State.MainPool = MainPool;

	DeferredPassInit(&State.MainPool);
	MainPassInit();
	LightningPassInit(&State.MainPool);

	InitStaticMeshPipeline(&State.MeshPipeline, &State.MainPool);
	InitImGuiPipeline(&State.DebugUiPool, WindowHandle);
}

void Render_DeInit()
{
	BmRender_DeviceWaitIdle();

	BmRender_DestroySampler(ShadowMapSampler);
	BmRender_DestroySampler(DiffuseTextureSampler);
	BmRender_DestroySampler(ColorAttachmentSampler);
	BmRender_DestroySampler(DepthAttachmentSampler);

	BmRender_DestroyDescriptorSetLayout(&FrameDataLayout);
	BmRender_DestroyDescriptorSetLayout(&ShadowMapArrayLayout);
	BmRender_DestroyDescriptorSetLayout(&MainPassOutputLayout);
	BmRender_DestroyDescriptorSetLayout(&LightSpaceMatrixLayout);

	DeInitImGuiPipeline(State.DebugUiPool);

	BmRender_DestroyCommandPool(RenderCommandPool);
		
	for (u32 i = 0; i < BmRender_GetSwapchainImageCount(); i++)
	{
		BmRender_DestroyImageView(State.MeshPipeline.ShadowMapArrayImageInterface[i]);
		BmRender_DestroySemaphore(ImageAvailable[i]);
		BmRender_DestroySemaphore(RenderFinished[i]);
		BmRender_DestroyFence(InFlightFence[i]);
	}
	
	DeferredPassDeInit();
	LightningPassDeInit();

	BmRender_DestroyDescriptorPool(State.MainPool);

	// Destroy GPUBuffers
	BmRender_DestroyGPUBuffer(&VertexBuffer);
	BmRender_DestroyGPUBuffer(&IndexBuffer);
	BmRender_DestroyGPUBuffer(&InstanceBuffer);
	BmRender_DestroyGPUBuffer(&MaterialBuffer);
	BmRender_DestroyGPUBuffer(&FrameDataBuffer);
}

void Render_Draw(DrawScene* Scene, u64 WaitSemaphoreValue)
{
	BmRender_WaitForFences(InFlightFence[CurrentFrame], true, UINT64_MAX);
	BmRender_ResetFences(InFlightFence[CurrentFrame]);

	RenderResources::UpdateBuffer(FrameDataBuffer, sizeof(Shader_FrameData) * CurrentFrame, &Scene->FrameDataBuffer, sizeof(Shader_FrameData));

	BmRender_AcquireNextSwapchainImage(UINT64_MAX, ImageAvailable[CurrentFrame], nullptr, &CurrentImageIndex);

	BmRender_BeginCommandBuffer(RenderCommandBuffers[CurrentFrame]);

	BmRender_RecordBindIndexBuffer(RenderCommandBuffers[CurrentFrame], &IndexBuffer, 0, BmRender_IndexType::Uint32);

	LightningPassDraw(Scene);
	MainPassBeginPass();
	DrawStaticMeshes(RenderCommandBuffers[CurrentFrame], &State.MeshPipeline, Scene, State.DescriptorSets);
	MainPassEndPass();
	
	BmRender_TransitionImageForSampling(RenderCommandBuffers[CurrentFrame], DeferredInputColorImage + CurrentFrame);
	BmRender_TransitionImageForSampling(RenderCommandBuffers[CurrentFrame], DeferredInputDepthImage + CurrentFrame);
	BmRender_TransitionImageForComputeWrite(RenderCommandBuffers[CurrentFrame], BmRender_GetSwapchainImage(CurrentImageIndex));

	DeferredPassDispatch();

	//BmRender_TransitionImageForRendering(RenderCommandBuffers[CurrentFrame], BmRender_GetSwapchainImage(CurrentImageIndex));
	//ImGui::Render();
	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), RenderCommandBuffers[CurrentFrame].InternalBuffer);


	BmRender_TransitionImageForPresentation(RenderCommandBuffers[CurrentFrame], BmRender_GetSwapchainImage(CurrentImageIndex));

	BmRender_EndCommandBuffer(RenderCommandBuffers[CurrentFrame]);

	BmRender_PipelineSyncStage WaitStages[] = {
		BmRender_PipelineSyncStage::ComputeShader,
	};

	BmRender_SubmitInfo SubmitInfo = { };
	SubmitInfo.WaitDstStageFlags = WaitStages;
	SubmitInfo.WaitSemaphores = &ImageAvailable[CurrentFrame];
	SubmitInfo.WaitSemaphoreCount = 1;
	SubmitInfo.WaitTimelineSemaphores = nullptr;
	SubmitInfo.WaitTimelineSemaphoreCount = 0;
	SubmitInfo.CommandBuffers = &RenderCommandBuffers[CurrentFrame];
	SubmitInfo.CommandBufferCount = 1;
	SubmitInfo.SignalSemaphores = &RenderFinished[CurrentFrame];
	SubmitInfo.SignalSemaphoreCount = 1;
	SubmitInfo.SignalTimelineSemaphores = nullptr;
	SubmitInfo.SignalTimelineSemaphoreCount = 0;

	BmRender_PresentInfo PresentInfo = { };
	PresentInfo.WaitSemaphores = &RenderFinished[CurrentFrame];
	PresentInfo.WaitSemaphoreCount = 1;
	PresentInfo.ImageIndices = &CurrentImageIndex;

	BmRender_QueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFence[CurrentFrame]);
	BmRender_QueuePresent(GraphicsQueue, &PresentInfo);

	CurrentFrame = Math::WrapIncrement(CurrentFrame, 3u);

	BmRender_FrameFree();
}

RenderState* GetRenderState()
{
	return &State;
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

AttachmentData* MainPassGetAttachmentData()
{
	return &MainPassPipelineAttachmentData;
}

DescriptorSetHandles* GetHandles()
{
	return &DescriptorSets;
}

BmRender_GPUBuffer* GetVertexBuffer()
{
	return &VertexBuffer;
}

BmRender_GPUBuffer* GetIndexBuffer()
{
	return &IndexBuffer;
}

BmRender_GPUBuffer* GetInstanceBuffer()
{
	return &InstanceBuffer;
}

BmRender_GPUBuffer* GetMaterialBuffer()
{
	return &MaterialBuffer;
}

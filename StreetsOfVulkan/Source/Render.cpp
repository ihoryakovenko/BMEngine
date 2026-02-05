#include "Render.h"

#include <cstddef>
#include <RenderInterface.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <Util/EngineTypes.h>
#include <SharedLib.h>

#define MAX_SWAPCHAIN_IMAGES 4

static Memory_LinearAllocator FrameMemory;

static BmRender_Shader VertexShader;
static BmRender_Shader FragmentShader;
static BmRender_DescriptorSetLayout DescriptorSetLayout;
static BmRender_DescriptorPool MaterialDescriptorPool;
static BmRender_DescriptorSet MaterialDescriptorSet;
static BmRender_GPUBuffer MaterialBuffer;
static BmRender_Image ColorImage;
static BmRender_ImageView ColorImageView;
static BmRender_Image DepthImage;
static BmRender_ImageView DepthImageView;
static BmRender_PipelineLayout PipelineLayout;
static BmRender_Pipeline Pipeline;
static BmRender_Queue GraphicsQueue;
static BmRender_CommandPool CommandPool;
static BmRender_CommandBuffer CommandBuffer;
static BmRender_Semaphore ImageAvailableSemaphore;
static BmRender_Semaphore RenderFinishedSemaphores[MAX_SWAPCHAIN_IMAGES];
static u32 SwapchainImageCount;
static BmRender_Fence InFlightFence;
static BmRender_GPUBuffer StagingBuffer;
static BmRender_Sampler AtlasSampler;

static u64 StagingBufferSize;

static bool LoadShaderFile(const char* FilePath, char** OutCode, size_t* OutCodeSize)
{
	FILE* file = fopen(FilePath, "rb");
	if (!file)
	{
		return false;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	if (fileSize < 0)
	{
		fclose(file);
		return false;
	}

	size_t codeSize = (size_t)fileSize;
	char* code = (char*)malloc(codeSize);
	if (!code)
	{
		fclose(file);
		return false;
	}

	fseek(file, 0, SEEK_SET);
	size_t readSize = fread(code, 1, codeSize, file);
	fclose(file);

	if (readSize != codeSize)
	{
		free(code);
		return false;
	}

	*OutCode = code;
	*OutCodeSize = codeSize;
	return true;
}

int StreetsRender_Init(GLFWwindow* Window, s32 WindowWidth, s32 WindowHeight)
{
	BmRender_Init(Window);

	Memory_LinearAllocator_Init(&FrameMemory, 1024 * 1024);

	char* VertexShaderCode = nullptr;
	size_t VertexShaderCodeSize = 0;
	if (!LoadShaderFile("./Map3DObject.vert.spv", &VertexShaderCode, &VertexShaderCodeSize))
	{
		return -1;
	}

	BmRender_ShaderDescription ShaderDesc = {};
	ShaderDesc.Code = reinterpret_cast<const u32*>(VertexShaderCode);
	ShaderDesc.CodeSize = VertexShaderCodeSize;
	ShaderDesc.Stage = BmRender_PipelineShaderStage::Vertex;
	VertexShader = BmRender_CreateShader(&ShaderDesc);

	free(VertexShaderCode);

	char* FragmentShaderCode = nullptr;
	size_t FragmentShaderCodeSize = 0;
	if (!LoadShaderFile("./Map3DObject.frag.spv", &FragmentShaderCode, &FragmentShaderCodeSize))
	{
		return -1;
	}

	BmRender_ShaderDescription FragShaderDesc = {};
	FragShaderDesc.Code = (const u32*)FragmentShaderCode;
	FragShaderDesc.CodeSize = FragmentShaderCodeSize;
	FragShaderDesc.Stage = BmRender_PipelineShaderStage::Fragment;
	FragmentShader = BmRender_CreateShader(&FragShaderDesc);

	free(FragmentShaderCode);

	BmRender_DescriptorSetLayoutBinding DescriptorBinding = {};
	DescriptorBinding.DescriptorType = BmRender_DescriptorType::StorageBuffer;
	DescriptorBinding.DescriptorCount = 1;
	DescriptorBinding.StageFlags = BmRender_DescriptorShaderStage::Fragment;
	DescriptorSetLayout = BmRender_CreateDescriptorSetLayout(&DescriptorBinding, 1);

	// Push constants: mat4 vp (64) + ivec2 (8) + float (4) + float (4) + vec2 (8) + int debugMode (4) = 92	constexpr u32 PUSH_CONSTANT_SIZE = sizeof(StreetsRender_FrameData);
	BmRender_PushConstant PushConstantRange = BmRender_CreatePushConstant(
		(BmRender_DescriptorShaderStage)((u64)BmRender_DescriptorShaderStage::Vertex | (u64)BmRender_DescriptorShaderStage::Fragment), 0, sizeof(StreetsRender_FrameData));

	BmRender_Extent2D SwapchainExtent = BmRender_GetSwapchainExtent();

	ColorImage = BmRender_CreateImage2D(SwapchainExtent.Width, SwapchainExtent.Height, BmRender_Format::R8G8B8A8_UNORM, BmRender_ImageType::MultiSampledColorAttachment, BmRender_SampleCount::Count4);
	ColorImageView = BmRender_CreateImageView2D(ColorImage);

	DepthImage = BmRender_CreateImage2D(SwapchainExtent.Width, SwapchainExtent.Height, BmRender_Format::D32_SFLOAT_S8_UINT, BmRender_ImageType::MultiSampledDepthAttachment, BmRender_SampleCount::Count4);
	DepthImageView = BmRender_CreateImageView2D(DepthImage);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 1;
	LayoutDesc.SetLayouts = &DescriptorSetLayout;
	LayoutDesc.PushConstantRangeCount = 1;
	LayoutDesc.PushConstantRanges = &PushConstantRange;
	LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;
	PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);

	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	BmRender_ImageView PipelineColorAttachments[1];
	PipelineColorAttachments[0] = ColorImageView;

	PipelineDesc.Attachment.ColorAttachmentCount = 1;
	PipelineDesc.Attachment.ColorAttachments = PipelineColorAttachments;
	PipelineDesc.Attachment.DepthAttachment = DepthImageView;
	PipelineDesc.Attachment.StencilAttachment = nullptr;

	BmRender_ShaderStageDescription ShaderStages[2] = {};
	ShaderStages[0].Shader = VertexShader;
	ShaderStages[0].EntryPointFunction = "main";
	ShaderStages[1].Shader = FragmentShader;
	ShaderStages[1].EntryPointFunction = "main";
	PipelineDesc.ShaderStages = ShaderStages;
	PipelineDesc.ShaderStagesCount = 2;

	VertexAttribute NanodegAttr = {};
	NanodegAttr.Type = BmRender_AttributeType::Ivec2;
	NanodegAttr.Offset = 0;

	VertexAttribute AltitudeAttr = {};
	AltitudeAttr.Type = BmRender_AttributeType::Float;
	AltitudeAttr.Offset = offsetof(StreetsRender_BuildingVertex, AltitudeMeters);

	VertexAttribute ColorAttr = {};
	ColorAttr.Type = BmRender_AttributeType::Vec3;
	ColorAttr.Offset = offsetof(StreetsRender_BuildingVertex, Color);

	VertexAttribute NormalAttr = {};
	NormalAttr.Type = BmRender_AttributeType::Vec3;
	NormalAttr.Offset = offsetof(StreetsRender_BuildingVertex, Normal);

	VertexAttribute VertexAttributes[4] = { NanodegAttr, AltitudeAttr, ColorAttr, NormalAttr };

	BmRender_VertexBinding VertexBinding = {};
	VertexBinding.Attributes = VertexAttributes;
	VertexBinding.AttributesCount = 4;
	VertexBinding.Stride = sizeof(StreetsRender_BuildingVertex);
	VertexBinding.InputRate = BmRender_VertexInputRate::Vertex;

	VertexAttribute MaterialIndexAttr = {};
	MaterialIndexAttr.Type = BmRender_AttributeType::Uint;
	MaterialIndexAttr.Offset = offsetof(StreetsRender_3DObjectInstance, MaterialIndex);

	BmRender_VertexBinding InstanceBinding = {};
	InstanceBinding.Attributes = &MaterialIndexAttr;
	InstanceBinding.AttributesCount = 1;
	InstanceBinding.Stride = sizeof(StreetsRender_3DObjectInstance);
	InstanceBinding.InputRate = BmRender_VertexInputRate::Instance;

	BmRender_VertexBinding Bindings[2] = { VertexBinding, InstanceBinding };
	PipelineDesc.VertexBindings = Bindings;
	PipelineDesc.VertexBindingsCount = 2;

	PipelineDesc.DescriptorSetLayouts = &DescriptorSetLayout;
	PipelineDesc.DescriptorSetLayoutsCount = 1;
	PipelineDesc.PushConstantRanges = &PushConstantRange;
	PipelineDesc.PushConstantRangesCount = 1;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.DepthClampEnable = false;
	PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.LineWidth = 1.0f;
	PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::Back;
	PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.DepthBiasEnable = false;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.BlendEnable = false;
	PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.LogicOpEnable = false;
	PipelineDesc.ColorBlendState.AttachmentCount = 1;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.DepthTestEnable = true;
	PipelineDesc.DepthStencilState.DepthWriteEnable = true;
	PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.StencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.SampleShadingEnable = false;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

	PipelineDesc.Extent = { (u32)WindowWidth, (u32)WindowHeight };
	BmRender_Viewport Viewport = {};
	Viewport.X = 0.0f;
	Viewport.Y = 0.0f;
	Viewport.Width = (f32)WindowWidth;
	Viewport.Height = (f32)WindowHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	PipelineDesc.Viewport = Viewport;

	BmRender_Rect2D Scissor = {};
	Scissor.Offset = { 0, 0 };
	Scissor.Extent = { (u32)WindowWidth, (u32)WindowHeight };
	PipelineDesc.Scissor = Scissor;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.ViewportCount = 1;
	PipelineDesc.ViewportState.ScissorCount = 1;

	StagingBufferSize = MB256;
	StagingBuffer = BmRender_CreateStagingBuffer(StagingBufferSize);

	Pipeline = BmRender_CreatePipeline(&PipelineDesc);
	GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);
	CommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);
	CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);
	ImageAvailableSemaphore = BmRender_CreateSemaphore();
	SwapchainImageCount = BmRender_GetSwapchainImageCount();
	for (u32 i = 0; i < SwapchainImageCount; ++i)
		RenderFinishedSemaphores[i] = BmRender_CreateSemaphore();
	InFlightFence = BmRender_CreateFence();
}

void StreetsRender_CreateMaterials(StreetsRender_Material* Materials, u32 MaterialsCount)
{
	const u64 MaterialBufferSize = sizeof(StreetsRender_Material) * MaterialsCount;
	MaterialBuffer = BmRender_CreateStorageBuffer(MaterialBufferSize, MemoryPropertyFlag::HostCompatible);
	BmRender_UpdateHostCompatibleBuffer(MaterialBuffer, 0, MaterialBufferSize, Materials);

	BmRender_DescriptorPoolSize PoolSize = {};
	PoolSize.Type = BmRender_DescriptorType::StorageBuffer;
	PoolSize.DescriptorCount = 1;
	MaterialDescriptorPool = BmRender_CreateDescriptorPool(&PoolSize, 1, 1, BmRender_DescriptorPoolType::None);

	BmRender_GPUBufferBinding MaterialRegion = {};
	MaterialRegion.GPUBufferHandle = MaterialBuffer;
	MaterialRegion.BufferOffset = 0;
	MaterialRegion.Size = MaterialBufferSize;

	BmRender_DescriptorSetBinding MaterialBinding = {};
	MaterialBinding.BufferRegions = &MaterialRegion;
	MaterialBinding.BindingCount = 1;
	MaterialBinding.DstArrayElement = 0;

	MaterialDescriptorSet = BmRender_CreateDescriptorSet(DescriptorSetLayout, MaterialDescriptorPool);
	BmRender_UpdateDescriptorSet(MaterialDescriptorSet, &MaterialBinding, 1);
}

void StreetsRender_DestroyMaterials()
{
	BmRender_DestroyDescriptorPool(MaterialDescriptorPool);
	BmRender_DestroyGPUBuffer(MaterialBuffer);
}

void StreetsRender_Draw(StreetsRender_FrameData* FrameData, StreetsRender_3DObjectsTile* Meshes, u32 MeshCount)
{
	BmRender_WaitForFences(InFlightFence, true, UINT64_MAX);
	BmRender_ResetFences(InFlightFence);

	u32 ImageIndex;
	BmRender_SwapchainResult AcquireResult = BmRender_AcquireNextSwapchainImage(UINT64_MAX, ImageAvailableSemaphore, nullptr, &ImageIndex);

	if (AcquireResult == BmRender_SwapchainResult::OutOfDate || AcquireResult == BmRender_SwapchainResult::Suboptimal)
	{
		return;
	}

	BmRender_BeginCommandBuffer(CommandBuffer);
	BmRender_TransitionImageForRendering(CommandBuffer, ColorImage);
	BmRender_TransitionImageForRendering(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));
	BmRender_TransitionImageForRendering(CommandBuffer, DepthImage);

	BmRender_RenderingColorAttachment ColorAttachment = {};
	ColorAttachment.ImageView = ColorImageView;
	ColorAttachment.ResolveImageView = BmRender_GetSwapchainImageView(ImageIndex);
	ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
	ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
	ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	BmRender_RenderingDepthAttachment DepthAttachment = {};
	DepthAttachment.ImageView = DepthImageView;
	DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
	DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
	DepthAttachment.ClearValue = { 1.0f, 0 };  // Clear to max depth

	BmRender_RenderingInfo RenderingInfo = {};
	RenderingInfo.Offset = { 0, 0 };
	RenderingInfo.Extent = BmRender_GetSwapchainExtent();
	RenderingInfo.ColorAttachments = &ColorAttachment;
	RenderingInfo.ColorAttachmentCount = 1;
	RenderingInfo.DepthAttachment = &DepthAttachment;

	BmRender_BeginRendering(CommandBuffer, &RenderingInfo);

	BmRender_BindPipeline(CommandBuffer, Pipeline);
	BmRender_RecordPushConstants(CommandBuffer, PipelineLayout, (BmRender_DescriptorShaderStage)((u64)BmRender_DescriptorShaderStage::Vertex | (u64)BmRender_DescriptorShaderStage::Fragment), 0, sizeof(StreetsRender_FrameData), FrameData);
	BmRender_RecordBindDescriptorSets(CommandBuffer, PipelineLayout, 0, 1, &MaterialDescriptorSet, 0, nullptr);

	for (u32 i = 0; i < MeshCount; ++i)
	{
		BmRender_GPUBuffer VertexBuffers[2] = { Meshes[i].VertexBuffer, Meshes[i].InstanceBuffer };
		u64 VertexOffsets[2] = { 0, 0 };
		BmRender_RecordBindVertexBuffers(CommandBuffer, 0, 2, VertexBuffers, VertexOffsets);
		BmRender_RecordBindIndexBuffer(CommandBuffer, Meshes[i].IndexBuffer, 0, BmRender_IndexType::Uint32);

		BmRender_RecordDrawIndexedIndirect(CommandBuffer, Meshes[i].IndirectBuffer, 0, Meshes[i].CommandCount, sizeof(BmRender_DrawIndexedIndirectCommand));
	}

	BmRender_EndRendering(CommandBuffer);

	BmRender_TransitionImageForPresentation(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));

	BmRender_EndCommandBuffer(CommandBuffer);

	BmRender_PipelineSyncStage WaitStages[] = { BmRender_PipelineSyncStage::ColorAttachmentOutput };

	BmRender_SubmitInfo SubmitInfo = {};
	SubmitInfo.WaitDstStageFlags = WaitStages;
	SubmitInfo.WaitSemaphores = &ImageAvailableSemaphore;
	SubmitInfo.WaitSemaphoreCount = 1;
	SubmitInfo.SignalSemaphores = &RenderFinishedSemaphores[ImageIndex];
	SubmitInfo.SignalSemaphoreCount = 1;
	SubmitInfo.CommandBuffers = &CommandBuffer;
	SubmitInfo.CommandBufferCount = 1;
	SubmitInfo.WaitTimelineSemaphores = nullptr;
	SubmitInfo.WaitTimelineSemaphoreCount = 0;
	SubmitInfo.SignalTimelineSemaphores = nullptr;
	SubmitInfo.SignalTimelineSemaphoreCount = 0;

	BmRender_QueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFence);

	BmRender_PresentInfo PresentInfo = {};
	PresentInfo.WaitSemaphores = &RenderFinishedSemaphores[ImageIndex];
	PresentInfo.WaitSemaphoreCount = 1;
	PresentInfo.ImageIndices = &ImageIndex;

	BmRender_SwapchainResult PresentResult = BmRender_QueuePresent(GraphicsQueue, &PresentInfo);

	BmRender_FrameFree();
}

StreetsRender_3DObjectsTile StreetsRender_Create3DObjectsTile(StreetsRender_3DObjectsTileCreateData* TileData)
{
	const u64 VertexBufferSize = sizeof(StreetsRender_BuildingVertex) * TileData->VertexCount;
	const u64 IndexBufferSize = sizeof(u32) * TileData->IndexCount;
	const u64 IndirectBufferSize = sizeof(BmRender_DrawIndexedIndirectCommand) * TileData->RangesCount;
	const u64 InstanceBufferSize = sizeof(StreetsRender_3DObjectInstance) * TileData->RangesCount;

	assert(StagingBufferSize > VertexBufferSize + IndexBufferSize + IndirectBufferSize + InstanceBufferSize);

	BmRender_DrawIndexedIndirectCommand* IndirectCommands = (BmRender_DrawIndexedIndirectCommand*)Memory_LinearAllocator_Alloc(&FrameMemory, IndirectBufferSize);
	for (u32 IndirectCommandIndex = 0; IndirectCommandIndex < TileData->RangesCount; ++IndirectCommandIndex)
	{
		BmRender_DrawIndexedIndirectCommand* Command = IndirectCommands + IndirectCommandIndex;
		StreetsRender_3DObjectRange* Range = TileData->Ranges + IndirectCommandIndex;

		Command->FirstIndex = Range->FirstIndex;
		Command->IndexCount = Range->IndexCount;
		Command->VertexOffset = 0;
		Command->FirstInstance = IndirectCommandIndex;
		Command->InstanceCount = 1;
	}

	StreetsRender_3DObjectsTile Mesh;
	Mesh.VertexBuffer = BmRender_CreateVertexStageBuffer(VertexBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.IndexBuffer = BmRender_CreateVertexStageBuffer(IndexBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.InstanceBuffer = BmRender_CreateInstanceBuffer(InstanceBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.IndirectBuffer = BmRender_CreateIndirectDrawBuffer(IndirectBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.VertexCount = TileData->VertexCount;
	Mesh.IndexCount = TileData->IndexCount;
	Mesh.CommandCount = TileData->RangesCount;

	const u64 StagingInstanceOffset = VertexBufferSize + IndexBufferSize + IndirectBufferSize;
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, 0, VertexBufferSize, TileData->Vertices);
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, VertexBufferSize, IndexBufferSize, TileData->Indices);
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, VertexBufferSize + IndexBufferSize, IndirectBufferSize, IndirectCommands);
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, StagingInstanceOffset, InstanceBufferSize, TileData->Instances);

	BmRender_BeginCommandBuffer(CommandBuffer);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.VertexBuffer, StagingBuffer, 0, 0, VertexBufferSize);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.IndexBuffer, StagingBuffer, VertexBufferSize, 0, IndexBufferSize);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.IndirectBuffer, StagingBuffer, VertexBufferSize + IndexBufferSize, 0, IndirectBufferSize);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.InstanceBuffer, StagingBuffer, StagingInstanceOffset, 0, InstanceBufferSize);
	BmRender_EndCommandBuffer(CommandBuffer);

	BmRender_SubmitInfo TransferSubmitInfo = {};
	TransferSubmitInfo.CommandBuffers = &CommandBuffer;
	TransferSubmitInfo.CommandBufferCount = 1;
	TransferSubmitInfo.WaitDstStageFlags = nullptr;
	TransferSubmitInfo.WaitSemaphores = nullptr;
	TransferSubmitInfo.WaitSemaphoreCount = 0;
	TransferSubmitInfo.SignalSemaphores = nullptr;
	TransferSubmitInfo.SignalSemaphoreCount = 0;
	TransferSubmitInfo.WaitTimelineSemaphores = nullptr;
	TransferSubmitInfo.WaitTimelineSemaphoreCount = 0;
	TransferSubmitInfo.SignalTimelineSemaphores = nullptr;
	TransferSubmitInfo.SignalTimelineSemaphoreCount = 0;

	BmRender_QueueSubmit(GraphicsQueue, 1, &TransferSubmitInfo, nullptr);
	BmRender_DeviceWaitIdle();

	return Mesh;
}

void StreetsRender_Destroy3DObjectsTile(StreetsRender_3DObjectsTile* Mesh)
{
	BmRender_QueueWaitIdle(GraphicsQueue);
	BmRender_DestroyGPUBuffer(Mesh->VertexBuffer);
	BmRender_DestroyGPUBuffer(Mesh->IndexBuffer);
	BmRender_DestroyGPUBuffer(Mesh->InstanceBuffer);
	BmRender_DestroyGPUBuffer(Mesh->IndirectBuffer);
}

void StreetsRender_DeInit()
{
	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyImageView(DepthImageView);
	BmRender_DestroyImage(DepthImage);
	BmRender_DestroyImage(ColorImage);
	BmRender_DestroyGPUBuffer(StagingBuffer);
	// BmRender_DestroyGPUBuffer(IndirectBuffer);
	StreetsRender_DestroyMaterials();
	BmRender_DestroyDescriptorSetLayout(DescriptorSetLayout);
	BmRender_DestroyShader(VertexShader);
	BmRender_DestroyShader(FragmentShader);
	BmRender_DestroySemaphore(ImageAvailableSemaphore);
	for (u32 i = 0; i < SwapchainImageCount; ++i)
		BmRender_DestroySemaphore(RenderFinishedSemaphores[i]);
	BmRender_DestroyCommandPool(CommandPool);
	BmRender_DestroyFence(InFlightFence);
	BmRender_DestroyPipeline(Pipeline);
	BmRender_DestroyPipelineLayout(PipelineLayout);

	Memory_LinearAllocator_Free(&FrameMemory);

	BmRender_DeInit();
}

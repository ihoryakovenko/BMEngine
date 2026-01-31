#include "Render.h"

#include <RenderInterface.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <Util/EngineTypes.h>

#define MAX_SWAPCHAIN_IMAGES 4

static BmRender_Shader VertexShader;
static BmRender_Shader FragmentShader;
static BmRender_DescriptorSetLayout DescriptorSetLayout;
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
static BmRender_GPUBuffer IndirectBuffer;
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

	char* VertexShaderCode = nullptr;
	size_t VertexShaderCodeSize = 0;
	if (!LoadShaderFile("./test_vertex.vert.spv", &VertexShaderCode, &VertexShaderCodeSize))
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
	if (!LoadShaderFile("./test_fragment.frag.spv", &FragmentShaderCode, &FragmentShaderCodeSize))
	{
		return -1;
	}

	BmRender_ShaderDescription FragShaderDesc = {};
	FragShaderDesc.Code = reinterpret_cast<const u32*>(FragmentShaderCode);
	FragShaderDesc.CodeSize = FragmentShaderCodeSize;
	FragShaderDesc.Stage = BmRender_PipelineShaderStage::Fragment;
	FragmentShader = BmRender_CreateShader(&FragShaderDesc);

	free(FragmentShaderCode);

	BmRender_DescriptorSetLayoutBinding DescriptorBinding = {};
	DescriptorBinding.DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
	DescriptorBinding.DescriptorCount = 1;
	DescriptorBinding.StageFlags = BmRender_DescriptorShaderStage::Fragment;
	DescriptorSetLayout = BmRender_CreateDescriptorSetLayout(&DescriptorBinding, 1);

	BmRender_PushConstant PushConstantRange = BmRender_CreatePushConstant(BmRender_DescriptorShaderStage::Vertex, 0, sizeof(glm::mat4));

	BmRender_Extent2D SwapchainExtent = BmRender_GetSwapchainExtent();
	DepthImage = BmRender_CreateImage2D(SwapchainExtent.Width, SwapchainExtent.Height, BmRender_Format::D32_SFLOAT_S8_UINT, BmRender_ImageType::DepthSamplad);
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
	PipelineColorAttachments[0] = BmRender_GetSwapchainImageView(0);

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

	// Define vertex bindings
	VertexAttribute PositionAttribute = {};
	PositionAttribute.Type = BmRender_AttributeType::Vec3;
	PositionAttribute.Offset = 0;

	VertexAttribute VertexAttributes[1] = { PositionAttribute };

	BmRender_VertexBinding VertexBinding = {};
	VertexBinding.Attributes = VertexAttributes;
	VertexBinding.AttributesCount = 1;
	VertexBinding.Stride = sizeof(StreetsRender_Vertex);
	VertexBinding.InputRate = BmRender_VertexInputRate::Vertex;

	PipelineDesc.VertexBindings = &VertexBinding;
	PipelineDesc.VertexBindingsCount = 1;

	PipelineDesc.DescriptorSetLayouts = &DescriptorSetLayout;
	PipelineDesc.DescriptorSetLayoutsCount = 1;
	PipelineDesc.PushConstantRanges = &PushConstantRange;
	PipelineDesc.PushConstantRangesCount = 1;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.depthClampEnable = false;
	PipelineDesc.RasterizationState.rasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.polygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.lineWidth = 1.0f;
	PipelineDesc.RasterizationState.cullMode = BmRender_CullModeFlags::Back;
	PipelineDesc.RasterizationState.frontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.depthBiasEnable = false;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.colorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.blendEnable = false;
	PipelineDesc.ColorBlendAttachment.srcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.dstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.colorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.srcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.dstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.alphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.logicOpEnable = false;
	PipelineDesc.ColorBlendState.attachmentCount = 1;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.depthTestEnable = true;
	PipelineDesc.DepthStencilState.depthWriteEnable = true;
	PipelineDesc.DepthStencilState.depthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.depthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.stencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.sampleShadingEnable = false;
	PipelineDesc.MultisampleState.rasterizationSamples = BmRender_SampleCount::Count1;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.primitiveRestartEnable = false;

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
	PipelineDesc.ViewportState.viewportCount = 1;
	PipelineDesc.ViewportState.scissorCount = 1;

	// BmRender_DrawIndexedIndirectCommand IndirectCommand = {};
	// IndirectCommand.IndexCount = 36;
	// IndirectCommand.InstanceCount = 1;

	// const u64 IndexBufferSize = sizeof(CubeIndices);
	// const u64 IndirectCommandSize = sizeof(IndirectCommand);

	StagingBufferSize = MB256;
	StagingBuffer = BmRender_CreateStagingBuffer(StagingBufferSize); // + IndexBufferSize + IndirectCommandSize);
	// IndirectBuffer = BmRender_CreateIndirectDrawBuffer(IndirectCommandSize, MemoryPropertyFlag::GPULocal);

	Pipeline = BmRender_CreatePipeline(&PipelineDesc);
	GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);
	CommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);
	CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);
	ImageAvailableSemaphore = BmRender_CreateSemaphore();
	SwapchainImageCount = BmRender_GetSwapchainImageCount();
	for (u32 i = 0; i < SwapchainImageCount; ++i)
		RenderFinishedSemaphores[i] = BmRender_CreateSemaphore();
	InFlightFence = BmRender_CreateFence();

	BmRHI_SamplerDescription SamplerDesc = {};
	SamplerDesc.MagFilter = BmRender_Filter::Linear;
	SamplerDesc.MinFilter = BmRender_Filter::Linear;
	SamplerDesc.MipmapMode = BmRender_SamplerMipmapMode::Linear;
	SamplerDesc.AddressModeU = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.AddressModeV = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.AddressModeW = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.MipLodBias = 0.0f;
	SamplerDesc.AnisotropyEnable = false;
	SamplerDesc.MaxAnisotropy = 1.0f;
	SamplerDesc.CompareEnable = false;
	SamplerDesc.CompareOp = BmRender_CompareOp::Always;
	SamplerDesc.MinLod = 0.0f;
	SamplerDesc.MaxLod = 0.0f;
	SamplerDesc.BorderColor = BmRender_BorderColor::FloatOpaqueBlack;
	SamplerDesc.UnnormalizedCoordinates = false;
	AtlasSampler = BmRender_CreateSampler(&SamplerDesc);

	//BmRender_UpdateHostCompatibleBuffer(StagingBuffer, 0, VertexBufferSize, CubeOutlineVertices);
	// BmRender_UpdateHostCompatibleBuffer(StagingBuffer, VertexBufferSize, IndexBufferSize, CubeIndices);
	// BmRender_UpdateHostCompatibleBuffer(StagingBuffer, VertexBufferSize + IndexBufferSize, IndirectCommandSize, &IndirectCommand);

	//BmRender_BeginCommandBuffer(CommandBuffer);
	// BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, IndexBuffer, StagingBuffer, VertexBufferSize, 0, IndexBufferSize);
	// BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, IndirectBuffer, StagingBuffer, VertexBufferSize + IndexBufferSize, 0, IndirectCommandSize);
	//BmRender_EndCommandBuffer(CommandBuffer);
}

void StreetsRender_Draw(glm::mat4 vp, StreetsRender_Mesh* Meshes, u32 MeshCount)
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
	BmRender_TransitionImageForRendering(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));
	BmRender_TransitionImageForRendering(CommandBuffer, DepthImage);

	BmRender_RenderingColorAttachment ColorAttachment = {};
	ColorAttachment.ImageView = BmRender_GetSwapchainImageView(ImageIndex);
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
	BmRender_RecordPushConstants(CommandBuffer, PipelineLayout, BmRender_DescriptorShaderStage::Vertex, 0, 64, glm::value_ptr(vp));

	for (u32 i = 0; i < MeshCount; ++i)
	{
		u64 vertexOffset = 0;
		BmRender_RecordBindVertexBuffers(CommandBuffer, 0, 1, &Meshes[i].VertexBuffer, &vertexOffset);
		BmRender_RecordBindIndexBuffer(CommandBuffer, Meshes[i].IndexBuffer, 0, BmRender_IndexType::Uint32);


		BmRender_DrawIndexed(CommandBuffer, Meshes[i].IndexCount, 1, 0, 0, 0);
		//BmRender_Draw(CommandBuffer, Meshes[i].VertexCount, 1, 0, 0); // cube outline: 24 vertices (12 lines)
		// BmRender_RecordDrawIndexedIndirect(CommandBuffer, IndirectBuffer, 0, 1, sizeof(BmRender_DrawIndexedIndirectCommand));
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

StreetsRender_Mesh StreetsRender_CreateMesh(StreetsRender_Vertex* Vertices, u32 VertexCount, u32* Indices, u32 IndexCount)
{
	const u64 VertexBufferSize = sizeof(StreetsRender_Vertex) * VertexCount;
	const u64 IndexBufferSize = sizeof(u32) * IndexCount;

	assert(StagingBufferSize > VertexBufferSize + IndexBufferSize);

	StreetsRender_Mesh Mesh;
	Mesh.VertexBuffer = BmRender_CreateVertexStageBuffer(VertexBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.IndexBuffer = BmRender_CreateVertexStageBuffer(IndexBufferSize, MemoryPropertyFlag::GPULocal);
	Mesh.VertexCount = VertexCount;
	Mesh.IndexCount = IndexCount;

	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, 0, VertexBufferSize, Vertices);
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, VertexBufferSize, IndexBufferSize, Indices);

	BmRender_BeginCommandBuffer(CommandBuffer);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.VertexBuffer, StagingBuffer, 0, 0, VertexBufferSize);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, Mesh.IndexBuffer, StagingBuffer, VertexBufferSize, 0, IndexBufferSize);
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

void StreetsRender_DestroyMesh(StreetsRender_Mesh* Mesh)
{
	BmRender_QueueWaitIdle(GraphicsQueue);
	BmRender_DestroyGPUBuffer(Mesh->VertexBuffer);
	BmRender_DestroyGPUBuffer(Mesh->IndexBuffer);
}

void StreetsRender_DeInit()
{
	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyImageView(DepthImageView);
	BmRender_DestroyImage(DepthImage);
	BmRender_DestroyGPUBuffer(StagingBuffer);
	// BmRender_DestroyGPUBuffer(IndexBuffer);
	// BmRender_DestroyGPUBuffer(IndirectBuffer);
	BmRender_DestroySampler(AtlasSampler);
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

	BmRender_DeInit();
}

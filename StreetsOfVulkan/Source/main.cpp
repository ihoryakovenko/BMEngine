#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdio>
#include <cstdlib>

#include <RenderInterface.h>

#include <ShortTypes.h>

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

#include <cstdint>
#include <array>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>

using Tile8x8 = std::array<uint8_t, 64>;

Tile8x8 DecodeGenesisTile4BPP(const uint8_t* d)
{
	Tile8x8 out{ };
	const uint8_t* p0 = d + 0, * p1 = d + 8, * p2 = d + 16, * p3 = d + 24;

	for (int y = 0; y < 8; y++)
	{
		uint8_t b0 = p0[y], b1 = p1[y], b2 = p2[y], b3 = p3[y];
		for (int x = 0; x < 8; x++)
		{
			int bit = 7 - x;
			uint8_t idx =
				((b0 >> bit) & 1) |
				(((b1 >> bit) & 1) << 1) |
				(((b2 >> bit) & 1) << 2) |
				(((b3 >> bit) & 1) << 3);
			out[y * 8 + x] = idx;
		}
	}
	return out;
}

inline uint8_t Expand3To8(int v) { return (v << 5) | (v << 2) | (v >> 1); }

struct RGBA { uint8_t r, g, b, a; };

RGBA DecodeGenesisColor(uint16_t c)
{
	int r = (c & 0x00E) >> 1;
	int g = (c & 0x0E0) >> 5;
	int b = (c & 0xE00) >> 9;
	return { Expand3To8(r), Expand3To8(g), Expand3To8(b), 255 };
}

std::array<std::array<RGBA, 16>, 4> BuildPalettes(const uint16_t* cram)
{
	std::array<std::array<RGBA, 16>, 4> p{ };
	for (int pal = 0; pal < 4; pal++)
	{
		for (int i = 0; i < 16; i++)
			p[pal][i] = DecodeGenesisColor(cram[pal * 16 + i]);

		p[pal][0].a = 0;
	}
	return p;
}

std::array<RGBA, 64> ApplyPalette(
	const Tile8x8& t,
	const std::array<std::array<RGBA, 16>, 4>& pal,
	int pi)
{
	std::array<RGBA, 64> out{ };
	const auto& p = pal[pi];
	for (int i = 0; i < 64; i++) out[i] = p[t[i]];
	return out;
}

constexpr int TILE_COUNT = 2048;
constexpr int TILE_SIZE = 32;
constexpr int GRID_W = 64;
constexpr int GRID_H = 32;
constexpr int ATLAS_W = GRID_W * 8;
constexpr int ATLAS_H = GRID_H * 8;

std::vector<RGBA> BuildVRAMAtlas(
	const uint8_t* vram,
	const uint16_t* cram)
{
	std::vector<RGBA> atlas(ATLAS_W * ATLAS_H);
	auto palettes = BuildPalettes(cram);

	for (int tile = 0; tile < TILE_COUNT; tile++)
	{
		const uint8_t* tdata = vram + tile * TILE_SIZE;
		Tile8x8 t = DecodeGenesisTile4BPP(tdata);
		auto rgba = ApplyPalette(t, palettes, 0);

		int tx = tile % GRID_W;
		int ty = tile / GRID_W;

		for (int y = 0; y < 8; y++)
			for (int x = 0; x < 8; x++)
			{
				int dx = tx * 8 + x;
				int dy = ty * 8 + y;
				atlas[dy * ATLAS_W + dx] = rgba[y * 8 + x];
			}
	}

	return atlas;
}

u64 ReadFile(FILE* File, void** OutData)
{
	fseek(File, 0, SEEK_END);
	const u32 FileSize = ftell(File);
	if (FileSize < 0)
	{
		return 0;
	}

	*OutData = (char*)malloc(FileSize);

	fseek(File, 0, SEEK_SET);
	return fread(*OutData, 1, FileSize, File);
}

int main()
{
	s32 WindowWidth = 1920;
	s32 WindowHeight = 1080;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);

	BmRender_Init(Window, 1);

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
	BmRender_Shader VertexShader = BmRender_CreateShader(&ShaderDesc);

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
	BmRender_Shader FragmentShader = BmRender_CreateShader(&FragShaderDesc);

	free(FragmentShaderCode);

	BmRender_DescriptorSetLayoutBinding DescriptorBinding = {};
	DescriptorBinding.DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
	DescriptorBinding.DescriptorCount = 1;
	DescriptorBinding.StageFlags = BmRender_DescriptorShaderStage::Fragment;
	BmRender_DescriptorSetLayout DescriptorSetLayout = BmRender_CreateDescriptorSetLayout(&DescriptorBinding, 1);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 1;
	LayoutDesc.SetLayouts = &DescriptorSetLayout;
	LayoutDesc.PushConstantRangeCount = 0;
	LayoutDesc.PushConstantRanges = nullptr;
	LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;
	BmRender_PipelineLayout PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);
	
	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 1;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = BmRender_GetSurfaceFormat().Format;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = BmRender_Format::Undefined;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = BmRender_Format::Undefined;

	BmRender_ShaderStageDescription ShaderStages[2] = {};
	ShaderStages[0].Shader = VertexShader;
	ShaderStages[0].EntryPointFunction = "main";
	ShaderStages[1].Shader = FragmentShader;
	ShaderStages[1].EntryPointFunction = "main";
	PipelineDesc.ShaderStages = ShaderStages;
	PipelineDesc.ShaderStagesCount = 2;

	PipelineDesc.VertexBindings = nullptr;
	PipelineDesc.VertexBindingsCount = 0;

	PipelineDesc.DescriptorSetLayouts = &DescriptorSetLayout;
	PipelineDesc.DescriptorSetLayoutsCount = 1;
	PipelineDesc.PushConstantRanges = nullptr;
	PipelineDesc.PushConstantRangesCount = 0;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.depthClampEnable = false;
	PipelineDesc.RasterizationState.rasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.polygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.lineWidth = 1.0f;
	PipelineDesc.RasterizationState.cullMode = BmRender_CullModeFlags::None;
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
	PipelineDesc.DepthStencilState.depthTestEnable = false;
	PipelineDesc.DepthStencilState.depthWriteEnable = false;
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

	BmRender_Pipeline Pipeline = BmRender_CreatePipeline(&PipelineDesc);

	BmRender_Queue GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);

	BmRender_CommandPool CommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);
	BmRender_CommandBuffer CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);

	BmRender_Semaphore ImageAvailableSemaphore = BmRender_CreateSemaphore();
	BmRender_Semaphore RenderFinishedSemaphore = BmRender_CreateSemaphore();
	BmRender_Fence InFlightFence = BmRender_CreateFence();

	u16* CRAMData = nullptr;
	FILE* CRAM = fopen("./CRAM.ram", "rb");
	if (!CRAM)
	{
		return 0;
	}

	u8* VRAMData = nullptr;
	FILE* VRAM = fopen("./VRAM.ram", "rb");
	if (!VRAM)
	{
		return 0;
	}

	ReadFile(CRAM, (void**)&CRAMData);
	ReadFile(VRAM, (void**)&VRAMData);

	fclose(CRAM);
	fclose(VRAM);

	std::vector<RGBA>VRAMAtlasData = BuildVRAMAtlas(VRAMData, CRAMData);

	BmRender_Image VRAMAtlasImage = BmRender_CreateImage2D(ATLAS_W, ATLAS_H, BmRender_Format::R8G8B8A8_UNORM, BmRender_ImageType::TransferSampled);
	BmRender_ImageView VRAMAtlasImageView = BmRender_CreateImageView2D(VRAMAtlasImage);

	u64 AtlasDataSize = VRAMAtlasData.size() * sizeof(RGBA);
	BmRender_GPUBuffer StagingBuffer = BmRender_CreateStagingBuffer(AtlasDataSize);
	
	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, 0, AtlasDataSize, VRAMAtlasData.data());

	BmRender_BeginCommandBuffer(CommandBuffer);
	
	VkImageMemoryBarrier2 TransferImageBarrier = {};
	TransferImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	TransferImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	TransferImageBarrier.srcAccessMask = 0;
	TransferImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	TransferImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	TransferImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	TransferImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	TransferImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransferImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransferImageBarrier.image = (VkImage)VRAMAtlasImage;
	TransferImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	TransferImageBarrier.subresourceRange.baseMipLevel = 0;
	TransferImageBarrier.subresourceRange.levelCount = 1;
	TransferImageBarrier.subresourceRange.baseArrayLayer = 0;
	TransferImageBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo TransferDepInfo = {};
	TransferDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	TransferDepInfo.imageMemoryBarrierCount = 1;
	TransferDepInfo.pImageMemoryBarriers = &TransferImageBarrier;

	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;
	
	vkCmdPipelineBarrier2(VkCmdBuffer, &TransferDepInfo);

	VkBufferImageCopy ImageRegion = {};
	ImageRegion.bufferOffset = 0;
	ImageRegion.bufferRowLength = 0;
	ImageRegion.bufferImageHeight = 0;
	ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageRegion.imageSubresource.mipLevel = 0;
	ImageRegion.imageSubresource.baseArrayLayer = 0;
	ImageRegion.imageSubresource.layerCount = 1;
	ImageRegion.imageOffset = { 0, 0, 0 };
	ImageRegion.imageExtent = { ATLAS_W, ATLAS_H, 1 };

	vkCmdCopyBufferToImage(VkCmdBuffer, (VkBuffer)StagingBuffer, (VkImage)VRAMAtlasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);

	VkImageMemoryBarrier2 ShaderReadBarrier = {};
	ShaderReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	ShaderReadBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	ShaderReadBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	ShaderReadBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	ShaderReadBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	ShaderReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ShaderReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ShaderReadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ShaderReadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ShaderReadBarrier.image = (VkImage)VRAMAtlasImage;
	ShaderReadBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ShaderReadBarrier.subresourceRange.baseMipLevel = 0;
	ShaderReadBarrier.subresourceRange.levelCount = 1;
	ShaderReadBarrier.subresourceRange.baseArrayLayer = 0;
	ShaderReadBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo ShaderReadDepInfo = {};
	ShaderReadDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	ShaderReadDepInfo.imageMemoryBarrierCount = 1;
	ShaderReadDepInfo.pImageMemoryBarriers = &ShaderReadBarrier;

	vkCmdPipelineBarrier2(VkCmdBuffer, &ShaderReadDepInfo);
	BmRender_EndCommandBuffer(CommandBuffer);

	BmRender_SubmitInfo TransferSubmitInfo = {};
	TransferSubmitInfo.CommandBuffers = &CommandBuffer;
	TransferSubmitInfo.CommandBufferCount = 1;
	TransferSubmitInfo.WaitSemaphoreCount = 0;
	TransferSubmitInfo.SignalSemaphoreCount = 0;
	BmRender_QueueSubmit(GraphicsQueue, 1, &TransferSubmitInfo, nullptr);
	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyGPUBuffer(StagingBuffer);

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
	BmRender_Sampler AtlasSampler = BmRender_CreateSampler(&SamplerDesc);

	BmRender_DescriptorPoolSize PoolSize = {};
	PoolSize.Type = BmRender_DescriptorType::CombinedImageSampler;
	PoolSize.DescriptorCount = 1;
	BmRender_DescriptorPool DescriptorPool = BmRender_CreateDescriptorPool(&PoolSize, 1, 1, BmRender_DescriptorPoolType::UpdateAfterBind);

	BmRender_DescriptorSet DescriptorSet = BmRender_CreateDescriptorSet(DescriptorSetLayout, DescriptorPool);

	BmRender_ImageBinding ImageBinding = {};
	ImageBinding.Sampler = AtlasSampler;
	ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
	ImageBinding.ImageView = VRAMAtlasImageView;

	BmRender_DescriptorSetBinding DescriptorSetBinding = {};
	DescriptorSetBinding.BufferRegions = nullptr;
	DescriptorSetBinding.ImageBinding = ImageBinding;
	DescriptorSetBinding.BindingCount = 1;
	DescriptorSetBinding.DstArrayElement = 0;
	BmRender_UpdateDescriptorSet(DescriptorSet, &DescriptorSetBinding, 1);

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		BmRender_WaitForFences(InFlightFence, true, UINT64_MAX);
		BmRender_ResetFences(InFlightFence);

		u32 ImageIndex;
		BmRender_SwapchainResult AcquireResult = BmRender_AcquireNextSwapchainImage(UINT64_MAX, ImageAvailableSemaphore, nullptr, &ImageIndex);

		if (AcquireResult == BmRender_SwapchainResult::OutOfDate || AcquireResult == BmRender_SwapchainResult::Suboptimal)
		{
			continue;
		}

		BmRender_BeginCommandBuffer(CommandBuffer);
		BmRender_TransitionImageForRendering(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));

		BmRender_RenderingColorAttachment ColorAttachment = {};
		ColorAttachment.ImageView = BmRender_GetSwapchainImageView(ImageIndex);
		ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingInfo RenderingInfo = {};
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = BmRender_GetSwapchainExtent();
		RenderingInfo.ColorAttachments = &ColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = nullptr;

		BmRender_BeginRendering(CommandBuffer, &RenderingInfo);

		BmRender_BindPipeline(CommandBuffer, Pipeline);

		BmRender_RecordBindDescriptorSets(CommandBuffer, PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);

		BmRender_Draw(CommandBuffer, 6, 1, 0, 0);

		BmRender_EndRendering(CommandBuffer);

		BmRender_TransitionImageForPresentation(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));

		BmRender_EndCommandBuffer(CommandBuffer);

		BmRender_PipelineSyncStage WaitStages[] = { BmRender_PipelineSyncStage::ColorAttachmentOutput };

		BmRender_SubmitInfo SubmitInfo = {};
		SubmitInfo.WaitDstStageFlags = WaitStages;
		SubmitInfo.WaitSemaphores = &ImageAvailableSemaphore;
		SubmitInfo.WaitSemaphoreCount = 1;
		SubmitInfo.SignalSemaphores = &RenderFinishedSemaphore;
		SubmitInfo.SignalSemaphoreCount = 1;
		SubmitInfo.CommandBuffers = &CommandBuffer;
		SubmitInfo.CommandBufferCount = 1;
		SubmitInfo.WaitTimelineSemaphores = nullptr;
		SubmitInfo.WaitTimelineSemaphoreCount = 0;
		SubmitInfo.SignalTimelineSemaphores = nullptr;
		SubmitInfo.SignalTimelineSemaphoreCount = 0;

		BmRender_QueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFence);

		BmRender_PresentInfo PresentInfo = {};
		PresentInfo.WaitSemaphores = &RenderFinishedSemaphore;
		PresentInfo.WaitSemaphoreCount = 1;
		PresentInfo.ImageIndices = &ImageIndex;

		BmRender_SwapchainResult PresentResult = BmRender_QueuePresent(GraphicsQueue, &PresentInfo);
	}

	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyImageView(VRAMAtlasImageView);
	BmRender_DestroyImage(VRAMAtlasImage);
	BmRender_DestroySampler(AtlasSampler);
	BmRender_DestroyDescriptorPool(DescriptorPool);
	BmRender_DestroyDescriptorSetLayout(DescriptorSetLayout);
	BmRender_DestroyShader(VertexShader);
	BmRender_DestroyShader(FragmentShader);
	BmRender_DestroySemaphore(ImageAvailableSemaphore);
	BmRender_DestroySemaphore(RenderFinishedSemaphore);
	BmRender_DestroyCommandPool(CommandPool);
	BmRender_DestroyFence(InFlightFence);
	BmRender_DestroyPipeline(Pipeline);
	BmRender_DestroyPipelineLayout(PipelineLayout);

	BmRender_DeInit();

	return 0;
}
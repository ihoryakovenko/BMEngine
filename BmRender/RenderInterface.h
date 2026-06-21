#pragma once

#include <ShortTypes.h>

#if defined (BM_RENDER_VULKAN_BACKEND)
#include "VulkanBackend/VulkanRender.h"
#endif

enum class BmRender_DescriptorShaderStage : u64
{
	None = 0,
	Vertex = 1 << 0,
	Fragment = 1 << 1,
	Compute = 1 << 2,
};

enum class BmRender_PipelineShaderStage : u8
{
	Vertex,
	Fragment,
	Geometry,
	TessControl,
	TessEval,
	Compute
};

enum class BmRender_PipelineSyncStage : u64
{
	None = 0,
	TopOfPipe = 1ull << 0,
	VertexShader = 1ull << 1,
	FragmentShader = 1ull << 2,
	ColorAttachmentOutput = 1ull << 3,
	ComputeShader = 1ull << 4,
	BottomOfPipe = 1ull << 5,
};

enum class BmRender_ImageType : u8
{
	TransferSampled,
	DepthSamplad,
	ColorAttachmentSampled,
	MultiSampledDepthAttachment,
	MultiSampledColorAttachment,
};

enum class BmRender_FenceStatus : u8
{
	NotReady,
	Signaled,
};

enum class BmRender_WaitResult : u8
{
	Success,
	Timeout,
};

enum class BmRender_SwapchainResult : u8
{
	Success,
	Suboptimal,
	OutOfDate,
};

enum class BmRender_SemaphoreType : u8
{
	Binary,
	Timeline,
};

enum class BmRender_QueueType : u32
{
	None = 0,
	Graphic = 1ull << 0,
	Transfer = 1ull << 1,
};

enum class BmRender_PipelineType : u8
{
	Graphics,
	Compute,
};

enum class BmRender_DescriptorPoolType : u32
{
	None = 0,
	UpdateAfterBind = 1ull << 0,
	CreateFree = 1ull << 1,
};

enum class MemoryPropertyFlag : u32
{
	None,
	GPULocal = 1,
	HostCompatible = 2,
};

enum class BmRender_Filter : u32
{
	Nearest,
	Linear,
};

enum class BmRender_SamplerMipmapMode : u32
{
	Nearest,
	Linear,
};

enum class BmRender_SamplerAddressMode : u32
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
	MirrorClampToEdge,
};

enum class BmRender_CompareOp : u32
{
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always,
};

enum class BmRender_BorderColor : u32
{
	FloatTransparentBlack,
	IntTransparentBlack,
	FloatOpaqueBlack,
	IntOpaqueBlack,
	FloatOpaqueWhite,
	IntOpaqueWhite,
};

enum class BmRender_ImageLayout : u32
{
	Undefined,
	General,
	ColorAttachmentOptimal,
	DepthStencilAttachmentOptimal,
	DepthStencilReadOnlyOptimal,
	ShaderReadOnlyOptimal,
	TransferSrcOptimal,
	TransferDstOptimal,
	Preinitialized,
	PresentSrcKHR,
};

enum class BmRender_AttachmentLoadOp : u32
{
	Load,
	Clear,
	DontCare,
};

enum class BmRender_AttachmentStoreOp : u32
{
	Store,
	DontCare,
};

enum class BmRender_DescriptorType : u32
{
	Sampler,
	CombinedImageSampler,
	SampledImage,
	StorageImage,
	UniformTexelBuffer,
	StorageTexelBuffer,
	UniformBuffer,
	StorageBuffer,
	UniformBufferDynamic,
	StorageBufferDynamic,
	InputAttachment,
};

enum class BmRender_IndexType : u32
{
	Uint16,
	Uint32,
};

enum class BmRender_Format : u32
{
	Undefined,
	// 8-bit formats
	R8_UNORM,
	R8_SNORM,
	R8_USCALED,
	R8_SSCALED,
	R8_UINT,
	R8_SINT,
	R8_SRGB,
	// 16-bit formats
	R8G8_UNORM,
	R8G8_SNORM,
	R8G8_USCALED,
	R8G8_SSCALED,
	R8G8_UINT,
	R8G8_SINT,
	R8G8_SRGB,
	R16_UNORM,
	R16_SNORM,
	R16_USCALED,
	R16_SSCALED,
	R16_UINT,
	R16_SINT,
	R16_SFLOAT,
	// 24-bit formats
	R8G8B8_UNORM,
	R8G8B8_SNORM,
	R8G8B8_USCALED,
	R8G8B8_SSCALED,
	R8G8B8_UINT,
	R8G8B8_SINT,
	R8G8B8_SRGB,
	B8G8R8_UNORM,
	B8G8R8_SNORM,
	B8G8R8_USCALED,
	B8G8R8_SSCALED,
	B8G8R8_UINT,
	B8G8R8_SINT,
	B8G8R8_SRGB,
	// 32-bit formats
	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R8G8B8A8_USCALED,
	R8G8B8A8_SSCALED,
	R8G8B8A8_UINT,
	R8G8B8A8_SINT,
	R8G8B8A8_SRGB,
	B8G8R8A8_UNORM,
	B8G8R8A8_SNORM,
	B8G8R8A8_USCALED,
	B8G8R8A8_SSCALED,
	B8G8R8A8_UINT,
	B8G8R8A8_SINT,
	B8G8R8A8_SRGB,
	R16G16_UNORM,
	R16G16_SNORM,
	R16G16_USCALED,
	R16G16_SSCALED,
	R16G16_UINT,
	R16G16_SINT,
	R16G16_SFLOAT,
	R32_UINT,
	R32_SINT,
	R32_SFLOAT,
	// 48-bit formats
	R16G16B16_UNORM,
	R16G16B16_SNORM,
	R16G16B16_USCALED,
	R16G16B16_SSCALED,
	R16G16B16_UINT,
	R16G16B16_SINT,
	R16G16B16_SFLOAT,
	// 64-bit formats
	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R16G16B16A16_USCALED,
	R16G16B16A16_SSCALED,
	R16G16B16A16_UINT,
	R16G16B16A16_SINT,
	R16G16B16A16_SFLOAT,
	R32G32_UINT,
	R32G32_SINT,
	R32G32_SFLOAT,
	// 96-bit formats
	R32G32B32_UINT,
	R32G32B32_SINT,
	R32G32B32_SFLOAT,
	// 128-bit formats
	R32G32B32A32_UINT,
	R32G32B32A32_SINT,
	R32G32B32A32_SFLOAT,
	// Special formats
	A2R10G10B10_UNORM_PACK32,
	A2R10G10B10_SNORM_PACK32,
	A2R10G10B10_USCALED_PACK32,
	A2R10G10B10_SSCALED_PACK32,
	A2R10G10B10_UINT_PACK32,
	A2R10G10B10_SINT_PACK32,
	A2B10G10R10_UNORM_PACK32,
	A2B10G10R10_SNORM_PACK32,
	A2B10G10R10_USCALED_PACK32,
	A2B10G10R10_SSCALED_PACK32,
	A2B10G10R10_UINT_PACK32,
	A2B10G10R10_SINT_PACK32,
	// Depth formats
	D16_UNORM,
	D24_UNORM_S8_UINT,
	D32_SFLOAT,
	S8_UINT,
	D16_UNORM_S8_UINT,
	D32_SFLOAT_S8_UINT,
	// Compressed formats - BC1/BC2/BC3
	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,
	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,
	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,
	// Compressed formats - BC7
	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,
	// Compressed formats - ETC2
	ETC2_R8G8B8_UNORM_BLOCK,
	ETC2_R8G8B8_SRGB_BLOCK,
	ETC2_R8G8B8A1_UNORM_BLOCK,
	ETC2_R8G8B8A1_SRGB_BLOCK,
	ETC2_R8G8B8A8_UNORM_BLOCK,
	ETC2_R8G8B8A8_SRGB_BLOCK,
	// Special packed formats (not commonly used but needed for GliFormatToVkFormat)
	B10G11R11_UFLOAT_PACK32,
	E5B9G9R9_UFLOAT_PACK32,
};

enum class BmRender_PolygonMode : u32
{
	Fill,
	Line,
	Point,
};

enum class BmRender_CullModeFlags : u32
{
	None = 0,
	Front = 1 << 0,
	Back = 1 << 1,
	FrontAndBack = Front | Back,
};

enum class BmRender_FrontFace : u32
{
	CounterClockwise,
	Clockwise,
};

enum class BmRender_ColorComponentFlags : u32
{
	None = 0,
	R = 1 << 0,
	G = 1 << 1,
	B = 1 << 2,
	A = 1 << 3,
	RGBA = R | G | B | A,
};

enum class BmRender_BlendFactor : u32
{
	Zero,
	One,
	SrcColor,
	DstColor,
	SrcAlpha,
	DstAlpha,
	OneMinusSrcColor,
	OneMinusDstColor,
	OneMinusSrcAlpha,
	OneMinusDstAlpha,
};

enum class BmRender_BlendOp : u32
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
};

enum class BmRender_PrimitiveTopology : u32
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	TriangleFan,
	LineListWithAdjacency,
	LineStripWithAdjacency,
	TriangleListWithAdjacency,
	TriangleStripWithAdjacency,
	PatchList,
};

enum class BmRender_SampleCount : u32
{
	Count1 = 1,
	Count2 = 2,
	Count4 = 4,
	Count8 = 8,
	Count16 = 16,
	Count32 = 32,
	Count64 = 64,
};

struct BmRender_Offset2D
{
	s32 X;
	s32 Y;
};

struct BmRender_Dimensions
{
	u32 Width;
	u32 Height;
};

struct BmRender_Viewport
{
	f32 X;
	f32 Y;
	f32 Width;
	f32 Height;
	f32 MinDepth;
	f32 MaxDepth;
};

struct BmRender_Rect2D
{
	BmRender_Offset2D Offset;
	BmRender_Dimensions Extent;
};

struct BmRender_ClearColorValue
{
	union
	{
		f32 Float32[4];
		s32 Int32[4];
		u32 Uint32[4];
	};
};

struct BmRender_ClearDepthStencilValue
{
	f32 Depth;
	u32 Stencil;
};

struct BmRender_DescriptorPoolSize
{
	BmRender_DescriptorType Type;
	u32 DescriptorCount;
};

struct BmRender_SurfaceFormat
{
	BmRender_Format Format;
	//VkColorSpaceKHR ColorSpace;
};

struct AttachmentData
{
	u32 ColorAttachmentCount;
	BmRender_ImageView* ColorAttachments;
	BmRender_ImageView* DepthAttachment;
	BmRender_ImageView* StencilAttachment;
};

struct BmRHI_SamplerDescription
{
	BmRender_Filter MagFilter;
	BmRender_Filter MinFilter;
	BmRender_SamplerMipmapMode MipmapMode;
	BmRender_SamplerAddressMode AddressModeU;
	BmRender_SamplerAddressMode AddressModeV;
	BmRender_SamplerAddressMode AddressModeW;
	f32 MipLodBias;
	bool AnisotropyEnable;
	f32 MaxAnisotropy;
	bool CompareEnable;
	BmRender_CompareOp CompareOp;
	f32 MinLod;
	f32 MaxLod;
	BmRender_BorderColor BorderColor;
	bool UnnormalizedCoordinates;
};

struct BmRender_ImageDescription
{
	u32 Width;
	u32 Height;
	BmRender_Format Format;
	u32 ArrayLayers;
	BmRender_ImageType Type;
	BmRender_SampleCount SampleCount;
};

struct BmRender_PushConstant
{
	u32 Offset;
	u32 Size;
	BmRender_DescriptorShaderStage StageFlags;
};

struct BmRender_DescriptorSetLayoutBinding
{
	BmRender_DescriptorType DescriptorType;
	u32 DescriptorCount;
	BmRender_DescriptorShaderStage StageFlags;
};

struct BmRender_ImageUpdateData
{
	BmRender_Sampler* Sampler;
	BmRender_ImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
	BmRender_ImageView* ImageView;
};

struct BmRender_GPUBufferUpdateData
{
	BmRender_GPUBuffer* GPUBufferHandle;
	u64 BufferOffset;
	u64 Size;
};

struct BmRender_RenderingColorAttachment
{
	BmRender_ImageView* ImageView;
	BmRender_ImageView* ResolveImageView;
	BmRender_AttachmentLoadOp LoadOp;
	BmRender_AttachmentStoreOp StoreOp;
	BmRender_ClearColorValue ClearValue;
};

struct BmRender_RenderingDepthAttachment
{
	BmRender_ImageView* ImageView;
	BmRender_AttachmentLoadOp LoadOp;
	BmRender_AttachmentStoreOp StoreOp;
	BmRender_ClearDepthStencilValue ClearValue;
};

struct BmRender_RenderingInfo
{
	BmRender_Offset2D Offset;
	BmRender_Dimensions Extent;
	const BmRender_RenderingColorAttachment* ColorAttachments;
	u32 ColorAttachmentCount;
	const BmRender_RenderingDepthAttachment* DepthAttachment;
};

struct BmRender_DescriptorSetUpdateData
{
	BmRender_GPUBufferUpdateData* BufferRegions;
	BmRender_ImageUpdateData ImageBinding;
	u32 BindingCount;
	u32 DstArrayElement;
	u32 DstBinding;
};

struct BmRender_ShaderStageDescription
{
	BmRender_Shader* Shader;
	const char* EntryPointFunction;
	BmRender_PipelineShaderStage Stage;
};

struct BmRender_RasterizationState
{
	bool DepthClampEnable;
	bool RasterizerDiscardEnable;
	BmRender_PolygonMode PolygonMode;
	f32 LineWidth;
	BmRender_CullModeFlags CullMode;
	BmRender_FrontFace FrontFace;
	bool DepthBiasEnable;
};

struct BmRender_ColorBlendAttachment
{
	BmRender_ColorComponentFlags ColorWriteMask;
	bool BlendEnable;
	BmRender_BlendFactor SrcColorBlendFactor;
	BmRender_BlendFactor DstColorBlendFactor;
	BmRender_BlendOp ColorBlendOp;
	BmRender_BlendFactor SrcAlphaBlendFactor;
	BmRender_BlendFactor DstAlphaBlendFactor;
	BmRender_BlendOp AlphaBlendOp;
};

struct BmRender_ColorBlendState
{
	bool LogicOpEnable;
	u32 AttachmentCount;
};

struct BmRender_DepthStencilState
{
	bool DepthTestEnable;
	bool DepthWriteEnable;
	BmRender_CompareOp DepthCompareOp;
	bool DepthBoundsTestEnable;
	bool StencilTestEnable;
};

struct BmRender_MultisampleState
{
	bool SampleShadingEnable;
};

struct BmRender_InputAssemblyState
{
	BmRender_PrimitiveTopology Topology;
	bool PrimitiveRestartEnable;
};

struct BmRender_ViewportState
{
	u32 ViewportCount;
	u32 ScissorCount;
};

struct BmRender_PipelineSettings
{
	BmRender_RasterizationState RasterizationState;
	BmRender_ColorBlendAttachment ColorBlendAttachment;
	BmRender_ColorBlendState ColorBlendState;
	BmRender_DepthStencilState DepthStencilState;
	BmRender_MultisampleState MultisampleState;
	BmRender_InputAssemblyState InputAssemblyState;
	BmRender_ViewportState ViewportState;

	BmRender_Dimensions Extent;
	BmRender_Viewport Viewport;
	BmRender_Rect2D Scissor;
};

struct BmRender_PipelineLayoutDescription
{
	const BmRender_DescriptorSetLayout* SetLayouts;
	const BmRender_PushConstant* PushConstantRanges;
	u32 SetLayoutCount;
	u32 PushConstantRangeCount;
};

struct BmRender_ShaderDescription
{
	const u32* Code;
	u64 CodeSize;
};

struct BmRender_TimelineSemaphoreSubmit
{
	BmRender_Semaphore* Semaphore;
	u64 Value;
};

struct BmRender_SubmitInfo
{
	const BmRender_PipelineSyncStage* WaitDstStageFlags;
	const BmRender_Semaphore* WaitSemaphores;
	const BmRender_Semaphore* SignalSemaphores;
	const BmRender_TimelineSemaphoreSubmit* WaitTimelineSemaphores;
	const BmRender_TimelineSemaphoreSubmit* SignalTimelineSemaphores;
	const BmRender_CommandBuffer* CommandBuffers;

	u32 WaitSemaphoreCount;
	u32 WaitTimelineSemaphoreCount;
	u32 CommandBufferCount;
	u32 SignalSemaphoreCount;
	u32 SignalTimelineSemaphoreCount;
};

struct BmRender_PresentInfo
{
	const BmRender_Semaphore* WaitSemaphores;
	u32 WaitSemaphoreCount;
	const u32* ImageIndices;
};

struct BmRender_DrawIndexedIndirectCommand
{
	u32 IndexCount;
	u32 InstanceCount;
	u32 FirstIndex;
	s32 VertexOffset;
	u32 FirstInstance;
};

struct BmRender_InitData
{
	void* NativeWindow;
	bool EnableDebug;
	u32 TypeDebugDatainitialSize;
};

void BmRender_Init(const BmRender_InitData* InitData);
void BmRender_DeInit();

u32 BmRender_GetSwapchainImageCount();
bool BmRender_IsDedicatedQueuePresent(BmRender_QueueType BmRender_QueueType);
BmRender_Queue BmRender_CreateQueue(BmRender_QueueType BmRender_QueueType);
void BmRender_QueueSubmit(BmRender_Queue Queue, u32 SubmitCount, const BmRender_SubmitInfo* pSubmits, BmRender_Fence Fence);
BmRender_SwapchainResult BmRender_QueuePresent(BmRender_Queue Queue, const BmRender_PresentInfo* pPresentInfo);
BmRender_SwapchainResult BmRender_AcquireNextSwapchainImage(u64 Timeout, BmRender_Semaphore Semaphore, BmRender_Fence Fence, u32* pImageIndex);

BmRender_SurfaceFormat BmRender_GetSurfaceFormat();
BmRender_Image* BmRender_GetSwapchainImage(u32 Index);
BmRender_ImageView* BmRender_GetSwapchainImageView(u32 Index);
BmRender_Dimensions BmRender_GetSwapchainExtent();

BmRender_Instance BmRender_GetVulkanInstance();
BmRender_PhysicalDevice BmRender_GetPhysicalDevice();
BmRender_Device BmRender_GetLogicalDevice();
u32 BmRender_GetGraphicsQueueFamily();
u32 BmRender_GetQueueFamily(BmRender_Queue Queue);

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description);
BmRender_Pipeline BmRender_CreateGraphicsPipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_PipelineSettings* Settings, const BmRender_ShaderStageDescription* ShaderStageDescriptions, u32 ShaderStagesCount, const AttachmentData* Attachment);
BmRender_Pipeline BmRender_CreateComputePipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_ShaderStageDescription* ShaderStageDescription);
BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description);
BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutBinding* Bindings, u32 BindingsCount);
BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolSize* PoolSizes, u32 MaxSets, u32 PoolSizeCount, BmRender_DescriptorPoolType Type);
BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout* LayoutHandle, BmRender_DescriptorPool* PoolHandle);
BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateIndirectDrawBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size);
BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type, BmRender_SampleCount SampleCount);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type, u32 ArrayLayers, BmRender_SampleCount SampleCount);
BmRender_ImageView BmRender_CreateImageView2D(const BmRender_Image* Handle);
BmRender_ImageView BmRender_CreateImageView2DArray(const BmRender_Image* Handle, u32 BaseLayer, u32 LayerCount);
BmRender_PushConstant BmRender_CreatePushConstant(BmRender_DescriptorShaderStage Stage, u32 Offset, u32 Size);
BmRender_Fence BmRender_CreateFence();
BmRender_Semaphore BmRender_CreateSemaphore();
BmRender_Semaphore BmRender_CreateTimelineSemaphore(u64 InitialValue);
BmRender_CommandPool BmRender_CreateCommandPool(BmRender_QueueType BmRender_QueueType);
BmRender_CommandBuffer BmRender_AllocateCommandBuffer(const BmRender_CommandPool* CommandPool);

void BmRender_UpdateHostCompatibleBuffer(BmRender_GPUBuffer* Buffer, u64 BufferOffset, u64 DataSize, const void* Data);
void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet* DescriptorSetHandle, const BmRender_DescriptorSetUpdateData* Bindings, u32 BindingsCount);

BmRender_FenceStatus BmRender_GetFenceStatus(BmRender_Fence Handle);
BmRender_WaitResult BmRender_WaitForFences(BmRender_Fence Handle, bool WaitAll, u64 Timeout);
u64 BmRender_GetBufferDeviceAddress(BmRender_GPUBuffer* Buffer);

void BmRender_ResetFences(BmRender_Fence Handle);
void BmRender_GetSemaphoreCounterValue(BmRender_Semaphore Handle, u64* pValue);
void BmRender_BeginCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_EndCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_QueueWaitIdle(BmRender_Queue Queue);
void BmRender_DeviceWaitIdle();

void BmRender_TransitionImageForRendering(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer = 0,  u32 LayersCount = 1);
void BmRender_TransitionImageForSampling(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_TransitionImageForPresentation(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_TransitionImageForComputeWrite(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_RecordUpdateGPULocalBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* DstBuffer, BmRender_GPUBuffer* SrcBuffer, u64 SrcOffset, u64 DstOffset, u64 DataSize);
void BmRender_BeginRendering(BmRender_CommandBuffer CommandBuffer, const BmRender_RenderingInfo* pRenderingInfo);
void BmRender_EndRendering(BmRender_CommandBuffer CommandBuffer);
void BmRender_BindPipeline(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline, BmRender_PipelineLayout Layout);
void BmRender_RecordPushConstants(BmRender_CommandBuffer CommandBuffer, BmRender_PipelineLayout PipelineLayout, BmRender_DescriptorShaderStage StageFlags, u32 Offset, u32 Size, const void* pValues);
void BmRender_RecordBindDescriptorSets(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline, BmRender_PipelineLayout PipelineLayout, u32 FirstSet, u32 DescriptorSetCount, const BmRender_DescriptorSet* pDescriptorSets, u32 DynamicOffsetCount, const u32* pDynamicOffsets);
void BmRender_RecordBindVertexBuffers(BmRender_CommandBuffer CommandBuffer, u32 FirstBinding, u32 BindingCount, const BmRender_GPUBuffer* Buffers, const u64* Offsets);
void BmRender_RecordBindIndexBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* Buffer, u64 Offset, BmRender_IndexType IndexType);
void BmRender_Draw(BmRender_CommandBuffer CommandBuffer, u32 VertexCount, u32 InstanceCount, u32 FirstVertex, u32 FirstInstance);
void BmRender_DrawIndexed(BmRender_CommandBuffer CommandBuffer, u32 IndexCount, u32 InstanceCount, u32 FirstIndex, u32 VertexOffset, u32 FirstInstance);
void BmRender_RecordDrawIndexedIndirect(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* IndirectBuffer, u64 Offset, u32 DrawCount, u32 Stride);
void BmRender_RecordDispatch(BmRender_CommandBuffer CommandBuffer, u32 GroupCountX, u32 GroupCountY, u32  GroupCountZ);

void BmRender_DestroySampler(BmRender_Sampler Handle);
void BmRender_DestroyPipeline(BmRender_Pipeline Handle);
void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle);
void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout* Handle);
void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle);
void BmRender_DestroyShader(BmRender_Shader Handle);
void BmRender_DestroyImage(BmRender_Image* Handle);
void BmRender_DestroyImageView(BmRender_ImageView Handle);
void BmRender_DestroyGPUBuffer(BmRender_GPUBuffer* Handle);
void BmRender_DestroyFence(BmRender_Fence Handle);
void BmRender_DestroySemaphore(BmRender_Semaphore Handle);
void BmRender_DestroyCommandPool(BmRender_CommandPool Handle);
void BmRender_FreeCommandBuffer(BmRender_CommandBuffer Handle);

void BmRender_FrameFree();

#if defined (BM_RENDER_VULKAN_BACKEND)

u32 BmRender_GetFormatAlignment(BmRender_Format Format);
VkFormat BmRender_FormatToVk(BmRender_Format Format);
VkAllocationCallbacks* BmRender_GetVulkanAllocator();

#endif

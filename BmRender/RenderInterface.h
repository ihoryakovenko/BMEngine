#pragma once

#include <ShortTypes.h>

struct GLFWwindow;

inline constexpr u32 MAX_DRAW_FRAMES = 3;

typedef struct BmRender_Instance_T* BmRender_Instance;
typedef struct BmRender_PhysicalDevice_T* BmRender_PhysicalDevice;
typedef struct BmRender_Device_T* BmRender_Device;
typedef struct BmRender_Sampler_T* BmRender_Sampler;
typedef struct BmRender_Pipeline_T* BmRender_Pipeline;
typedef struct BmRender_PipelineLayout_T* BmRender_PipelineLayout;
typedef struct BmRender_DescriptorSetLayout_T* BmRender_DescriptorSetLayout;
typedef struct BmRender_DescriptorPool_T* BmRender_DescriptorPool;
typedef struct BmRender_Shader_T* BmRender_Shader;
typedef struct BmRender_Image_T* BmRender_Image;
typedef struct BmRender_ImageView_T* BmRender_ImageView;
typedef struct BmRender_GPUBuffer_T* BmRender_GPUBuffer;
typedef struct BmRender_DescriptorSet_T* BmRender_DescriptorSet;
typedef struct BmRender_CommandWorker_T* BmRender_CommandWorker;
typedef struct BmRender_Fence_T* BmRender_Fence;
typedef struct BmRender_Semaphore_T* BmRender_Semaphore;
typedef struct BmRender_CommandPool_T* BmRender_CommandPool;
typedef struct BmRender_CommandBuffer_T* BmRender_CommandBuffer;
typedef struct BmRender_Queue_T* BmRender_Queue;
typedef struct BmRender_DeviceMemory_T* BmRender_DeviceMemory;

enum class BmRender_AttributeType : u8
{
	Int,
	Uint,
	Float,
	Vec2,
	Vec3,
	Vec4,
	Mat4
};

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
	GPULocal = 0,
	HostCompatible = 1,
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

enum class BmRender_VertexInputRate : u32
{
	Vertex,
	Instance,
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
	Count1,
	Count2,
	Count4,
	Count8,
	Count16,
	Count32,
	Count64,
};


struct BmRender_Offset2D
{
	s32 X;
	s32 Y;
};

struct BmRender_Extent2D
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
	BmRender_Extent2D Extent;
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
	BmRender_ImageView DepthAttachment;
	BmRender_ImageView StencilAttachment;
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
};

struct BmRender_PushConstant
{
	u32 offset;
	u32 size;
	BmRender_DescriptorShaderStage stageFlags; // Use existing BmRender_DescriptorShaderStage
};

struct BmRender_DescriptorSetLayoutBinding
{
	BmRender_DescriptorType DescriptorType;
	u32 DescriptorCount;
	BmRender_DescriptorShaderStage StageFlags;
};

struct BmRender_ImageBinding
{
	BmRender_Sampler Sampler;
	BmRender_ImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
	BmRender_ImageView ImageView;
};

struct BmRender_GPUBufferBinding
{
	BmRender_GPUBuffer GPUBufferHandle;
	u64 BufferOffset;
	u64 Size;
};

struct BmRender_RenderingColorAttachment
{
	BmRender_ImageView ImageView;
	BmRender_AttachmentLoadOp LoadOp;
	BmRender_AttachmentStoreOp StoreOp;
	BmRender_ClearColorValue ClearValue;
};

struct BmRender_RenderingDepthAttachment
{
	BmRender_ImageView ImageView;
	BmRender_AttachmentLoadOp LoadOp;
	BmRender_AttachmentStoreOp StoreOp;
	BmRender_ClearDepthStencilValue ClearValue;
};

struct BmRender_RenderingInfo
{
	BmRender_Offset2D Offset;
	BmRender_Extent2D Extent;
	const BmRender_RenderingColorAttachment* ColorAttachments;
	u32 ColorAttachmentCount;
	const BmRender_RenderingDepthAttachment* DepthAttachment;
};

struct BmRender_DescriptorSetBinding
{
	BmRender_GPUBufferBinding* BufferRegions;
	BmRender_ImageBinding ImageBinding;
	u32 BindingCount;
	u32 DstArrayElement;
};

struct VertexAttribute
{
	BmRender_AttributeType Type;
	u32 Offset;
};

struct BmRender_VertexBinding
{
	VertexAttribute* Attributes;
	u32 AttributesCount;
	u32 Stride;
	BmRender_VertexInputRate InputRate;
};

struct BmRender_ShaderStageDescription
{
	BmRender_Shader Shader;
	const char* EntryPointFunction;
};

struct BmRender_RasterizationState
{
	bool depthClampEnable;
	bool rasterizerDiscardEnable;
	BmRender_PolygonMode polygonMode;
	f32 lineWidth;
	BmRender_CullModeFlags cullMode;
	BmRender_FrontFace frontFace;
	bool depthBiasEnable;
};

struct BmRender_ColorBlendAttachment
{
	BmRender_ColorComponentFlags colorWriteMask;
	bool blendEnable;
	BmRender_BlendFactor srcColorBlendFactor;
	BmRender_BlendFactor dstColorBlendFactor;
	BmRender_BlendOp colorBlendOp;
	BmRender_BlendFactor srcAlphaBlendFactor;
	BmRender_BlendFactor dstAlphaBlendFactor;
	BmRender_BlendOp alphaBlendOp;
};

struct BmRender_ColorBlendState
{
	bool logicOpEnable;
	u32 attachmentCount;
};

struct BmRender_DepthStencilState
{
	bool depthTestEnable;
	bool depthWriteEnable;
	BmRender_CompareOp depthCompareOp;
	bool depthBoundsTestEnable;
	bool stencilTestEnable;
};

struct BmRender_MultisampleState
{
	bool sampleShadingEnable;
	BmRender_SampleCount rasterizationSamples;
};

struct BmRender_InputAssemblyState
{
	BmRender_PrimitiveTopology topology;
	bool primitiveRestartEnable;
};

struct BmRender_ViewportState
{
	u32 viewportCount;
	u32 scissorCount;
};

struct BmRender_PipelineDescription
{
	BmRender_PipelineLayout PipelineLayout;
	AttachmentData Attachment;

	const BmRender_ShaderStageDescription* ShaderStages;
	const BmRender_VertexBinding* VertexBindings;
	const BmRender_PushConstant* PushConstantRanges;
	const BmRender_DescriptorSetLayout* DescriptorSetLayouts;

	u32 ShaderStagesCount;
	u32 VertexBindingsCount;
	u32 DescriptorSetLayoutsCount;
	u32 PushConstantRangesCount;

	BmRender_RasterizationState RasterizationState;
	BmRender_ColorBlendAttachment ColorBlendAttachment;
	BmRender_ColorBlendState ColorBlendState;
	BmRender_DepthStencilState DepthStencilState;
	BmRender_MultisampleState MultisampleState;
	BmRender_InputAssemblyState InputAssemblyState;
	BmRender_ViewportState ViewportState;

	BmRender_Extent2D Extent;
	BmRender_Viewport Viewport;
	BmRender_Rect2D Scissor;
};

struct BmRender_PipelineLayoutDescription
{
	const BmRender_DescriptorSetLayout* SetLayouts;
	const BmRender_PushConstant* PushConstantRanges;
	u32 SetLayoutCount;
	u32 PushConstantRangeCount;
	BmRender_PipelineType PipelineType;
};

struct BmRender_ShaderDescription
{
	const u32* Code;
	u64 CodeSize;
	BmRender_PipelineShaderStage Stage;
};

struct BmRender_TimelineSemaphoreSubmit
{
	BmRender_Semaphore Semaphore;
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

struct BmRender_DescriptorSetLayoutBindingData
{
	BmRender_DescriptorType DescriptorType;
};

struct BmRender_DescriptorSetLayoutData
{
	BmRender_DescriptorSetLayoutBindingData* LayoutBindings;
	u32 BindingsCount;
};

struct BmRender_ShaderData
{
	BmRender_PipelineShaderStage Stage;
};

struct BmRender_ImageResource
{
	BmRender_DeviceMemory Memory;
	BmRender_Format Format;
	u64 Size;
	BmRender_ImageType Type;
	u32 Width;
	u32 Height;
};

struct BmRender_GPUBufferData
{
	BmRender_DeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
};

struct BmRender_DescriptorSetData
{
	BmRender_DescriptorSetLayout Layout;
};

struct BmRender_SemaphoreData
{
	BmRender_SemaphoreType Type;
};

struct BmRender_CommandPoolData
{
	u32 QueueFamilyIndex;
};

struct BmRender_CommandBufferData
{
	BmRender_CommandPool CommandPool;
};

struct BmRender_QueueData
{
	BmRender_QueueType QueueType;
};

struct BmRender_PipelineLayoutData
{
	BmRender_PipelineType PipelineType;
};

struct BmRender_ImageViewData
{
	BmRender_Image Image;
};

struct BmRender_DrawIndexedIndirectCommand
{
	u32 IndexCount;
	u32 InstanceCount;
	u32 FirstIndex;
	s32 VertexOffset;
	u32 FirstInstance;
};

void BmRender_Init(GLFWwindow* WindowHandler);
void BmRender_DeInit();

u32 BmRender_GetSwapchainImageCount();
bool BmRender_IsDedicatedQueuePresent(BmRender_QueueType BmRender_QueueType);
BmRender_Queue BmRender_CreateQueue(BmRender_QueueType BmRender_QueueType);
void BmRender_QueueSubmit(BmRender_Queue Queue, u32 SubmitCount, const BmRender_SubmitInfo* pSubmits, BmRender_Fence Fence);
BmRender_SwapchainResult BmRender_QueuePresent(BmRender_Queue Queue, const BmRender_PresentInfo* pPresentInfo);
BmRender_SwapchainResult BmRender_AcquireNextSwapchainImage(u64 Timeout, BmRender_Semaphore Semaphore, BmRender_Fence Fence, u32* pImageIndex);

BmRender_SurfaceFormat BmRender_GetSurfaceFormat();
BmRender_Image BmRender_GetSwapchainImage(u32 Index);
BmRender_ImageView BmRender_GetSwapchainImageView(u32 Index);
BmRender_Extent2D BmRender_GetSwapchainExtent();

BmRender_Instance BmRender_GetVulkanInstance();
BmRender_PhysicalDevice BmRender_GetPhysicalDevice();
BmRender_Device BmRender_GetLogicalDevice();
u32 BmRender_GetGraphicsQueueFamily();
BmRender_QueueType BmRender_GetQueueType(BmRender_Queue Queue);
u32 BmRender_GetQueueFamily(BmRender_Queue Queue);

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description);
BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description);
BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description);
BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutBinding* Bindings, u32 BindingsCount);
BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolSize* PoolSizes, u32 MaxSets, u32 PoolSizeCount, BmRender_DescriptorPoolType Type);
BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateIndirectDrawBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size);
BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type, u32 ArrayLayers);
BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle);
BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
BmRender_PushConstant BmRender_CreatePushConstant(BmRender_DescriptorShaderStage Stage, u32 Offset, u32 Size);
BmRender_Fence BmRender_CreateFence();
BmRender_Semaphore BmRender_CreateSemaphore();
BmRender_Semaphore BmRender_CreateTimelineSemaphore(u64 InitialValue);
BmRender_CommandPool BmRender_CreateCommandPool(BmRender_QueueType BmRender_QueueType);
BmRender_CommandBuffer BmRender_AllocateCommandBuffer(BmRender_CommandPool CommandPool);

void BmRender_UpdateHostCompatibleBuffer(BmRender_GPUBuffer Buffer, u64 BufferOffset, u64 DataSize, const void* Data);
void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u32 BindingsCount);

BmRender_FenceStatus BmRender_GetFenceStatus(BmRender_Fence Handle);
BmRender_WaitResult BmRender_WaitForFences(BmRender_Fence Handle, bool WaitAll, u64 Timeout);
u64 BmRender_GetBufferDeviceAddress(BmRender_GPUBuffer Buffer);

void BmRender_ResetFences(BmRender_Fence Handle);
void BmRender_GetSemaphoreCounterValue(BmRender_Semaphore Handle, u64* pValue);
void BmRender_BeginCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_EndCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_QueueWaitIdle(BmRender_Queue Queue);
void BmRender_DeviceWaitIdle();

void BmRender_TransitionImageForRendering(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0,  u32 LayersCount = 1);
void BmRender_TransitionImageForSampling(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_TransitionImageForPresentation(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_RecordUpdateGPULocalBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer DstBuffer, BmRender_GPUBuffer SrcBuffer, u64 SrcOffset, u64 DstOffset, u64 DataSize);
void BmRender_BeginRendering(BmRender_CommandBuffer CommandBuffer, const BmRender_RenderingInfo* pRenderingInfo);
void BmRender_EndRendering(BmRender_CommandBuffer CommandBuffer);
void BmRender_BindPipeline(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline);
void BmRender_RecordPushConstants(BmRender_CommandBuffer CommandBuffer, BmRender_PipelineLayout PipelineLayout, BmRender_DescriptorShaderStage StageFlags, u32 Offset, u32 Size, const void* pValues);
void BmRender_RecordBindDescriptorSets(BmRender_CommandBuffer CommandBuffer, BmRender_PipelineLayout PipelineLayout, u32 FirstSet, u32 DescriptorSetCount, const BmRender_DescriptorSet* pDescriptorSets, u32 DynamicOffsetCount, const u32* pDynamicOffsets);
void BmRender_RecordBindVertexBuffers(BmRender_CommandBuffer CommandBuffer, u32 FirstBinding, u32 BindingCount, const BmRender_GPUBuffer* Buffers, const u64* Offsets);
void BmRender_RecordBindIndexBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer Buffer, u64 Offset, BmRender_IndexType IndexType);
void BmRender_Draw(BmRender_CommandBuffer CommandBuffer, u32 VertexCount, u32 InstanceCount, u32 FirstVertex, u32 FirstInstance);
void BmRender_DrawIndexed(BmRender_CommandBuffer CommandBuffer, u32 IndexCount, u32 InstanceCount, u32 FirstIndex, u32 VertexOffset, u32 FirstInstance);
void BmRender_RecordDrawIndexedIndirect(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer IndirectBuffer, u64 Offset, u32 DrawCount, u32 Stride);

void BmRender_DestroySampler(BmRender_Sampler Handle);
void BmRender_DestroyPipeline(BmRender_Pipeline Handle);
void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle);
void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle);
void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle);
void BmRender_DestroyShader(BmRender_Shader Handle);
void BmRender_DestroyImage(BmRender_Image Handle);
void BmRender_DestroyImageView(BmRender_ImageView Handle);
void BmRender_DestroyGPUBuffer(BmRender_GPUBuffer Handle);
void BmRender_DestroyFence(BmRender_Fence Handle);
void BmRender_DestroySemaphore(BmRender_Semaphore Handle);
void BmRender_DestroyCommandPool(BmRender_CommandPool Handle);
void BmRender_FreeCommandBuffer(BmRender_CommandBuffer Handle);

void BmRender_GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle, BmRender_DescriptorSetLayoutData* OutData);
bool BmRender_GetShaderData(BmRender_Shader Handle, BmRender_ShaderData* OutData);
bool BmRender_GetImageData(BmRender_Image Handle, BmRender_ImageResource* OutData);
bool BmRender_GetGPUBufferData(BmRender_GPUBuffer Handle, BmRender_GPUBufferData* OutData);
bool BmRender_GetDescriptorSetData(BmRender_DescriptorSet Handle, BmRender_DescriptorSetData* OutData);
bool BmRender_GetSemaphoreData(BmRender_Semaphore Handle, BmRender_SemaphoreData* OutData);
bool BmRender_GetCommandPoolData(BmRender_CommandPool Handle, BmRender_CommandPoolData* OutData);
bool BmRender_GetCommandBufferData(BmRender_CommandBuffer Handle, BmRender_CommandBufferData* OutData);
bool BmRender_GetQueueData(BmRender_Queue Handle, BmRender_QueueData* OutData);
bool BmRender_GetPipelineLayoutData(BmRender_PipelineLayout Handle, BmRender_PipelineLayoutData* OutData);
bool BmRender_GetImageViewData(BmRender_ImageView Handle, BmRender_ImageViewData* OutData);

void BmRender_FrameFree();

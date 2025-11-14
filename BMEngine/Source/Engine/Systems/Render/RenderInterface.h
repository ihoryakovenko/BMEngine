#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <string>
#include <unordered_map>

struct GLFWwindow;

typedef u64 PrivateHandle;
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

enum class BmRender_QueueType : u8
{
	None = 0,
	Graphic = 1ull << 0,
	Transfer = 1ull << 1,
};


// TODO: Check

enum class BufferUsageFlag
{
	UniformFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	StagingFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	StorageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	VertexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	IndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	CombinedVertexIndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	InstanceFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	IndirectDrawBufferFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
};

enum class MemoryPropertyFlag
{
	GPULocal = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	HostCompatible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
};

struct AttachmentData
{
	u32 ColorAttachmentCount;
	VkFormat ColorAttachmentFormats[16]; // get max attachments from device
	VkFormat DepthAttachmentFormat;
	VkFormat StencilAttachmentFormat;
};

struct PipelineResourceInfo
{
	AttachmentData PipelineAttachmentData;
	BmRender_PipelineLayout PipelineLayout = {};
};
// Check

struct BmRHI_SamplerDescription
{
	VkFilter MagFilter;
	VkFilter MinFilter;
	VkSamplerMipmapMode MipmapMode;
	VkSamplerAddressMode AddressModeU;
	VkSamplerAddressMode AddressModeV;
	VkSamplerAddressMode AddressModeW;
	f32 MipLodBias;
	VkBool32 AnisotropyEnable;
	f32 MaxAnisotropy;
	VkBool32 CompareEnable;
	VkCompareOp CompareOp;
	f32 MinLod;
	f32 MaxLod;
	VkBorderColor BorderColor;
	VkBool32 UnnormalizedCoordinates;
};

struct BmRender_ImageDescription
{
	u32 Width;
	u32 Height;
	VkFormat Format;
	u32 ArrayLayers;
	BmRender_ImageType Type;
};

struct BmRender_PushConstant
{
	u32 offset;
	u32 size;
	VkShaderStageFlags stageFlags;
};

struct BmRender_DescriptorSetLayoutBinding
{
	VkDescriptorType DescriptorType;
	u32 DescriptorCount;
	BmRender_DescriptorShaderStage StageFlags;
};

struct BmRender_ImageBinding
{
	BmRender_Sampler Sampler;
	VkImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
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
	VkAttachmentLoadOp LoadOp;
	VkAttachmentStoreOp StoreOp;
	VkClearColorValue ClearValue;
};

struct BmRender_RenderingDepthAttachment
{
	BmRender_ImageView ImageView;
	VkAttachmentLoadOp LoadOp;
	VkAttachmentStoreOp StoreOp;
	VkClearDepthStencilValue ClearValue;
};

struct BmRender_RenderingInfo
{
	VkOffset2D Offset;
	VkExtent2D Extent;
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
	VkVertexInputRate InputRate;
};

struct BmRender_ShaderStageDescription
{
	BmRender_Shader Shader;
	const char* EntryPointFunction;
};

struct BmRender_PipelineDescription
{
	BmRender_PipelineLayout PipelineLayout;
	PipelineResourceInfo ResourceInfo;

	const BmRender_ShaderStageDescription* ShaderStages;
	const BmRender_VertexBinding* VertexBindings;
	const BmRender_PushConstant* PushConstantRanges;
	const BmRender_DescriptorSetLayout* DescriptorSetLayouts;

	u32 ShaderStagesCount;
	u32 VertexBindingsCount;
	u32 DescriptorSetLayoutsCount;
	u32 PushConstantRangesCount;

	VkPipelineRasterizationStateCreateInfo RasterizationState;
	VkPipelineColorBlendAttachmentState ColorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo ColorBlendState;
	VkPipelineDepthStencilStateCreateInfo DepthStencilState;
	VkPipelineMultisampleStateCreateInfo MultisampleState;
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
	VkPipelineViewportStateCreateInfo ViewportState;

	VkExtent2D Extent;
	VkViewport Viewport;
	VkRect2D Scissor;
};

struct BmRender_PipelineLayoutDescription
{
	const BmRender_DescriptorSetLayout* SetLayouts;
	const BmRender_PushConstant* PushConstantRanges;
	u32 SetLayoutCount;
	u32 PushConstantRangeCount;
};

struct BmRender_DescriptorPoolDescription
{
	u32 MaxSets;
	u32 PoolSizeCount;
	const VkDescriptorPoolSize* PoolSizes;
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
	const VkPipelineStageFlags* WaitDstStageFlags;
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

void BmRender_Init(GLFWwindow* WindowHandler, u32 MaxFramesInFly);
void BmRender_DeInit();

u32 BmRender_GetSwapchainImageCount();
bool BmRender_IsDedicatedQueuePresent(BmRender_QueueType BmRender_QueueType);
BmRender_Queue BmRender_CreateQueue(BmRender_QueueType BmRender_QueueType);
void BmRender_QueueSubmit(BmRender_Queue Queue, u32 SubmitCount, const BmRender_SubmitInfo* pSubmits, BmRender_Fence Fence);
BmRender_SwapchainResult BmRender_QueuePresent(BmRender_Queue Queue, const BmRender_PresentInfo* pPresentInfo);
BmRender_SwapchainResult BmRender_AcquireNextSwapchainImage(u64 Timeout, BmRender_Semaphore Semaphore, VkFence Fence, u32* pImageIndex);

VkSurfaceFormatKHR BmRender_GetSurfaceFormat();
BmRender_Image BmRender_GetSwapchainImage(u32 Index);
BmRender_ImageView BmRender_GetSwapchainImageView(u32 Index);
VkExtent2D BmRender_GetSwapchainExtent();

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description);
BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description);
BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description);
BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutBinding* Bindings, u64 BindingsCount);
BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description);
BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, MemoryPropertyFlag MemoryFlag, BmRender_PipelineSyncStage BufferStage);
BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag, BmRender_PipelineSyncStage BufferStage);
BmRender_GPUBuffer BmRender_CreateIndirectDrawBuffer(u64 Size, MemoryPropertyFlag MemoryFlag);
BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size);
BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type, u32 ArrayLayers);
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
void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);

BmRender_FenceStatus BmRender_GetFenceStatus(BmRender_Fence Handle);
BmRender_WaitResult BmRender_WaitForFences(BmRender_Fence Handle, VkBool32 WaitAll, u64 Timeout);
void BmRender_ResetFences(BmRender_Fence Handle);
void BmRender_GetSemaphoreCounterValue(BmRender_Semaphore Handle, u64* pValue);
void BmRender_BeginCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_EndCommandBuffer(BmRender_CommandBuffer Handle);
void BmRender_QueueWaitIdle(BmRender_Queue Queue);
void BmRender_DeviceWaitIdle();

void BmRender_TransitionImageForRendering(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0,  u32 LayersCount = 1);
void BmRender_TransitionImageForSampling(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_TransitionImageForPresentation(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer = 0, u32 LayersCount = 1);
void BmRender_BeginRendering(BmRender_CommandBuffer CommandBuffer, const BmRender_RenderingInfo* pRenderingInfo);
void BmRender_EndRendering(BmRender_CommandBuffer CommandBuffer);
void BmRender_BindPipeline(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline);
void BmRender_Draw(BmRender_CommandBuffer CommandBuffer, u32 VertexCount, u32 InstanceCount, u32 FirstVertex, u32 FirstInstance);
void BmRender_DrawIndexed(BmRender_CommandBuffer CommandBuffer, u32 IndexCount, u32 InstanceCount, u32 FirstIndex, u32 VertexOffset, u32 FirstInstance);


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






void Test_Memory_LinearAllocator_FreeAll();


//void BmRender_RecordBufferCopy(BmRender_GPUBuffer StagingBuffer, u64 StagingBufferOffset, BmRender_GPUBuffer DstBuffer, u64 DstBufferOffset);
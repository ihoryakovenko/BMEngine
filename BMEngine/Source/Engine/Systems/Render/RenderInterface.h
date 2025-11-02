#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <string>
#include <unordered_map>

struct GLFWwindow;

typedef u64 PrivateHandle;
typedef struct { PrivateHandle Private; } BmRender_Sampler;
typedef struct { PrivateHandle Private; } BmRender_Pipeline;
typedef struct { PrivateHandle Private; } BmRender_PipelineLayout;
typedef struct { PrivateHandle Private; } BmRender_DescriptorSetLayout;
typedef struct { PrivateHandle Private; } BmRender_DescriptorPool;
typedef struct { PrivateHandle Private; } BmRender_Shader;
typedef struct { PrivateHandle Private; } BmRender_Image;
typedef struct { PrivateHandle Private; } BmRender_ImageView;
typedef struct { PrivateHandle Private; } BmRender_GPUBuffer;
typedef struct { PrivateHandle Private; } BmRender_PushConstant;
typedef struct { PrivateHandle Private; } BmRender_DescriptorSet;
typedef struct { PrivateHandle Private; } BmRender_CommandWorker;

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


// TODO: Check

enum class BufferUsageFlag
{
	UniformFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	StagingFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	StorageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	VertexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	IndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	CombinedVertexIndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	InstanceFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

void BmRender_Init(GLFWwindow* WindowHandler, u32 MaxFramesInFly);
void BmRender_DeInit();

u32 BmRender_GetMaxFramesInFly();

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
BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size);
BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type, u32 ArrayLayers);
BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle, VkImageAspectFlags AspectFlags);
BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
BmRender_PushConstant BmRender_CreatePushConstant(BmRender_DescriptorShaderStage Stage, u32 Offset, u32 Size);

void BmRender_UpdateHostCompatibleBuffer(BmRender_GPUBuffer Buffer, u64 BufferOffset, u64 DataSize, const void* Data);
void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);

void BmRender_DestroySampler(BmRender_Sampler Handle);
void BmRender_DestroyPipeline(BmRender_Pipeline Handle);
void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle);
void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle);
void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle);
void BmRender_DestroyShader(BmRender_Shader Handle);
void BmRender_DestroyImage(BmRender_Image Handle);

void Test_FrameFree();

u32 BmRender_GetCurrentFrameIndex();

u32 BmRender_AcquireNextSwapchainImage(u32 CurrentFrame);
void BmRender_StartRecording(BmRender_CommandWorker Handle);

BmRender_CommandWorker BmRender_AcquireWorker(u64 Timeout);
//void BmRender_RecordBufferCopy(BmRender_GPUBuffer StagingBuffer, u64 StagingBufferOffset, BmRender_GPUBuffer DstBuffer, u64 DstBufferOffset);
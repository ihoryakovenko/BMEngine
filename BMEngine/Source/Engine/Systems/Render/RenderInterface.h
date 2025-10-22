#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

enum class BufferUpdateFrequency
{
	Static,
	PerFrame
};

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
typedef struct { PrivateHandle Private; } BmRender_GPUBufferEntry;
typedef struct { PrivateHandle Private; } BmRender_PushConstant;
typedef struct { PrivateHandle Private; } BmRender_DescriptorSet;

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

enum class PipelineStage
{
	Vertex = 0x00000001,
	Fragment = 0x00000080,
};

enum class ImageType
{
	TransferSampled,
	DepthSamplad,
	ColorAttachmentSampled,
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
	VkPipelineLayout PipelineLayout = nullptr;
};

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
	ImageType Type;
};

struct BmRender_LayoutBinding
{
	VkDescriptorType DescriptorType;
	u32 DescriptorCount;
	VkShaderStageFlags StageFlags;
};

struct BmRender_ImageBinding
{
	BmRender_Sampler Sampler;
	VkImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
	BmRender_ImageView ImageView;
};

struct BmRender_DescriptorSetBinding
{
	BmRender_GPUBufferEntry* BufferRegions;
	BmRender_ImageBinding ImageBinding;

	u32 BindingCount;
	u32 DstArrayElement;
};

struct BmRender_DescriptorSetLayoutDescription
{
	const BmRender_LayoutBinding* Bindings;
	u64 BindingsCount;
};

struct BmRender_PipelineDescription
{
	VkExtent2D Extent;
	VkPipelineLayout PipelineLayout;
	PipelineResourceInfo ResourceInfo;

	const VkPipelineShaderStageCreateInfo* ShaderStages;
	u32 ShaderStagesCount;
	const VkVertexInputBindingDescription* VertexBindings;
	u32 VertexBindingsCount;
	const VkVertexInputAttributeDescription* VertexAttributes;
	u32 VertexAttributesCount;

	// Pipeline layout information
	const BmRender_DescriptorSetLayout* DescriptorSetLayouts;
	u32 DescriptorSetLayoutsCount;
	const BmRender_PushConstant* PushConstantRanges;
	u32 PushConstantRangesCount;
	VkPipelineLayoutCreateFlags PipelineLayoutFlags;

	VkPipelineRasterizationStateCreateInfo RasterizationState;
	VkPipelineColorBlendAttachmentState ColorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo ColorBlendState;
	VkPipelineDepthStencilStateCreateInfo DepthStencilState;
	VkPipelineMultisampleStateCreateInfo MultisampleState;
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
	VkPipelineViewportStateCreateInfo ViewportState;
	VkViewport Viewport;
	VkRect2D Scissor;
};

struct BmRender_PipelineLayoutDescription
{
	u32 SetLayoutCount;
	const BmRender_DescriptorSetLayout* SetLayouts;
	u32 PushConstantRangeCount;
	const BmRender_PushConstant* PushConstantRanges;
	VkPipelineLayoutCreateFlags Flags;
};

BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency);
BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency);
BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage);
BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage);

BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, ImageType Type);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, ImageType Type, u32 ArrayLayers);

BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle, VkImageAspectFlags AspectFlags);
BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags);

BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);

BmRender_PushConstant BmRender_CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size);



struct BmRender_DescriptorPoolDescription
{
	u32 MaxSets;
	u32 PoolSizeCount;
	const VkDescriptorPoolSize* PoolSizes;
	VkDescriptorPoolCreateFlags Flags;
};

struct BmRender_ShaderDescription
{
	const u32* Code;
	u64 CodeSize;
};


void BmRender_Init(GLFWwindow* WindowHandler);
void BmRender_DeInit();

VkAllocationCallbacks* BmRender_GetVulkanAllocator();

void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);


BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description);
BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description);
BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description);
BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutDescription* Description);
BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description);
BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description);
BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle);
BmRender_GPUBufferEntry BmRender_CreateGPUBufferEntry(u64 BufferOffset, u64 RegionSize, BmRender_GPUBuffer BufferHandle);

void BmRender_DestroySampler(BmRender_Sampler Handle);
void BmRender_DestroyPipeline(BmRender_Pipeline Handle);
void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle);
void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle);
void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle);
void BmRender_DestroyShader(BmRender_Shader Handle);
void BmRender_DestroyImage(BmRender_Image Handle);

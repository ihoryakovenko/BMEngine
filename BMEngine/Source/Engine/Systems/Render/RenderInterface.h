#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vector>

#include "Util/EngineTypes.h"

enum class BufferUpdateFrequency
{
	Static,
	PerFrame
};

typedef struct BmRender_BufferRegion_T* BmRender_BufferRegion;
typedef struct BmRender_BufferArrayRegion_T* BmRender_BufferArrayRegion;
typedef struct BmRender_PushConstant_T* BmRender_PushConstant;

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
	const char* Sampler;
	VkImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
	BmRender_ImageView ImageView;
};

struct BmRender_DescriptorSetBinding
{
	BmRender_BufferRegion* BufferRegions;
	BmRender_ImageBinding ImageBinding;

	u32 BindingCount;
	u32 DstArrayElement;
};

struct BmRender_DescriptorSetDescription
{
	std::string Layout;
	std::string Pool;
	const BmRender_DescriptorSetBinding* Bindings;
	u64 BindingsCount;
};

struct BmRender_DescriptorSetLayoutDescription
{
	const BmRender_LayoutBinding* Bindings;
	u64 BindingsCount;
};

struct BmRender_ImageViewBindingDescription
{
	const char* Sampler;
	u32 BindingIndex;
	u64 ArrayElement;
};

struct BmRender_PipelineDescription
{
	VkExtent2D Extent;
	VkPipelineLayout PipelineLayout;
	PipelineResourceInfo ResourceInfo;

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkVertexInputBindingDescription> VertexBindings;
	std::vector<VkVertexInputAttributeDescription> VertexAttributes;

	// Pipeline layout information
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkPushConstantRange> PushConstantRanges;
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
	const VkDescriptorSetLayout* SetLayouts;
	u32 PushConstantRangeCount;
	const VkPushConstantRange* PushConstantRanges;
	VkPipelineLayoutCreateFlags Flags;
	const void* Next;
};

void BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name);
void BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name);
void BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name);
void BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name);

BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, ImageType Type);
BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, ImageType Type, u32 ArrayLayers);

BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle, VkImageAspectFlags AspectFlags);
BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags);

void BmRender_CreateDescriptorSet(const std::string& Name, BmRender_DescriptorSetLayout LayoutHandle, const std::string& PoolName);
void BmRender_UpdateDescriptorSet(const std::string& DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);

BmRender_PushConstant CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size);



struct BmRender_DescriptorPoolDescription
{
	u32 MaxSets;
	u32 PoolSizeCount;
	const VkDescriptorPoolSize* PoolSizes;
	VkDescriptorPoolCreateFlags Flags;
	const void* Next;
};

struct BmRender_ShaderDescription
{
	const u32* Code;
	u64 CodeSize;
};


void BmRender_Init();
void BmRender_DeInit();

VkAllocationCallbacks* BmRender_GetVulkanAllocator();



BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description);
BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description);
BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description);
BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutDescription* Description);
BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description);
BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description);


void BmRender_DestroySampler(BmRender_Sampler Handle);
void BmRender_DestroyPipeline(BmRender_Pipeline Handle);
void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle);
void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle);
void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle);
void BmRender_DestroyShader(BmRender_Shader Handle);
void BmRender_DestroyImage(BmRender_Image Handle);

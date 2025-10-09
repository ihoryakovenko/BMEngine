#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

enum class BufferUpdateFrequency
{
	Static,
	PerFrame
};

typedef struct BmRender_BufferRegion_T* BmRender_BufferRegion;
typedef struct BmRender_BufferArrayRegion_T* BmRender_BufferArrayRegion;
typedef struct BmRender_ImageResource_T* BmRender_ImageResource;
typedef struct BmRender_ImageViewResource_T* BmRender_ImageViewResource;
typedef struct BmRender_PushConstant_T* BmRender_PushConstant;

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

struct BmRender_SamplerDescription
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

struct BmRender_BufferBinding
{
	std::string Buffer;
	u64 Offset;
	u64 Range;
};

struct BmRender_ImageBinding
{
	std::string Sampler;
	VkImageLayout ImageLayout; // Check if can store layout with Image resource as target layout and use instead this
	BmRender_ImageViewResource ImageView;
};

struct BmRender_DescriptorSetBinding
{
	union
	{
		BmRender_BufferBinding BufferBinding;
		BmRender_ImageBinding ImageBinding;
	};
	u32 DstArrayElement;

	///////////////// REPLACE FUCKING STRINGSSSSSSSS!!!!!!!!!!!!!!!!!!!!!!!!
	BmRender_DescriptorSetBinding() : BufferBinding{}, DstArrayElement(0) {}

	///////////////// REPLACE FUCKING STRINGSSSSSSSS!!!!!!!!!!!!!!!!!!!!!!!!
	~BmRender_DescriptorSetBinding() {}

	///////////////// REPLACE FUCKING STRINGSSSSSSSS!!!!!!!!!!!!!!!!!!!!!!!!
	BmRender_DescriptorSetBinding(const BmRender_DescriptorSetBinding& other) : BufferBinding{other.BufferBinding}, DstArrayElement(other.DstArrayElement) {}

	///////////////// REPLACE FUCKING STRINGSSSSSSSS!!!!!!!!!!!!!!!!!!!!!!!!
	BmRender_DescriptorSetBinding& operator=(const BmRender_DescriptorSetBinding& other) {
		if (this != &other) {
			BufferBinding = other.BufferBinding;
			DstArrayElement = other.DstArrayElement;
		}
		return *this;
	}
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

void BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name);
void BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name);
void BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name);
void BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name);

BmRender_ImageResource BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, ImageType Type);
BmRender_ImageResource BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, ImageType Type, u32 ArrayLayers);

BmRender_ImageViewResource BmRender_CreateImageView2D(BmRender_ImageResource Handle, VkImageAspectFlags AspectFlags);
BmRender_ImageViewResource BmRender_CreateImageView2DArray(BmRender_ImageResource Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags);

void BmRender_CreateDescriptorSet(const std::string& Name, const std::string& LayoutName, const std::string& PoolName);
void BmRender_UpdateDescriptorSet(const std::string& DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);

BmRender_PushConstant CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size);

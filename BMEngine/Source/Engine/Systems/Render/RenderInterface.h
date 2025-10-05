#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

enum class BufferUpdateFrequency
{
	Static,
	Dynamic
};

typedef struct BmRender_BufferRegion_T* BmRender_BufferRegion;
typedef struct BmRender_BufferArrayRegion_T* BmRender_BufferArrayRegion;
typedef struct BmRender_ImageResource_T* BmRender_ImageResource;
typedef struct BmRender_ImageViewResource_T* BmRender_ImageViewResource;

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

enum class StageBarier
{
	Vertex = 1,
	Fragment = 2,
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
};

struct BmRender_DescriptorSetBinding
{
	std::string Buffer;
	u32 Binding;
	VkDescriptorType DescriptorType;
	u64 Offset;
	u64 Range;
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
	const VkDescriptorSetLayoutBinding* Bindings;
	u64 BindingsCount;
	const void* Next;
};

struct BmRender_ImageViewBindingDescription
{
	const char* Sampler;
	u32 BindingIndex;
	u64 ArrayElement;
};

void BmRender_CreateVertexStageBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name);
void BmRender_CreateInstanceBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name);
void BmRender_CreateUniformBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, std::string& Name);
void BmRender_CreateStorageBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, std::string& Name);

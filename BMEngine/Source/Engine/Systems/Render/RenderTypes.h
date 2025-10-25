#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/HandleManager.h"

#include "RenderInterface.h"

#include <atomic>

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}


struct SamplerData
{
	VkSampler VulkanSampler;
};

struct PipelineData
{
	VkPipeline VulkanPipeline;
};

struct PipelineLayoutData
{
	VkPipelineLayout VulkanPipelineLayout;
};

struct DescriptorSetLayoutBinding
{
	VkDescriptorType DescriptorType;
};

struct DescriptorSetLayoutData
{
	VkDescriptorSetLayout Layout;
	DescriptorSetLayoutBinding* LayoutBindings;
	u32 BindingsCount;
};

struct DescriptorPoolData
{
	VkDescriptorPool VulkanDescriptorPool;
};

struct ShaderData
{
	VkShaderModule VulkanShaderModule;
	PipelineStage Stage;
};

struct ImageResource
{
	VkImage Image;
	VkDeviceMemory Memory;
	u64 Size;
	std::atomic<bool> IsLoaded;
	VkFormat Format;
};

struct ImageViewData
{
	VkImageView View;
};

struct GPUBufferData
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
	PipelineStage BufferStage;
	BufferUpdateFrequency UpdateFrequency;
};

struct DescriptorSetData
{
	VkDescriptorSet Set;
	BmRender_DescriptorSetLayout Layout;
};

struct GPUBufferEntryData
{
	std::atomic<bool> IsLoaded;
	BmRender_GPUBuffer GPUBufferHandle;
	u64 BufferOffset;
	u64 Size;
};

struct PushConstantData
{
	VkPushConstantRange PushConstants;
};

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext::VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

void InitializeSamplerManager(u32 Size);
void InitializePipelineManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeDescriptorPoolManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeImageViewManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializeGPUBufferEntryManager(u32 Size);
void InitializePushConstantManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);

void DeinitSamplerManager(void(*CleanUpFunc)(SamplerData*));
void DeinitPipelineManager(void(*CleanUpFunc)(PipelineData*));
void DeinitPipelineLayoutManager(void(*CleanUpFunc)(PipelineLayoutData*));
void DeinitDescriptorSetLayoutManager(void(*CleanUpFunc)(DescriptorSetLayoutData*));
void DeinitDescriptorPoolManager(void(*CleanUpFunc)(DescriptorPoolData*));
void DeinitShaderManager(void(*CleanUpFunc)(ShaderData*));
void DeinitImageManager(void(*CleanUpFunc)(ImageResource*));
void DeinitImageViewManager(void(*CleanUpFunc)(ImageViewData*));
void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*));
void DeinitGPUBufferEntryManager();
void DeinitPushConstantManager();
void DeinitDescriptorSetManager();

BmRender_Sampler CreateSamplerHandle(const SamplerData* Data);
BmRender_Pipeline CreatePipelineHandle(const PipelineData* Data);
BmRender_PipelineLayout CreatePipelineLayoutHandle(const PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(const DescriptorPoolData* Data);
BmRender_Shader CreateShaderHandle(const ShaderData* Data);
BmRender_Image CreateImageHandle(const ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(const ImageViewData* Data);
BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data);
BmRender_GPUBufferEntry CreateGPUBufferEntryHandle(const GPUBufferEntryData* Data);
BmRender_PushConstant CreatePushConstantHandle(const PushConstantData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data);

void DestroySamplerHandle(BmRender_Sampler handle);
void DestroyPipelineHandle(BmRender_Pipeline handle);
void DestroyPipelineLayoutHandle(BmRender_PipelineLayout handle);
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout handle);
void DestroyDescriptorPoolHandle(BmRender_DescriptorPool handle);
void DestroyShaderHandle(BmRender_Shader handle);
void DestroyImageHandle(BmRender_Image handle);
void DestroyImageViewHandle(BmRender_ImageView Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroyGPUBufferEntryHandle(BmRender_GPUBufferEntry Handle);
void DestroyPushConstantHandle(BmRender_PushConstant Handle);
void DestroyDescriptorSetHandle(BmRender_DescriptorSet Handle);

SamplerData* GetSamplerData(BmRender_Sampler Handle);
PipelineData* GetPipelineData(BmRender_Pipeline Handle);
PipelineLayoutData* GetPipelineLayoutData(BmRender_PipelineLayout Handle);
DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle);
DescriptorPoolData* GetDescriptorPoolData(BmRender_DescriptorPool Handle);
ShaderData* GetShaderData(BmRender_Shader Handle);
ImageResource* GetImageData(BmRender_Image Handle);
ImageViewData* GetImageViewData(BmRender_ImageView Handle);
GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle);
GPUBufferEntryData* GetGPUBufferEntryData(BmRender_GPUBufferEntry Handle);
PushConstantData* GetPushConstantData(BmRender_PushConstant Handle);
DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle);


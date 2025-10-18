#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/HandleManager.h"

#include <atomic>

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

struct DescriptorSetLayoutData
{
	VkDescriptorSetLayout Layout;
	u32 BindingsIndex;
	u32 BindingsCount;
};

struct DescriptorPoolData
{
	VkDescriptorPool VulkanDescriptorPool;
};

struct ShaderData
{
	VkShaderModule VulkanShaderModule;
};

struct DescriptorSetLayoutBinding
{
	VkDescriptorType DescriptorType;
};

struct ImageResource
{
	VkImage Image;
	VkDeviceMemory Memory;
	u64 Size;
	std::atomic<bool> IsLoaded;
	VkFormat Format;
};

typedef struct { System_HandleManager_Handle Private; } BmRender_Sampler;
typedef struct { System_HandleManager_Handle Private; } BmRender_Pipeline;
typedef struct { System_HandleManager_Handle Private; } BmRender_PipelineLayout;
typedef struct { System_HandleManager_Handle Private; } BmRender_DescriptorSetLayout;
typedef struct { System_HandleManager_Handle Private; } BmRender_DescriptorPool;
typedef struct { System_HandleManager_Handle Private; } BmRender_Shader;
typedef struct { System_HandleManager_Handle Private; } BmRender_Image;

void InitializeSamplerManager(u32 Size);
void InitializePipelineManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeDescriptorPoolManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);

void DeinitSamplerManager(void(*CleanUpFunc)(SamplerData*));
void DeinitPipelineManager(void(*CleanUpFunc)(PipelineData*));
void DeinitPipelineLayoutManager(void(*CleanUpFunc)(PipelineLayoutData*));
void DeinitDescriptorSetLayoutManager(void(*CleanUpFunc)(DescriptorSetLayoutData*));
void DeinitDescriptorPoolManager(void(*CleanUpFunc)(DescriptorPoolData*));
void DeinitShaderManager(void(*CleanUpFunc)(ShaderData*));
void DeinitImageManager(void(*CleanUpFunc)(ImageResource*));

BmRender_Sampler CreateSamplerHandle(const SamplerData* Data);
BmRender_Pipeline CreatePipelineHandle(const PipelineData* Data);
BmRender_PipelineLayout CreatePipelineLayoutHandle(const PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(const DescriptorPoolData* Data);
BmRender_Shader CreateShaderHandle(const ShaderData* Data);
BmRender_Image CreateImageHandle(const ImageResource* Data);

void DestroySamplerHandle(BmRender_Sampler handle);
void DestroyPipelineHandle(BmRender_Pipeline handle);
void DestroyPipelineLayoutHandle(BmRender_PipelineLayout handle);
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout handle);
void DestroyDescriptorPoolHandle(BmRender_DescriptorPool handle);
void DestroyShaderHandle(BmRender_Shader handle);
void DestroyImageHandle(BmRender_Image handle);

SamplerData* GetSamplerData(BmRender_Sampler Handle);
PipelineData* GetPipelineData(BmRender_Pipeline Handle);
PipelineLayoutData* GetPipelineLayoutData(BmRender_PipelineLayout Handle);
DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle);
DescriptorPoolData* GetDescriptorPoolData(BmRender_DescriptorPool Handle);
ShaderData* GetShaderData(BmRender_Shader Handle);
ImageResource* GetImageData(BmRender_Image Handle);
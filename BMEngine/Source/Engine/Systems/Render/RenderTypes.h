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
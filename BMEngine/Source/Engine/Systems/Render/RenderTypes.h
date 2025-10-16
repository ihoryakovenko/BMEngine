#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/HandleManager.h"

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

typedef struct { System_HandleManager_Handle Private; } BmRender_Sampler;
typedef struct { System_HandleManager_Handle Private; } BmRender_Pipeline;
typedef struct { System_HandleManager_Handle Private; } BmRender_PipelineLayout;
typedef struct { System_HandleManager_Handle Private; } BmRender_DescriptorSetLayout;
typedef struct { System_HandleManager_Handle Private; } BmRender_DescriptorPool;
typedef struct { System_HandleManager_Handle Private; } BmRender_Shader;
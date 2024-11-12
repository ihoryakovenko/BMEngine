#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace BMR::VulkanResourceManagementSystem
{
	void Init(VkDevice ActiveLogicalDevice, u32 MaximumPipelines);
	void DeInit();

	VkPipelineLayout CreatePipelineLayout(const VkDescriptorSetLayout* DescriptorLayouts,
		u32 DescriptorLayoutsCount, const VkPushConstantRange* PushConstant, u32 PushConstantsCount);

	VkPipeline* CreatePipelines(const BMRSPipelineShaderInfo* ShaderInputs, const BMRVertexInput* VertexInputs,
		const BMRPipelineSettings* PipelinesSettings, const BMRPipelineResourceInfo* ResourceInfos,
		u32 PipelineCount);
}

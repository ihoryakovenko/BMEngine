#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCoreTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	namespace PushConstantHandles
	{
		enum
		{
			Model,

			Count
		};
	}

	namespace RenderPassHandles
	{
		enum
		{
			Depth,
			Main,

			Count
		};
	}

	namespace FrameBuffersHandles
	{
		enum
		{
			Tex1,
			Tex2,
			Main,

			Count
		};
	}

	struct BMRPipelineShaderInputDepr
	{
		BMRPipelineHandles Handle = BMRPipelineHandles::PipelineHandlesNone;
		BMRShaderStage Stage = BMRShaderStage::BMRShaderStagesNone;
		const char* EntryPoint = nullptr;
		u32* Code = nullptr;
		u32 CodeSize = 0; 
	};

	struct BMRMainRenderPass
	{
		void SetupPushConstants();
		void CreatePipelineLayouts(VkDevice LogicalDevice, VkDescriptorSetLayout VpLayout,
			VkDescriptorSetLayout EntityLightLayout, VkDescriptorSetLayout LightSpaceMatrixLayout, VkDescriptorSetLayout Material,
			VkDescriptorSetLayout Deferred, VkDescriptorSetLayout ShadowArray, VkDescriptorSetLayout EntitySampler,
			VkDescriptorSetLayout TerrainSkyBoxSampler);
		void CreatePipelinesDepr(VkDevice LogicalDevice, VkExtent2D SwapExtent,
			BMRPipelineShaderInputDepr ShaderInputs[BMRShaderNames::ShaderNamesCount], VkRenderPass main, VkRenderPass depth);

		BMRPipeline Pipelines[BMRPipelineHandles::PipelineHandlesCount];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;
		u32 ActiveLightSpaceMatrixSet = 0;
	};
}
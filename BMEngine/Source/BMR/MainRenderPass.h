#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCoreTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	enum DescriptorLayoutHandles
	{
		TerrainSampler,
		EntitySampler,
		SkyBoxSampler, // TODO Same as TerrainSampler

		Count
	};

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
		void ClearResources(VkDevice LogicalDevice, u32 ImagesCount);

		void SetupPushConstants();
		void CreateDescriptorLayouts(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice, BMRUniformLayout VpLayout,
			BMRUniformLayout EntityLightLayout, BMRUniformLayout LightSpaceMatrixLayout, BMRUniformLayout Material,
			BMRUniformLayout Deferred, BMRUniformLayout ShadowArray);
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
			BMRPipelineShaderInputDepr ShaderInputs[BMRShaderNames::ShaderNamesCount], VkRenderPass main, VkRenderPass depth);

		BMRPipeline Pipelines[BMRPipelineHandles::PipelineHandlesCount];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];
		VkDescriptorSetLayout DescriptorLayouts[DescriptorLayoutHandles::Count];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;
		u32 ActiveLightSpaceMatrixSet = 0;

		// TODO: Fix
		static inline VkDescriptorSet SamplerDescriptors[MAX_IMAGES];

		u32 TextureDescriptorCount = 0;
	};
}
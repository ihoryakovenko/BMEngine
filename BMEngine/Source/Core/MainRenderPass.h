#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	namespace DescriptorLayoutHandles
	{
		enum
		{
			TerrainVp = 0, // TODO Same as EntityVpLayout, change or delete
			TerrainSampler,
			EntityVp,
			EntitySampler,
			Light,
			Material,
			DeferredInput,
			SkyBoxVp, // TODO Same as EntityVpLayout, change or delete
			SkyBoxSampler, // TODO Same as TerrainSampler

			Count
		};
	}

	namespace DescriptorHandles
	{
		enum
		{
			SkyBoxVp,
			TerrainVp,
			EntityVp,
			EntityLigh,
			DeferredInput,

			Count
		};
	}

	namespace PushConstantHandles
	{
		enum
		{
			Model,

			Count
		};
	}

	struct BMRPipelineShaderInput
	{
		BMRPipelineHandles Handle = BMRPipelineHandles::PipelineHandlesNone;
		BMRShaderStages Stage = BMRShaderStages::BMRShaderStagesNone;
		const char* EntryPoint = nullptr;
		u32* Code = nullptr;
		u32 CodeSize = 0; 
	};

	struct BMRMainRenderPass
	{
		void ClearResources(VkDevice LogicalDevice, u32 ImagesCount);

		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void SetupPushConstants();
		void CreateDescriptorLayouts(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice); 
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
			BMRPipelineShaderInput ShaderInputs[BMRShaderNames::ShaderNamesCount]);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			u32 ImagesCount);
		void CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount);

		VkRenderPass RenderPass = nullptr;

		VkPipelineLayout PipelineLayouts[BMRPipelineHandles::PipelineHandlesCount];
		VkPipeline Pipelines[BMRPipelineHandles::PipelineHandlesCount];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];
		VkDescriptorSetLayout DescriptorLayouts[DescriptorLayoutHandles::Count];
		VkDescriptorSet DescriptorsToImages[DescriptorHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;

		BMRImageBuffer ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRImageBuffer DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRGPUBuffer VpUniformBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRGPUBuffer LightBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		// TODO: Fix
		static inline VkDescriptorSet TerrainSamplerDescriptorSets[MAX_IMAGES];
		static inline VkDescriptorSet EntitySamplerDescriptorSets[MAX_IMAGES];
		static inline VkDescriptorSet SkyBoxSamplerDescriptorSets[MAX_IMAGES];

		BMRGPUBuffer MaterialBuffer;
		VkDescriptorSet MaterialSet = nullptr; // TODO: Should be with textures

		u32 TextureDescriptorCountTest = 0;
	};
}
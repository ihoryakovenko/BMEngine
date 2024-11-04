#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCoreTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	enum DescriptorLayoutHandles
	{
		TerrainVp = 0, // TODO Same as EntityVpLayout, change or delete
		TerrainSampler,
		EntityVp,
		EntitySampler,
		EntityLigh,
		EntityMaterial,
		EntityShadowMapSampler,
		DepthEntityLightSpaceMatrix,
		DeferredInputAttachments,
		SkyBoxVp, // TODO Same as EntityVpLayout, change or delete
		SkyBoxSampler, // TODO Same as TerrainSampler

		Count
	};

	namespace DescriptorHandles
	{
		enum
		{
			DepthEntityLightSpaceMatrix = 0,
			ShadowMapSampler,
			SkyBoxVp,
			TerrainVp,
			EntityVp,
			EntityLigh,
			DeferredInputAttachments,

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

	namespace RenderPasses
	{
		enum
		{
			Depth,
			Main,

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
		void CreateImages(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			u32 ImagesCount);
		// TODO: ShadowMapSampler == shit
		void CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount, VkSampler ShadowMapSampler);
		void CreateFrameBuffer(VkDevice LogicalDevice, VkExtent2D FrameBufferSizes, u32 ImagesCount,
			VkImageView SwapchainImageViews[MAX_SWAPCHAIN_IMAGES_COUNT]);

		VkRenderPass RenderPasses[RenderPasses::Count];

		VkPipelineLayout PipelineLayouts[BMRPipelineHandles::PipelineHandlesCount];
		VkPipeline Pipelines[BMRPipelineHandles::PipelineHandlesCount];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];
		VkDescriptorSetLayout DescriptorLayouts[DescriptorLayoutHandles::Count];
		VkDescriptorSet DescriptorsToImages[DescriptorHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];
		VkFramebuffer Framebuffers[RenderPasses::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;
		u32 ActiveLightSpaceMatrixSet = 0;

		BMRImageBuffer ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRImageBuffer DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRGPUBuffer VpUniformBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRGPUBuffer LightBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRImageBuffer ShadowDepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowDepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRGPUBuffer DepthLightSpaceMatrixBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		// TODO: Fix
		static inline VkDescriptorSet SamplerDescriptors[MAX_IMAGES];

		BMRGPUBuffer MaterialBuffer;
		VkDescriptorSet MaterialSet = nullptr; // TODO: Should be with textures

		u32 TextureDescriptorCount = 0;
	};
}
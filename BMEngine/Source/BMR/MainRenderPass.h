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
		EntityShadowMapSampler,
		DeferredInputAttachments,
		SkyBoxSampler, // TODO Same as TerrainSampler

		Count
	};

	namespace DescriptorHandles
	{
		enum
		{
			ShadowMapSampler,
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
			BMRUniformLayout EntityLightLayout, BMRUniformLayout LightSpaceMatrixLayout, BMRUniformLayout Material);
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
			BMRPipelineShaderInputDepr ShaderInputs[BMRShaderNames::ShaderNamesCount], VkRenderPass main, VkRenderPass depth);
		void CreateImages(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		// TODO: ShadowMapSampler == shit
		void CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount, VkSampler ShadowMapSampler);
		void CreateFrameBuffer(VkDevice LogicalDevice, VkExtent2D FrameBufferSizes, u32 ImagesCount,
			VkImageView SwapchainImageViews[MAX_SWAPCHAIN_IMAGES_COUNT], VkRenderPass main, VkRenderPass depth);

		BMRPipeline Pipelines[BMRPipelineHandles::PipelineHandlesCount];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];
		VkDescriptorSetLayout DescriptorLayouts[DescriptorLayoutHandles::Count];
		VkDescriptorSet DescriptorsToImages[DescriptorHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];
		VkFramebuffer Framebuffers[FrameBuffersHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;
		u32 ActiveLightSpaceMatrixSet = 0;

		BMRUniform ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRUniform DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRUniform ShadowDepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowArrayViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowDepthBufferViews1[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowDepthBufferViews2[MAX_SWAPCHAIN_IMAGES_COUNT];

		// TODO: Fix
		static inline VkDescriptorSet SamplerDescriptors[MAX_IMAGES];

		u32 TextureDescriptorCount = 0;
	};
}
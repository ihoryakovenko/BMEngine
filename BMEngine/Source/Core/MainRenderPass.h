#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace Core
{
	struct BrShaderInput
	{
		static VkShaderModule FindShaderModuleByName(ShaderName Name, BrShaderInput* ShaderInputs, u32 ShaderInputsCount);

		ShaderName ShaderName = nullptr;
		VkShaderModule Module = nullptr;
	};

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
			//SkyBoxVb, // TODO Same as EntityVpLayout, change or delete

			Count
		};
	}

	namespace PipelineHandles
	{
		enum
		{
			Terrain = 0,
			Entity,
			Deferred,
			//SkyBoxPipeline,

			Count
		};
	}

	namespace DescriptorHandles
	{
		enum
		{
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
			Entity,

			Count
		};
	}

	struct BrMainRenderPass
	{
		void ClearResources(VkDevice LogicalDevice, u32 ImagesCount);

		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void SetupPushConstants();
		void CreateSamplerSetLayout(VkDevice LogicalDevice);
		void CreateTerrainSetLayout(VkDevice LogicalDevice);
		void CreateEntitySetLayout(VkDevice LogicalDevice);
		void CreateDeferredSetLayout(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice); 
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent, BrShaderInput* ShaderInputs, u32 ShaderInputsCount);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			u32 ImagesCount);
		void CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount);

		VkRenderPass RenderPass = nullptr;

		VkPipelineLayout PipelineLayouts[PipelineHandles::Count];
		VkPipeline Pipelines[PipelineHandles::Count];

		VkPushConstantRange PushConstants[PushConstantHandles::Count];
		VkDescriptorSetLayout DescriptorLayouts[DescriptorLayoutHandles::Count];
		VkDescriptorSet DescriptorsToImages[DescriptorHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		u32 ActiveLightSet = 0;
		u32 ActiveVpSet = 0;

		BrImageBuffer ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BrImageBuffer DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BrGPUBuffer VpUniformBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		BrGPUBuffer LightBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		static inline VkDescriptorSet TerrainSamplerDescriptorSets[MAX_IMAGES];
		static inline VkDescriptorSet EntitySamplerDescriptorSets[MAX_IMAGES];

		BrGPUBuffer MaterialBuffer;
		VkDescriptorSet MaterialSet = nullptr; // TODO: Should be with textures

		u32 TextureDescriptorCountTest = 0;
	};
}
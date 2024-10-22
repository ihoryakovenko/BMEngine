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

	struct BrTerrainSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout TerrainSetLayout = nullptr;
		VkDescriptorSet TerrainSets[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout TerrainSamplerSetLayout = nullptr;
		static inline VkDescriptorSet TerrainSamplerDescriptorSets[MAX_IMAGES];
	};

	struct BrEntitySubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout EntitySetLayout = nullptr;
		VkDescriptorSet EntitySets[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout EntitySamplerSetLayout = nullptr;
		static inline VkDescriptorSet EntitySamplerDescriptorSets[MAX_IMAGES];
	};

	struct BrDeferredSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout DeferredLayout = nullptr;
		VkDescriptorSet DeferredSets[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

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

		BrTerrainSubpass TerrainPass;
		BrEntitySubpass EntityPass;
		BrDeferredSubpass DeferredPass;

		BrImageBuffer ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		BrImageBuffer DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		u32 ActiveVpSet = 0;
		BrGPUBuffer VpUniformBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout LightingSetLayout = nullptr;
		u32 ActiveLightSet = 0;
		VkDescriptorSet LightingSets[MAX_SWAPCHAIN_IMAGES_COUNT];

		BrGPUBuffer LightBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		BrGPUBuffer MaterialBuffer;
		VkDescriptorSetLayout MaterialLayout = nullptr;
		VkDescriptorSet MaterialSet = nullptr;

		u32 TextureDescriptorCountTest = 0;
	};
}
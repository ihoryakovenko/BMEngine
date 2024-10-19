#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace Core
{
	struct ShaderInput
	{
		static VkShaderModule FindShaderModuleByName(ShaderName Name, ShaderInput* ShaderInputs, u32 ShaderInputsCount);

		ShaderName ShaderName = nullptr;
		VkShaderModule Module = nullptr;
	};

	struct TerrainSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout TerrainSetLayout = nullptr;
		VkDescriptorSet TerrainSets[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct EntitySubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout EntitySetLayout = nullptr;
		VkDescriptorSet EntitySets[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct DeferredSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout DeferredLayout = nullptr;
		VkDescriptorSet DeferredSets[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct MainRenderPass
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
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent, ShaderInput* ShaderInputs, u32 ShaderInputsCount);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			u32 ImagesCount);
		void CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount);

		VkRenderPass RenderPass = nullptr;

		TerrainSubpass TerrainPass;
		EntitySubpass EntityPass;
		DeferredSubpass DeferredPass;

		ImageBuffer ColorBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ColorBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		ImageBuffer DepthBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DepthBufferViews[MAX_SWAPCHAIN_IMAGES_COUNT];

		GPUBuffer VpUniformBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout LightingSetLayout = nullptr;
		VkDescriptorSet LightingSets[MAX_SWAPCHAIN_IMAGES_COUNT];

		GPUBuffer AmbientLightingBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		GPUBuffer PointLightingBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout SamplerSetLayout = nullptr;
	};
}
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCoreTypes.h"

namespace Core
{
	struct TerrainSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout TerrainSetLayout = nullptr;
		VkDescriptorSet* TerrainSets = nullptr;
	};

	struct EntitySubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout SamplerSetLayout = nullptr;
		VkDescriptorSetLayout EntitySetLayout = nullptr;
		VkDescriptorSet* EntitySets = nullptr;
	};

	struct DeferredSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout DeferredLayout = nullptr;
		VkDescriptorSet* DeferredSets = nullptr;
	};

	struct MainRenderPass
	{
		static void GetPoolSizes(uint32_t TotalImagesCount, std::vector<VkDescriptorPoolSize>& TotalPassPoolSizes,
			uint32_t& TotalDescriptorCount);

		void ClearResources(VkDevice LogicalDevice, uint32_t ImagesCount);

		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void SetupPushConstants();
		void CreateSamplerSetLayout(VkDevice LogicalDevice);
		void CreateCommandPool(VkDevice LogicalDevice, uint32_t FamilyIndex);
		void CreateTerrainSetLayout(VkDevice LogicalDevice);
		void CreateEntitySetLayout(VkDevice LogicalDevice);
		void CreateDeferredSetLayout(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice);
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, uint32_t ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			uint32_t ImagesCount);
		void CreateSets(VkDevice LogicalDevice, VkDescriptorPool DescriptorPool, uint32_t ImagesCount);

		VkRenderPass RenderPass = nullptr;

		TerrainSubpass TerrainPass;
		EntitySubpass EntityPass;
		DeferredSubpass DeferredPass;

		VkCommandPool GraphicsCommandPool = nullptr;

		ImageBuffer* ColorBuffers = nullptr;
		ImageBuffer* DepthBuffers = nullptr;
		GPUBuffer* VpUniformBuffers = nullptr;
	};
}
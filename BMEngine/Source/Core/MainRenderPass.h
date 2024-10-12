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
		VkDescriptorSet* TerrainSets = nullptr;
	};

	struct EntitySubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkPushConstantRange PushConstantRange;
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
		void ClearResources(VkDevice LogicalDevice, u32 ImagesCount);

		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void SetupPushConstants();
		void CreateSamplerSetLayout(VkDevice LogicalDevice);
		void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex);
		void CreateTerrainSetLayout(VkDevice LogicalDevice);
		void CreateEntitySetLayout(VkDevice LogicalDevice);
		void CreateDeferredSetLayout(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice); 
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent, ShaderInput* ShaderInputs, u32 ShaderInputsCount);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			u32 ImagesCount);
		void CreateSets(VkDevice LogicalDevice, u32 ImagesCount);

		VkRenderPass RenderPass = nullptr;

		TerrainSubpass TerrainPass;
		EntitySubpass EntityPass;
		DeferredSubpass DeferredPass;

		VkCommandPool GraphicsCommandPool = nullptr;

		ImageBuffer* ColorBuffers = nullptr;
		ImageBuffer* DepthBuffers = nullptr;
		VkBuffer* VpUniformBuffers = nullptr;

		VkDescriptorSetLayout SamplerSetLayout = nullptr;
	};
}
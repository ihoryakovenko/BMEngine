#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "VulkanCoreTypes.h"

namespace Core
{
	struct MainRenderPass
	{
		void Init(VkDevice InLogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat, int GraphicsFamily, uint32_t MaxDrawFrames,
			VkExtent2D* SwapExtents, uint32_t SwapExtentsCount);

		void DeInit();

		VkDevice LogicalDevice = nullptr;
		VkRenderPass RenderPass = nullptr;

		uint32_t MaxFrameDrawsCounter = 0;
		VkSemaphore* ImageAvailable = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout SamplerSetLayout = nullptr;
		VkDescriptorSetLayout DescriptorSetLayout = nullptr;
		VkDescriptorSetLayout InputSetLayout = nullptr; // Set for different pipeline, goes to different shader

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipelineLayout SecondPipelineLayout = nullptr;

		VkPipeline GraphicsPipeline = nullptr;
		VkPipeline SecondPipeline = nullptr;

		VkCommandPool GraphicsCommandPool = nullptr;

	private:
		void CreateVulkanPass(VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat);
		void CreateSamplerSetLayout();
		void CreateCommandPool(uint32_t FamilyIndex);
		void CreateSynchronisation(uint32_t MaxFrameDraws);
		void DestroyRenderPassSynchronisationData();
		void CreateDescriptorSetLayout();
		void CreateInputSetLayout();
		void CreatePipelineLayout();
		void CreatePipelines(VkExtent2D* SwapExtents, uint32_t SwapExtentsCount);
	};
}
#pragma once





//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////








#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <mutex>

struct GLFWwindow;

namespace VulkanInterface
{
	struct UniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformImage
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct RenderPipeline
	{
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	// TO refactor
	u32 GetImageCount();
	VkImageView* GetSwapchainImageViews();
	VkImage* GetSwapchainImages();
	u32 TestGetImageIndex();

	void WaitDevice();




	typedef UniformBuffer IndexBuffer;
	typedef UniformBuffer VertexBuffer;

	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkCommandBuffer GetCommandBuffer();
	VkQueue GetTransferQueue();
	VkQueue GetGraphicsQueue();
	VkFormat GetSurfaceFormat();
	u32 GetQueueGraphicsFamilyIndex();
	VkInstance GetInstance();
	VkCommandPool GetTransferCommandPool();
	VkSwapchainKHR GetSwapchain();


}

#pragma once

#include "Util/EngineTypes.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace VulkanCoreContext
{
	static const u32 MAX_SWAPCHAIN_IMAGES_COUNT = 3;

	struct VulkanCoreContext
	{
		VkInstance VulkanInstance;
		VkDebugUtilsMessengerEXT DebugMessenger;

		VkPhysicalDevice PhysicalDevice;
		VkDevice LogicalDevice;
		VulkanHelper::PhysicalDeviceIndices Indices;

		VkSwapchainKHR VulkanSwapchain;
		u32 ImagesCount;
		VkImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImage Images[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent;

		VkSurfaceKHR Surface;
		VkSurfaceFormatKHR SurfaceFormat;
		GLFWwindow* WindowHandler;

		VkQueue GraphicsQueue;
	};

	void CreateCoreContext(VulkanCoreContext* Context, GLFWwindow* Window);
	void DestroyCoreContext(VulkanCoreContext* Context);
}

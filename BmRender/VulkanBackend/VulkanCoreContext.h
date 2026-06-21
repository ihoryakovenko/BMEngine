#pragma once

#include "VulkanHelper.h"

#include <vulkan/vulkan.h>

#define MAX_SWAPCHAIN_IMAGES_COUNT  3

struct VulkanCoreContext
{
	VkInstance VulkanInstance;
	VkDebugUtilsMessengerEXT DebugMessenger;

	VkPhysicalDevice PhysicalDevice;
	VkDevice LogicalDevice;
	PhysicalDeviceIndices Indices;

	VkSwapchainKHR VulkanSwapchain;
	u32 ImagesCount;
	BmRender_ImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
	BmRender_Image Images[MAX_SWAPCHAIN_IMAGES_COUNT];
	VkExtent2D SwapExtent;

	VkSurfaceKHR Surface;
	VkSurfaceFormatKHR SurfaceFormat;
	void* WindowHandle;
};

void CreateCoreContext(VulkanCoreContext* Context, void* Window);
void DestroyCoreContext(VulkanCoreContext* Context);

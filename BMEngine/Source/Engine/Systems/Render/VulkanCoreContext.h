#pragma once

#include "Util/EngineTypes.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Engine/Systems/Render/RenderInterface.h"

#include <mutex>

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
		PhysicalDeviceIndices Indices;

		VkSwapchainKHR VulkanSwapchain;
		u32 ImagesCount;
		BmRender_ImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		BmRender_Image Images[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent;

		VkSurfaceKHR Surface;
		VkSurfaceFormatKHR SurfaceFormat;
		GLFWwindow* WindowHandler;
	};

	void CreateCoreContext(VulkanCoreContext* Context, GLFWwindow* Window);
	void DestroyCoreContext(VulkanCoreContext* Context);
}

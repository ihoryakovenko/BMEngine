#pragma once

#include <vector>

#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct QueueFamilyIndices
	{
		int _GraphicsFamily = -1;
		int _PresentationFamily = -1;

		bool IsValid() const
		{
			return _GraphicsFamily >= 0 && _PresentationFamily >= 0;
		}

		bool CreateQueueFamilyIndices(VkPhysicalDevice Device, VkSurfaceKHR Surface);
	};

	struct SwapchainDetails
	{
		VkSurfaceCapabilitiesKHR _SurfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> _Formats;
		std::vector<VkPresentModeKHR> _PresentationModes;

		bool CreateSwapchainDetails(VkPhysicalDevice Device, VkSurfaceKHR Surface);

		VkSurfaceFormatKHR GetBestSurfaceFormat() const;
		VkPresentModeKHR GetBestPresentationMode() const;
		VkExtent2D GetBestSwapExtent(const Window& Window) const;
	};

	struct SwapchainImage
	{
		VkImage _Image = nullptr;
		VkImageView _ImageView = nullptr;

		bool CreateSwapchainImage(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
			VkImageAspectFlags AspectFlags);
	};
}

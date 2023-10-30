#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct MainDevice
	{
		int _GraphicsFamily = -1;
		int _PresentationFamily = -1;

		VkPhysicalDevice _PhysicalDevice = nullptr;
		VkDevice _LogicalDevice = nullptr;

		VkSurfaceCapabilitiesKHR _SurfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> _Formats;
		std::vector<VkPresentModeKHR> _PresentationModes;

		VkSurfaceFormatKHR GetBestSurfaceFormat() const;
		VkPresentModeKHR GetBestPresentationMode() const;
		VkExtent2D GetBestSwapExtent(GLFWwindow* Window) const;

		static inline std::vector<const char*> _DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		bool SetupPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface);
		bool CreateLogicalDevice();
	};
}

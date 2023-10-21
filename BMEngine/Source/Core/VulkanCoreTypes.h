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

	struct MainDevice
	{
		VkPhysicalDevice _PhysicalDevice = nullptr;
		VkDevice _LogicalDevice = nullptr;

		QueueFamilyIndices _PhysicalDeviceIndices;
		SwapchainDetails _SwapchainDetails;

		static inline std::vector<const char*> _DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		bool SetupPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface);
		bool CreateLogicalDevice();

		bool IsDeviceExtensionSupported(const char* Extension) const;
		bool IsDeviceSuitable(VkSurfaceKHR Surface); // todo validate
	};

	struct Queues
	{
		VkQueue _GraphicsQueue = nullptr;
		VkQueue _PresentationQueue = nullptr;

		void SetupQueues(VkDevice LogicalDevice, QueueFamilyIndices Indices);
	};

	struct Swapchain
	{
		VkSwapchainKHR _VulkanSwapchain = nullptr;
		std::vector<SwapchainImage> _SwapchainImages;
		VkFormat _SwapchainImageFormat{};
		VkExtent2D _SwapchainExtent{};

		bool CreateSwapchain(MainDevice& Device, Window ActiveWindow, VkSurfaceKHR Surface);
		void CleanupSwapchain(VkDevice _LogicalDevice);
	};

	struct Instance
	{
		VkInstance _VulkanInstance = nullptr;

		bool CreateInstance();

		bool IsInstanceExtensionSupported(const char* Extension) const;
		bool GetRequiredInstanceExtensions(std::vector<const char*>& InstanceExtensions) const;
	};

	struct GraphicsPipeline
	{
		bool CreateGraphicsPipeline();
	};
}

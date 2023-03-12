#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "VulkanUtil.h"

namespace Core
{
	class Window;

	class VulkanRender
	{
	public:
		bool Init(Window* NewWindow);
		void Cleanup();

	private:
		bool CreateInstance();
		bool CreateLogicalDevice();

		bool SetupPhysicalDevice();

	private:
		Window* _Window = nullptr;
		VkInstance _Instance = nullptr;
		VkPhysicalDevice _PhysicalDevice = nullptr;
		QueueFamilyIndices _PhysicalDeviceIndices;
		SwapchainDetails _SwapchainDetails;
		VkDevice _LogicalDevice = nullptr;
		VkQueue _GraphicsQueue = nullptr;
		VkQueue _PresentationQueue = nullptr;
		VkSurfaceKHR _Surface = nullptr;
	};
}

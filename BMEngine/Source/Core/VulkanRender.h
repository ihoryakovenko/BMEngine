#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Window.h"

namespace Core
{
	struct QueueFamilyIndices
	{
		int _GraphicsFamily = -1;

		bool IsValid() const
		{
			return _GraphicsFamily >= 0;
		}
	};

	class VulkanRender
	{
	public:
		bool Init(Window* NewWindow);
		void Cleanup();

	private:
		bool CreateInstance();
		bool CreateLogicalDevice();

		bool SetupPhysicalDevice();
		QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice Device) const;

		bool IsInstanceExtensionSupported(const char* Extension) const;
		bool IsDeviceSuitable(VkPhysicalDevice Device) const;
	private:
		Window* _Window = nullptr;
		VkInstance _Instance = nullptr;
		VkPhysicalDevice _PhysicalDevice = nullptr;
		VkDevice _LogicalDevice = nullptr;
		VkQueue GraphicsQueue = nullptr;
	};
}

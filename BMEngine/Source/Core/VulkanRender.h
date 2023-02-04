#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Window.h"

namespace Core
{
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
		VkDevice _LogicalDevice = nullptr;
		VkQueue GraphicsQueue = nullptr;
	};
}

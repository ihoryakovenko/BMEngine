#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "VulkanUtil.h"
#include "VulkanCoreTypes.h"

namespace Core
{
	class Window;

	class VulkanRender
	{
	public:
		bool Init(Window* NewWindow);
		void Cleanup();

	private:
		Window* _Window = nullptr;
		VkSurfaceKHR _Surface = nullptr;
		Queues _Queues;
		Instance _Instance;
		MainDevice _MainDevice;
		Swapchain _Swapchain;
	};
}

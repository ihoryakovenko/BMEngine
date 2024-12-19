#pragma once

// TODO: use windows API only
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "Util/EngineTypes.h"

namespace Platform
{
	typedef GLFWwindow BMRTMPWindowHandler;
	typedef HWND BMRWindowHandler;
	static const char WinWindowExtension[] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

	BMRTMPWindowHandler* CreatePlatformWindow(s32 WindowWidth, s32 WindowHeight);

	VkResult CreateSurface(BMRWindowHandler WindowHandler, VkInstance VulkanInstance, VkSurfaceKHR* Surface);
	void GetWindowSizes(BMRWindowHandler WindowHandler, u32* Width, u32* Height);

	void DisableCursor(BMRTMPWindowHandler* Window);
}

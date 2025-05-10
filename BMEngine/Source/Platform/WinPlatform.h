#pragma once

#include <httplib.h>

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
	static const char WinWindowExtension[] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

	GLFWwindow* CreatePlatformWindow(s32 WindowWidth, s32 WindowHeight);

	VkResult CreateSurface(GLFWwindow* WindowHandler, VkInstance VulkanInstance, VkSurfaceKHR* Surface);
	void GetWindowSizes(GLFWwindow* WindowHandler, u32* Width, u32* Height);

	void DisableCursor(GLFWwindow* Window);
}

#include "WinPlatform.h"

namespace Platform
{
	GLFWwindow* CreatePlatformWindow(s32 WindowWidth, s32 WindowHeight)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		return glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
	}

	VkResult CreateSurface(GLFWwindow* WindowHandler, VkInstance VulkanInstance, VkSurfaceKHR* Surface)
	{
		return glfwCreateWindowSurface(VulkanInstance, WindowHandler, nullptr, Surface);
	}

	void GetWindowSizes(GLFWwindow* WindowHandler, u32* Width, u32* Height)
	{
		int width, height;
		glfwGetFramebufferSize(WindowHandler, &width, &height);
		*Width = static_cast<u32>(width);
		*Height = static_cast<u32>(height);
	}

	void DisableCursor(GLFWwindow* Window)
	{
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}
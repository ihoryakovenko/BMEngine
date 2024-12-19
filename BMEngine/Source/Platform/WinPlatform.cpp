#include "WinPlatform.h"

namespace Platform
{
	BMRTMPWindowHandler* CreatePlatformWindow(s32 WindowWidth, s32 WindowHeight)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		return glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
	}

	VkResult CreateSurface(BMRWindowHandler WindowHandler, VkInstance VulkanInstance, VkSurfaceKHR* Surface)
	{
		VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = { };
		SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		SurfaceCreateInfo.hwnd = WindowHandler;
		SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

		return vkCreateWin32SurfaceKHR(VulkanInstance, &SurfaceCreateInfo, nullptr, Surface);
	}

	void GetWindowSizes(BMRWindowHandler WindowHandler, u32* Width, u32* Height)
	{
		RECT Rect;
		if (!GetClientRect(WindowHandler, &Rect))
		{
			*Width = 0;
			*Height = 0;

			return;
		}

		*Width = Rect.right - Rect.left;
		*Height = Rect.bottom - Rect.top;
	}

	void DisableCursor(BMRTMPWindowHandler* Window)
	{
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}
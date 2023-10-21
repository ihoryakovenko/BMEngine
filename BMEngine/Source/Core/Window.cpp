#include "Window.h"
#include "Util/Log.h"

namespace Core
{
	bool Window::Init(const std::string& Name, int Width, int Height)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		_Window = glfwCreateWindow(Width, Height, Name.c_str(), nullptr, nullptr);
		if (_Window == nullptr)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		return true;
	}

	void Window::Destroy()
	{
		glfwDestroyWindow(_Window);
	}

	VkSurfaceKHR Window::CreateSurface(VkInstance Instance, const VkAllocationCallbacks* Allocator) const
	{
		VkSurfaceKHR Surface;
		const VkResult Result = glfwCreateWindowSurface(Instance, _Window, Allocator, &Surface);
		if (Result != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			return nullptr;
		}

		return Surface;
	}

	void Window::GetWindowSize(int& Width, int& Height) const
	{
		glfwGetFramebufferSize(_Window, &Width, &Height);
	}
}

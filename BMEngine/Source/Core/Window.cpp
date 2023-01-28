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
			Util::GlfwCheckError();
			return false;
		}

		return true;
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(_Window);
	}

	void Window::PollEvents() const
	{
		glfwPollEvents();
	}

	void Window::Destroy()
	{
		glfwDestroyWindow(_Window);
	}
}

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine.h"
#include "Util/Log.h"

namespace Core
{
	bool Engine::Init()
	{
		const int InitResult = glfwInit();
		if (InitResult == GL_FALSE)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		if (!_Window.Init())
		{
			return false;
		}
		
		if (!_VulkanRender.Init(&_Window))
		{
			return false;
		}

		return true;
	}

	void Engine::GameLoop()
	{
		while (!_Window.ShouldClose())
		{
			_Window.PollEvents();
		}
	}

	void Engine::Terminate()
	{
		_VulkanRender.Cleanup();
		_Window.Destroy();
		glfwTerminate();
	}
}

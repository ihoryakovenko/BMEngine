#include <string>
#include <iostream>
#include <cassert>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Log.h"

namespace Util
{
	void GlfwCheckError()
	{
		const char* ErrorDescription;
		const int LastErrorCode = glfwGetError(&ErrorDescription);

		if (LastErrorCode != GLFW_NO_ERROR)
		{
			std::cout << ErrorDescription;
		}
		else
		{
			std::cout << "No GLFW error";
		}
	}
}

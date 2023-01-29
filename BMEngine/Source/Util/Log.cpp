#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Log.h"

namespace Util
{
	void GlfwLogError()
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

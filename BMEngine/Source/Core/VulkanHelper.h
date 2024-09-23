#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	bool CreateShader(VkDevice LogicalDevice, const uint32_t* Code, uint32_t CodeSize,
		VkShaderModule& VertexShaderModule);
}

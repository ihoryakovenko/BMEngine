#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	class Window
	{
	public:
		bool Init(const std::string& Name = "BMEngine", int Width = 800, int Height = 600);
		bool ShouldClose() const;
		void PollEvents() const;
		void Destroy();
		VkSurfaceKHR CreateSurface(VkInstance Instance, const VkAllocationCallbacks* Allocator) const;

	private:
		GLFWwindow* _Window = nullptr;
	};
}

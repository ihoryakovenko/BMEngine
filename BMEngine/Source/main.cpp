//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <cassert>
#include <algorithm>

#include "Core/VulkanCoreTypes.h"

int main()
{
	if (glfwInit() == GL_FALSE)
	{
		Util::Log().Error("glfwInit result is GL_FALSE");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(800, 600, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log().GlfwLogError();
		glfwTerminate();
		return -1;
	}

	Core::MainInstance Instance;
	Core::InitMainInstance(Instance, Util::EnableValidationLayers);

	Core::VulkanRenderInstance RenderInstance;
	Core::InitVulkanRenderInstance(RenderInstance, Instance.VulkanInstance, Window);

	// Mesh
	const uint32_t MeshVerticesCount = 3;
	Core::Vertex MeshVertices[MeshVerticesCount] = {
		{{0.0, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},
		{{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
		{{-0.4, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}}
	};

	Core::LoadVertices(RenderInstance, MeshVertices, MeshVerticesCount);
	Core::RecordCommands(RenderInstance);

	while (!glfwWindowShouldClose(RenderInstance.Window))
	{
		glfwPollEvents();

		Core::Draw(RenderInstance);
	}

	Core::DeinitVulkanRenderInstance(RenderInstance);
	Core::DeinitMainInstance(Instance);

	glfwDestroyWindow(Window);

	glfwTerminate();

	if (Util::Memory::AllocateCounter != 0)
	{
		Util::Log().Error("AllocateCounter in not equal 0, counter is {}", Util::Memory::AllocateCounter);
		assert(false);
	}

	return 0;
}
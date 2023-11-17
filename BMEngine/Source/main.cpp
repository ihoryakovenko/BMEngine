//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <cassert>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

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

	//Mesh
	const uint32_t MeshVerticesCount = 4;
	Core::Vertex MeshVertices[MeshVerticesCount] = {
			{ { -0.1, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
			{ { -0.1, 0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
			{ { -0.9, 0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
			{ { -0.9, -0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } }   // 3
	};

	const uint32_t MeshIndicesCount = 6;
	uint32_t MeshIndices[MeshIndicesCount] = {
		0, 1, 2,
		2, 3, 0
	};

	Core::Mesh Mesh;
	Mesh.MeshVerticesCount = MeshVerticesCount;
	Mesh.MeshVertices = MeshVertices;
	Mesh.MeshIndicesCount = MeshIndicesCount;
	Mesh.MeshIndices = MeshIndices;

	Core::LoadMesh(RenderInstance, Mesh);

	Core::Vertex MeshVertices2[MeshVerticesCount] = {
		{ { 0.9, -0.3, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
		{ { 0.9, 0.1, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
		{ { 0.1, 0.3, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
		{ { 0.1, -0.3, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
	};

	Mesh.MeshVertices = MeshVertices2;

	Core::LoadMesh(RenderInstance, Mesh);
	Core::RecordCommands(RenderInstance);

	float Angle = 0.0f;
	double DeltaTime = 0.0f;
	double LastTime = 0.0f;

	while (!glfwWindowShouldClose(RenderInstance.Window))
	{
		glfwPollEvents();

		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<float>(CurrentTime);

		Angle += 60.0f * static_cast<float>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		RenderInstance.Mvp.Model = glm::rotate(glm::mat4(1.0f), glm::radians(Angle), glm::vec3(0.0f, 0.0f, 1.0f));

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
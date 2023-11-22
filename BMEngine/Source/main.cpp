//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
		Util::Log::Error("glfwInit result is GL_FALSE");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(800, 600, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log::GlfwLogError();
		glfwTerminate();
		return -1;
	}

	Core::MainInstance Instance;
	Core::InitMainInstance(Instance, Util::EnableValidationLayers);

	Core::VulkanRenderInstance RenderInstance;
	Core::InitVulkanRenderInstance(RenderInstance, Instance.VulkanInstance, Window);

	// Function LoadTexture
	int Width, Height;
	uint64_t ImageSize; // Todo: DeviceSize?
	int Channels;

	const char* TestTexture = "./Resources/Textures/giraffe.jpg";
	stbi_uc* ImageData = stbi_load(TestTexture, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		return -1;
	}

	ImageSize = Width * Height * 4;
	// Function end LoadTexture

	Core::CreateTexture(RenderInstance, ImageData, Width, Height, ImageSize);

	stbi_image_free(ImageData);

	//Mesh
	const uint32_t MeshVerticesCount = 4;
	Core::Vertex MeshVertices[MeshVerticesCount] = {
			{ { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
			{ { -0.4, -0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
			{ { 0.4, -0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
			{ { 0.4, 0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
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
			{ { -0.25, 0.6, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
			{ { -0.25, -0.6, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
			{ { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
			{ { 0.25, 0.6, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
	};

	Mesh.MeshVertices = MeshVertices2;

	Core::LoadMesh(RenderInstance, Mesh);

	float Angle = 0.0f;
	double DeltaTime = 0.0f;
	double LastTime = 0.0f;

	float XMove = 0.0f;
	float MoveScale = 4.0f;

	while (!glfwWindowShouldClose(RenderInstance.Window))
	{
		glfwPollEvents();

		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<float>(CurrentTime);

		Angle += 10.0f * static_cast<float>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);

		firstModel = glm::translate(firstModel, glm::vec3(0.0f, -0.5f, -2.0f));
		firstModel = glm::rotate(firstModel, glm::radians(Angle), glm::vec3(0.0f, 0.0f, 1.0f));

		secondModel = glm::translate(secondModel, glm::vec3(0.0f, 0.0f, -2.5f));
		secondModel = glm::rotate(secondModel, glm::radians(-Angle * 100), glm::vec3(0.0f, 0.0f, 1.0f));

		RenderInstance.DrawableObjects[0].Model = firstModel;
		RenderInstance.DrawableObjects[1].Model = secondModel;

		XMove += MoveScale * static_cast<float>(DeltaTime);
		if (std::abs(XMove) >= 8.0f)
		{
			MoveScale *= -1;
		}

		RenderInstance.ViewProjection.View = glm::lookAt(glm::vec3(XMove, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -6.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		Core::Draw(RenderInstance);
	}

	Core::DeinitVulkanRenderInstance(RenderInstance);
	Core::DeinitMainInstance(Instance);

	glfwDestroyWindow(Window);

	glfwTerminate();

	if (Util::Memory::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Util::Memory::AllocateCounter);
		assert(false);
	}

	return 0;
}
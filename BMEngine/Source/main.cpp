//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <cassert>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "Core/VulkanRenderingSystem.h"

#include <tiny_obj_loader.h>

#include <unordered_map>

#include "Core/VulkanCoreTypes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct TestMesh
{
	std::vector<Core::Vertex> vertices;
	std::vector<uint32_t> indices;
	int TextureId;
};

template<> struct std::hash<Core::Vertex>
{
	size_t operator()(Core::Vertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.Position) ^
			(std::hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
			(std::hash<glm::vec2>()(vertex.TextureCoords) << 1);
	}
};

struct VertexEqual
{
	bool operator()(const Core::Vertex& lhs, const Core::Vertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.Color == rhs.Color && lhs.TextureCoords == rhs.TextureCoords;
	}
};

struct Camera
{
	glm::vec3 CameraPosition = glm::vec3(0.0f, 0.0f, 20.0f);
	glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

void AddTexture(Core::VulkanRenderingSystem& RenderingSystem, const char* TexturePath)
{
	int Width, Height;
	uint64_t ImageSize; // Todo: DeviceSize?
	int Channels;

	stbi_uc* ImageData = stbi_load(TexturePath, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		return;
	}

	ImageSize = Width * Height * 4;

	RenderingSystem.CreateImageBuffer(ImageData, Width, Height, ImageSize);

	stbi_image_free(ImageData);
}

Core::VulkanRenderingSystem* ins;
void WindowCloseCallback(GLFWwindow* Window)
{
	ins->RemoveViewport(Window);
	glfwDestroyWindow(Window);
}

int main()
{
	if (glfwInit() == GL_FALSE)
	{
		Util::Log::Error("glfwInit result is GL_FALSE");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(1600, 800, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log::GlfwLogError();
		glfwTerminate();
		return -1;
	}

	GLFWwindow* Window2 = glfwCreateWindow(1600, 800, "BMEngine2", nullptr, nullptr);
	if (Window2 == nullptr)
	{
		Util::Log::GlfwLogError();
		glfwTerminate();
		return -1;
	}

	Core::VulkanRenderingSystem RenderingSystem;
	RenderingSystem.Init(Window, Window2);

	ins = &RenderingSystem;
	


	RenderingSystem.Viewports[0]->ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		static_cast<float>(RenderingSystem.Viewports[0]->SwapExtent.width) / static_cast<float>(RenderingSystem.Viewports[0]->SwapExtent.height), 0.1f, 100.0f);

	RenderingSystem.Viewports[0]->ViewProjection.Projection[1][1] *= -1;

	RenderingSystem.Viewports[0]->ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const char* TestTexture = "./Resources/Textures/giraffe.jpg";
	AddTexture(RenderingSystem, TestTexture);

	const char* Modelpath = "./Resources/Models/uh60.obj";
	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		return -1;
	}

	std::vector<int> MaterialToTexture(Materials.size());

	for (size_t i = 0; i < Materials.size(); i++)
	{
		MaterialToTexture[i] = 0;
		const tinyobj::material_t& Material = Materials[i];
		if (!Material.diffuse_texname.empty())
		{
			int Idx = Material.diffuse_texname.rfind("\\");
			std::string FileName = "./Resources/Textures/" + Material.diffuse_texname.substr(Idx + 1);

			MaterialToTexture[i] = RenderingSystem.TextureImagesCount;
			AddTexture(RenderingSystem, FileName.c_str());
		}
	}

	std::vector<TestMesh> ModelMeshes;
	ModelMeshes.reserve(Shapes.size());

	std::unordered_map < Core::Vertex, uint32_t, std::hash<Core::Vertex>, VertexEqual> uniqueVertices{};

	for (const auto& Shape : Shapes)
	{
		TestMesh Tm;

		for (const auto& index : Shape.mesh.indices)
		{
			Core::Vertex vertex{};

			vertex.Position =
			{
				Attrib.vertices[3 * index.vertex_index + 0],
				Attrib.vertices[3 * index.vertex_index + 1],
				Attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.TextureCoords =
			{
				Attrib.texcoords[2 * index.texcoord_index + 0],
				Attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(Tm.vertices.size());
				Tm.vertices.push_back(vertex);
			}
			
			Tm.indices.push_back(uniqueVertices[vertex]);
		}

		Tm.TextureId = MaterialToTexture[Shape.mesh.material_ids[0]];

		ModelMeshes.push_back(Tm);
	}

	for (int i = 0; i < ModelMeshes.size(); ++i)
	{
		Core::Mesh m;
		m.MeshVertices = ModelMeshes[i].vertices.data();
		m.MeshVerticesCount = ModelMeshes[i].vertices.size();


		m.MeshIndices = ModelMeshes[i].indices.data();
		m.MeshIndicesCount = ModelMeshes[i].indices.size();

		RenderingSystem.LoadMesh(m);
		RenderingSystem.DrawableObjects[RenderingSystem.DrawableObjectsCount - 1].TextureId = ModelMeshes[i].TextureId;
	}

	float Angle = 0.0f;
	double DeltaTime = 0.0f;
	double LastTime = 0.0f;

	const float RotationSpeed = 2.0f;
	const float CameraSpeed = 10.0f;

	RenderingSystem.Viewports[1]->ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		static_cast<float>(RenderingSystem.Viewports[1]->SwapExtent.width) / static_cast<float>(RenderingSystem.Viewports[1]->SwapExtent.height), 0.1f, 100.0f);
	RenderingSystem.Viewports[1]->ViewProjection.Projection[1][1] *= -1;
	RenderingSystem.Viewports[1]->ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	Camera Cameras[2];

	glfwSetWindowCloseCallback(Window2, WindowCloseCallback);

	while (!glfwWindowShouldClose(RenderingSystem.Viewports[0]->Window))
	{
		glfwPollEvents();

		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<float>(CurrentTime);

		float CameraDeltaSpeed = CameraSpeed * DeltaTime;
		float CameraDeltaRotationSpeed = RotationSpeed * DeltaTime;

		for (int ViewportIndex = 0; ViewportIndex < RenderingSystem.ViewportsCount; ++ViewportIndex)
		{
			Core::ViewportInstance* ProcessedViewport = RenderingSystem.Viewports[ViewportIndex];

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_W) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition += CameraDeltaSpeed * Cameras[ViewportIndex].CameraFront;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_S) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition -= CameraDeltaSpeed * Cameras[ViewportIndex].CameraFront;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_A) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition -= glm::normalize(glm::cross(Cameras[ViewportIndex].CameraFront, Cameras[ViewportIndex].CameraUp)) * CameraDeltaSpeed;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_D) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition += glm::normalize(glm::cross(Cameras[ViewportIndex].CameraFront, Cameras[ViewportIndex].CameraUp)) * CameraDeltaSpeed;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_SPACE) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition += CameraDeltaSpeed * Cameras[ViewportIndex].CameraUp;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			{
				Cameras[ViewportIndex].CameraPosition -= CameraDeltaSpeed * Cameras[ViewportIndex].CameraUp;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_Q) == GLFW_PRESS)
			{
				glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), CameraDeltaRotationSpeed, Cameras[ViewportIndex].CameraUp);
				Cameras[ViewportIndex].CameraFront = glm::mat3(rotation) * Cameras[ViewportIndex].CameraFront;
			}

			if (glfwGetKey(ProcessedViewport->Window, GLFW_KEY_E) == GLFW_PRESS)
			{
				glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -CameraDeltaRotationSpeed, Cameras[ViewportIndex].CameraUp);
				Cameras[ViewportIndex].CameraFront = glm::mat3(rotation) * Cameras[ViewportIndex].CameraFront;
			}

			ProcessedViewport->ViewProjection.View = glm::lookAt(Cameras[ViewportIndex].CameraPosition, Cameras[ViewportIndex].CameraPosition + Cameras[ViewportIndex].CameraFront, Cameras[ViewportIndex].CameraUp);
		}

		//Angle += 30.0f * static_cast<float>(DeltaTime);
		//if (Angle > 360.0f)
		//{
		//	Angle -= 360.0f;
		//}

		//glm::mat4 TestMat = glm::rotate(glm::mat4(1.0f), glm::radians(Angle), glm::vec3(0.0f, 1.0f, 0.0f));

		//for (int i = 0; i < DrawableObjectsCount; ++i)
		//{
		//	DrawableObjects[i].Model = TestMat;
		//}

		RenderingSystem.Draw();
	}

	RenderingSystem.DeInit();


	glfwDestroyWindow(Window);

	glfwTerminate();

	if (Util::Memory::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Util::Memory::AllocateCounter);
		assert(false);
	}

	return 0;
}
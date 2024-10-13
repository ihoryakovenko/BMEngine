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
#include <random>
#include "Memory/MemoryManagmentSystem.h"
#include "Util/EngineTypes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

const u32 NumRows = 600;
const u32 NumCols = 600;

Core::TerrainVertex TerrainVerticesData[NumRows][NumCols];

std::vector<Core::DrawEntity> DrawEntities;
Core::ShaderCodeDescription ShaderCodeDescriptions[Core::ShaderNames::ShadersCount];
std::vector<std::vector<char>> ShaderCodes(Core::ShaderNames::ShadersCount);

Core::VulkanRenderingSystem RenderingSystem;

void GenerateTerrain()
{
	const float MaxAltitude = 10.0f;
	const float MinAltitude = 0.0f;
	const float SmoothMin = 7.0f;
	const float SmoothMax = 3.0f;
	const float SmoothFactor = 0.5f;
	const float ScaleFactor = 0.2f;

	std::mt19937 Gen(1);
	std::uniform_real_distribution<float> Dist(MinAltitude, MaxAltitude);

	bool UpFactor = false;
	bool DownFactor = false;

	for (int i = 0; i < NumRows; ++i)
	{
		for (int j = 0; j < NumCols; ++j)
		{
			const float RandomAltitude = Dist(Gen);
			const float Probability = (RandomAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

			const float PreviousCornerAltitude = i > 0 && j > 0 ? TerrainVerticesData[i - 1][j - 1].Altitude : 5.0f;
			const float PreviousIAltitude = i > 0 ? TerrainVerticesData[i - 1][j].Altitude : 5.0f;
			const float PreviousJAltitude = j > 0 ? TerrainVerticesData[i][j - 1].Altitude : 5.0f;

			const float PreviousAverageAltitude = (PreviousCornerAltitude + PreviousIAltitude + PreviousJAltitude) / 3.0f;

			float NormalizedAltitude = (PreviousAverageAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

			const float Smooth = (PreviousAverageAltitude <= SmoothMin || PreviousAverageAltitude >= SmoothMax) ? SmoothFactor : 1.0f;

			if (UpFactor)
			{
				NormalizedAltitude *= ScaleFactor;
			}
			else if (DownFactor)
			{
				NormalizedAltitude /= ScaleFactor;
			}

			if (NormalizedAltitude > Probability)
			{
				TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude - Probability * Smooth;
				UpFactor = false;
				DownFactor = true;
			}
			else
			{
				TerrainVerticesData[i][j].Altitude = PreviousAverageAltitude + Probability * Smooth;
				UpFactor = true;
				DownFactor = false;
			}
		}
	}
}

Core::TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
u32 TerrainVerticesCount = NumRows * NumCols;

struct TestMesh
{
	std::vector<Core::EntityVertex> vertices;
	std::vector<u32> indices;
	int TextureId;
};

TestMesh CreateCubeMesh()
{
	TestMesh cube;

	// Define the 8 unique vertices for the cube with positions, colors, and texture coordinates
	glm::vec3 red = { 1.0f, 0.0f, 0.0f };
	glm::vec3 green = { 0.0f, 1.0f, 0.0f };
	glm::vec3 blue = { 0.0f, 0.0f, 1.0f };
	glm::vec3 white = { 1.0f, 1.0f, 1.0f };
	glm::vec3 yellow = { 1.0f, 1.0f, 0.0f };
	glm::vec3 cyan = { 0.0f, 1.0f, 1.0f };
	glm::vec3 magenta = { 1.0f, 0.0f, 1.0f };
	glm::vec3 black = { 0.0f, 0.0f, 0.0f };

	cube.vertices = {
		// Front face
		{ { -1.0f, -1.0f, 1.0f }, red, { 0.0f, 0.0f } }, // 0: Bottom-left
		{ { 1.0f, -1.0f, 1.0f }, green, { 1.0f, 0.0f } }, // 1: Bottom-right
		{ { 1.0f, 1.0f, 1.0f }, blue, { 1.0f, 1.0f } }, // 2: Top-right
		{ { -1.0f, 1.0f, 1.0f }, white, { 0.0f, 1.0f } }, // 3: Top-left

		// Back face
		{ { -1.0f, -1.0f, -1.0f }, yellow, { 1.0f, 0.0f } }, // 4: Bottom-left
		{ { 1.0f, -1.0f, -1.0f }, cyan, { 0.0f, 0.0f } }, // 5: Bottom-right
		{ { 1.0f, 1.0f, -1.0f }, magenta, { 0.0f, 1.0f } }, // 6: Top-right
		{ { -1.0f, 1.0f, -1.0f }, black, { 1.0f, 1.0f } }, // 7: Top-left
	};

	// Define the indices for the cube (2 triangles per face, 6 faces total)
	cube.indices = {
		// Front face
		0, 1, 2, 2, 3, 0,
		// Right face
		1, 5, 6, 6, 2, 1,
		// Back face
		5, 4, 7, 7, 6, 5,
		// Left face
		4, 0, 3, 3, 7, 4,
		// Top face
		3, 2, 6, 6, 7, 3,
		// Bottom face
		4, 5, 1, 1, 0, 4
	};

	// Set the texture ID (if needed)
	cube.TextureId = 0;

	return cube;
}

template<> struct std::hash<Core::EntityVertex>
{
	size_t operator()(Core::EntityVertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.Position) ^
			(std::hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
			(std::hash<glm::vec2>()(vertex.TextureCoords) << 1);
	}
};

struct VertexEqual
{
	bool operator()(const Core::EntityVertex& lhs, const Core::EntityVertex& rhs) const
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

std::vector<Core::TextureInfo> TexturesInfo;

void AddTexture(const char* TexturePath)
{
	int Width;
	int Height;
	int Channels;

	// TODO: delete
	stbi_uc* ImageData = stbi_load(TexturePath, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		assert(false);
	}

	TexturesInfo.push_back({ .Width =  Width, .Height = Height, .Format = STBI_rgb_alpha, .Data = ImageData });
}

void WindowCloseCallback(GLFWwindow* Window)
{
	glfwDestroyWindow(Window);
}

bool Close = false;

static bool FirstMouse = true;
static float LastX = 400, LastY = 300;
float Yaw = -90.0f;
float Pitch = 0.0f;

void MoveCamera(GLFWwindow* Window, float DeltaTime, Camera& MainCamera)
{
	const float RotationSpeed = 0.1f;
	const float CameraSpeed = 10.0f;

	float CameraDeltaSpeed = CameraSpeed * DeltaTime;

	// Handle camera movement with keys
	if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
	{
		MainCamera.CameraPosition += CameraDeltaSpeed * MainCamera.CameraFront;
	}

	if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
	{
		MainCamera.CameraPosition -= CameraDeltaSpeed * MainCamera.CameraFront;
	}

	if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
	{
		MainCamera.CameraPosition -= glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp)) * CameraDeltaSpeed;
	}

	if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
	{
		MainCamera.CameraPosition += glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp)) * CameraDeltaSpeed;
	}

	if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		MainCamera.CameraPosition += CameraDeltaSpeed * MainCamera.CameraUp;
	}

	if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		MainCamera.CameraPosition -= CameraDeltaSpeed * MainCamera.CameraUp;
	}

	if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		Close = true;
	}

	double MouseX, MouseY;
	glfwGetCursorPos(Window, &MouseX, &MouseY);

	if (FirstMouse)
	{
		LastX = MouseX;
		LastY = MouseY;
		FirstMouse = false;
	}

	float OffsetX = MouseX - LastX;
	float OffsetY = LastY - MouseY;
	LastX = MouseX;
	LastY = MouseY;

	OffsetX *= RotationSpeed;
	OffsetY *= RotationSpeed;

	Yaw += OffsetX;
	Pitch += OffsetY;

	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;

	glm::vec3 Front;
	Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front.y = sin(glm::radians(Pitch));
	Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	MainCamera.CameraFront = glm::normalize(Front);
}

void LoadDrawEntities()
{
	const char* TestTexture = "./Resources/Textures/giraffe.jpg";
	const char* Modelpath = "./Resources/Models/uh60.obj";
	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	// TODO: -_-
	int TextureIndex = 1;
	std::vector<int> MaterialToTexture(Materials.size());

	AddTexture(TestTexture);

	for (size_t i = 0; i < Materials.size(); i++)
	{
		MaterialToTexture[i] = 0;
		const tinyobj::material_t& Material = Materials[i];
		if (!Material.diffuse_texname.empty())
		{
			int Idx = Material.diffuse_texname.rfind("\\");
			std::string FileName = "./Resources/Textures/" + Material.diffuse_texname.substr(Idx + 1);

			MaterialToTexture[i] = TextureIndex;
			AddTexture(FileName.c_str());
			++TextureIndex;
		}
	}

	std::vector<TestMesh> ModelMeshes;
	ModelMeshes.reserve(Shapes.size());

	std::unordered_map<Core::EntityVertex, u32, std::hash<Core::EntityVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		TestMesh Tm;

		for (const auto& index : Shape.mesh.indices)
		{
			Core::EntityVertex vertex{ };

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
				uniqueVertices[vertex] = static_cast<u32>(Tm.vertices.size());
				Tm.vertices.push_back(vertex);
			}

			Tm.indices.push_back(uniqueVertices[vertex]);
		}

		Tm.TextureId = MaterialToTexture[Shape.mesh.material_ids[0]];

		ModelMeshes.push_back(Tm);
	}

	ModelMeshes.emplace_back(CreateCubeMesh());

	DrawEntities.resize(ModelMeshes.size());
	
	std::vector<Core::Mesh> m(ModelMeshes.size());

	for (int i = 0; i < ModelMeshes.size(); ++i)
	{
		m[i].MeshVertices = ModelMeshes[i].vertices.data();
		m[i].MeshVerticesCount = ModelMeshes[i].vertices.size();


		m[i].MeshIndices = ModelMeshes[i].indices.data();
		m[i].MeshIndicesCount = ModelMeshes[i].indices.size();

		DrawEntities[i].TextureId = ModelMeshes[i].TextureId;
	}

	RenderingSystem.CreateDrawEntities(m.data(), m.size(), DrawEntities.data());

	glm::vec3 CubePos(1.2f, 1.0f, 2.0f);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, CubePos);
	model = glm::scale(model, glm::vec3(0.2f));

	DrawEntities[ModelMeshes.size() - 1].Model = model;
}

void LoadShaders()
{
	std::vector<const char*> ShaderPaths;
	ShaderPaths.reserve(Core::ShaderNames::ShadersCount);
	std::vector<Core::ShaderName> NameToPath;
	NameToPath.reserve(Core::ShaderNames::ShadersCount);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_vert.spv");
	NameToPath.push_back(Core::ShaderNames::TerrainVertex);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_frag.spv");
	NameToPath.push_back(Core::ShaderNames::TerrainFragment);

	ShaderPaths.push_back("./Resources/Shaders/vert.spv");
	NameToPath.push_back(Core::ShaderNames::EntityVertex);

	ShaderPaths.push_back("./Resources/Shaders/frag.spv");
	NameToPath.push_back(Core::ShaderNames::EntityFragment);

	ShaderPaths.push_back("./Resources/Shaders/second_vert.spv");
	NameToPath.push_back(Core::ShaderNames::DeferredVertex);

	ShaderPaths.push_back("./Resources/Shaders/second_frag.spv");
	NameToPath.push_back(Core::ShaderNames::DeferredFragment);

	for (u32 i = 0; i < Core::ShaderNames::ShadersCount; ++i)
	{
		Util::OpenAndReadFileFull(ShaderPaths[i], ShaderCodes[i], "rb");
		ShaderCodeDescriptions[i].Code = reinterpret_cast<u32*>(ShaderCodes[i].data());
		ShaderCodeDescriptions[i].CodeSize = ShaderCodes[i].size();
		ShaderCodeDescriptions[i].Name = NameToPath[i];
	}
}

int main()
{
	const u32 FrameAllocSize = 1024 * 1024;

	auto MemorySystem = Memory::MemoryManagementSystem::Get();
	MemorySystem->Init(FrameAllocSize);

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

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	GenerateTerrain();
	LoadShaders();

	std::vector<u32> indices;
	for (int row = 0; row < NumRows - 1; ++row)
	{
		for (int col = 0; col < NumCols - 1; ++col)
		{
			u32 topLeft = row * NumCols + col;
			u32 topRight = topLeft + 1;
			u32 bottomLeft = (row + 1) * NumCols + col;
			u32 bottomRight = bottomLeft + 1;

			// First triangle (Top-left, Bottom-left, Bottom-right)
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);

			// Second triangle (Top-left, Bottom-right, Top-right)
			indices.push_back(topLeft);
			indices.push_back(bottomRight);
			indices.push_back(topRight);
		}
	}


	Core::RenderConfig Config;
	Config.RenderShaders = ShaderCodeDescriptions;
	Config.ShadersCount = Core::ShaderNames::ShadersCount;
	Config.MaxTextures = 33;

	RenderingSystem.Init(Window, Config);
	LoadDrawEntities();

	Core::DrawScene Scene;

	Scene.ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		static_cast<float>(1600) / static_cast<float>(800), 0.1f, 100.0f);
	Scene.ViewProjection.Projection[1][1] *= -1;
	Scene.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	
	Core::DrawTerrainEntity TestDrawTerrainEntity;
	RenderingSystem.CreateTerrainDrawEntity(&TerrainVerticesData[0][0], NumRows * NumCols, TestDrawTerrainEntity);

	RenderingSystem.LoadTextures(TexturesInfo.data(), TexturesInfo.size());
	RenderingSystem.CreateTerrainIndices(indices.data(), indices.size());

	Scene.DrawTerrainEntities = &TestDrawTerrainEntity;
	Scene.DrawTerrainEntitiesCount = 1;

	Scene.DrawEntities = DrawEntities.data();
	Scene.DrawEntitiesCount = DrawEntities.size();


	double DeltaTime = 0.0f;
	double LastTime = 0.0f;

	Camera MainCamera;

	MemorySystem->FrameDealloc();

	while (!glfwWindowShouldClose(Window) && !Close)
	{
		glfwPollEvents();

		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<float>(CurrentTime);

		MoveCamera(Window, DeltaTime, MainCamera);
		Scene.ViewProjection.View = glm::lookAt(MainCamera.CameraPosition, MainCamera.CameraPosition + MainCamera.CameraFront, MainCamera.CameraUp);

		RenderingSystem.Draw(Scene);

		MemorySystem->FrameDealloc();
	}

	for (int i = 0; i < DrawEntities.size(); ++i)
	{
		RenderingSystem.DestroyDrawEntity(DrawEntities[i]);
	}

	RenderingSystem.DestroyTerrainDrawEntity(TestDrawTerrainEntity);
	
	RenderingSystem.DeInit();


	glfwDestroyWindow(Window);

	glfwTerminate();

	MemorySystem->DeInit();

	if (Memory::MemoryManagementSystem::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Memory::MemoryManagementSystem::AllocateCounter);
		assert(false);
	}

	return 0;
}
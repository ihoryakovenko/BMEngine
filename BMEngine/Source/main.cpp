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
#include "Core/BMRInterface.h"
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <random>
#include "Memory/MemoryManagmentSystem.h"
#include "Util/EngineTypes.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "stb_image.h"

const u32 NumRows = 600;
const u32 NumCols = 600;

u32 TestTextureIndex;
u32 WhiteTextureIndex;
u32 ContainerTextureIndex;
u32 ContainerSpecularTextureIndex;
u32 BlendWindowIndex;
u32 GrassTextureIndex;

u32 TestMaterialIndex;
u32 WhiteMaterialIndex;
u32 ContainerMaterialIndex;
u32 BlendWindowMaterial;
u32 GrassMaterial;

BMR::BMRTerrainVertex TerrainVerticesData[NumRows][NumCols];

std::vector<BMR::BMRDrawEntity> DrawEntities;
BMR::BMRConfig Config;
std::vector<std::vector<char>> ShaderCodes(BMR::BMRShaderNames::ShaderNamesCount); // UB?
BMR::BMRDrawSkyBoxEntity SkyBox;

void GenerateTerrain()
{
	const f32 MaxAltitude = 10.0f;
	const f32 MinAltitude = 0.0f;
	const f32 SmoothMin = 7.0f;
	const f32 SmoothMax = 3.0f;
	const f32 SmoothFactor = 0.5f;
	const f32 ScaleFactor = 0.2f;

	std::mt19937 Gen(1);
	std::uniform_real_distribution<f32> Dist(MinAltitude, MaxAltitude);

	bool UpFactor = false;
	bool DownFactor = false;

	for (int i = 0; i < NumRows; ++i)
	{
		for (int j = 0; j < NumCols; ++j)
		{
			const f32 RandomAltitude = Dist(Gen);
			const f32 Probability = (RandomAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

			const f32 PreviousCornerAltitude = i > 0 && j > 0 ? TerrainVerticesData[i - 1][j - 1].Altitude : 5.0f;
			const f32 PreviousIAltitude = i > 0 ? TerrainVerticesData[i - 1][j].Altitude : 5.0f;
			const f32 PreviousJAltitude = j > 0 ? TerrainVerticesData[i][j - 1].Altitude : 5.0f;

			const f32 PreviousAverageAltitude = (PreviousCornerAltitude + PreviousIAltitude + PreviousJAltitude) / 3.0f;

			f32 NormalizedAltitude = (PreviousAverageAltitude - MinAltitude) / (MaxAltitude - MinAltitude);

			const f32 Smooth = (PreviousAverageAltitude <= SmoothMin || PreviousAverageAltitude >= SmoothMax) ? SmoothFactor : 1.0f;

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

BMR::BMRTerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
u32 TerrainVerticesCount = NumRows * NumCols;

struct TestMesh
{
	std::vector<BMR::BMREntityVertex> vertices;
	std::vector<u32> indices;
	int MaterialIndex;
};

namespace std
{
	template<> struct hash<BMR::BMREntityVertex>
	{
		size_t operator()(BMR::BMREntityVertex const& vertex) const
		{
			size_t hashPosition = std::hash<glm::vec3>()(vertex.Position);
			size_t hashColor = std::hash<glm::vec3>()(vertex.Color);
			size_t hashTextureCoords = std::hash<glm::vec2>()(vertex.TextureCoords);
			size_t hashNormal = std::hash<glm::vec3>()(vertex.Normal);

			size_t combinedHash = hashPosition;
			combinedHash ^= (hashColor << 1);
			combinedHash ^= (hashTextureCoords << 1);
			combinedHash ^= (hashNormal << 1);

			return combinedHash;
		}
	};
}

struct VertexEqual
{
	bool operator()(const BMR::BMREntityVertex& lhs, const BMR::BMREntityVertex& rhs) const
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

u32 AddTexture(const char* DiffuseTexturePath)
{
	int Width;
	int Height;
	int Channels;

	// TODO: delete
	stbi_uc* ImageData = stbi_load(DiffuseTexturePath, &Width, &Height, &Channels, STBI_rgb_alpha);

	if (ImageData == nullptr)
	{
		assert(false);
	}

	BMR::BMRTextureArrayInfo Info;
	Info.Width = Width;
	Info.Height = Height;
	Info.Format = STBI_rgb_alpha;
	Info.LayersCount = 1;
	Info.Data = ImageData;

	return BMR::LoadTexture(Info);
}

void WindowCloseCallback(GLFWwindow* Window)
{
	glfwDestroyWindow(Window);
}

bool Close = false;

static bool FirstMouse = true;
static f32 LastX = 400, LastY = 300;
f32 Yaw = -90.0f;
f32 Pitch = 0.0f;

void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera)
{
	const f32 RotationSpeed = 0.1f;
	const f32 CameraSpeed = 10.0f;

	f32 CameraDeltaSpeed = CameraSpeed * DeltaTime;

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

	f64 MouseX, MouseY;
	glfwGetCursorPos(Window, &MouseX, &MouseY);

	if (FirstMouse)
	{
		LastX = MouseX;
		LastY = MouseY;
		FirstMouse = false;
	}

	f32 OffsetX = MouseX - LastX;
	f32 OffsetY = LastY - MouseY;
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

TestMesh CreateCubeMesh(const char* Modelpath, int materialIndex)
{
	TestMesh cube;

	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	std::unordered_map<BMR::BMREntityVertex, u32, std::hash<BMR::BMREntityVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		for (const auto& index : Shape.mesh.indices)
		{
			BMR::BMREntityVertex vertex{ };

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

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					Attrib.normals[3 * index.normal_index + 0],
					Attrib.normals[3 * index.normal_index + 1],
					Attrib.normals[3 * index.normal_index + 2]
				};
			}
			else
			{
				assert(false);
				// Fallback if normal is not available (optional)
				vertex.Normal = { 0.0f, 1.0f, 0.0f }; // Default to up-direction, or compute it later
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<u32>(cube.vertices.size());
				cube.vertices.push_back(vertex);
			}

			cube.indices.push_back(uniqueVertices[vertex]);
		}

		cube.MaterialIndex = materialIndex;
	}

	return cube;
}

void LoadDrawEntities()
{
	const char* Grass = "./Resources/Textures/grass.png";
	const char* BlendWindow = "./Resources/Textures/blending_transparent_window.png";
	const char* ContainerSpecularTexture = "./Resources/Textures/container2_specular.png";
	const char* ContainerTexture = "./Resources/Textures/container2.png";
	const char* WhiteTexture = "./Resources/Textures/White.png";
	const char* TestTexture = "./Resources/Textures/1giraffe.jpg";
	const char* Modelpath = "./Resources/Models/uh60.obj";
	const char* CubeObj = "./Resources/Models/cube.obj";
	const char* SkyBoxObj = "./Resources/Models/SkyBox.obj";

	const char* BaseDir = "./Resources/Models/";

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, Modelpath, BaseDir))
	{
		assert(false);
	}

	std::vector<int> MaterialToTexture(Materials.size());

	TestTextureIndex = AddTexture(TestTexture);
	WhiteTextureIndex = AddTexture(WhiteTexture);
	ContainerTextureIndex = AddTexture(ContainerTexture);
	ContainerSpecularTextureIndex = AddTexture(ContainerSpecularTexture);
	BlendWindowIndex = AddTexture(BlendWindow);
	GrassTextureIndex = AddTexture(Grass);

	TestMaterialIndex = BMR::LoadMaterial(TestTextureIndex, ContainerSpecularTextureIndex);
	WhiteMaterialIndex = BMR::LoadMaterial(WhiteTextureIndex, WhiteTextureIndex);
	ContainerMaterialIndex = BMR::LoadMaterial(ContainerTextureIndex, ContainerSpecularTextureIndex);
	BlendWindowMaterial = BMR::LoadMaterial(BlendWindowIndex, BlendWindowIndex);
	GrassMaterial = BMR::LoadMaterial(GrassTextureIndex, GrassTextureIndex);

	for (size_t i = 0; i < Materials.size(); i++)
	{
		MaterialToTexture[i] = BlendWindowMaterial;
		const tinyobj::material_t& Material = Materials[i];
		if (!Material.diffuse_texname.empty())
		{
			int Idx = Material.diffuse_texname.rfind("\\");
			std::string FileName = "./Resources/Textures/" + Material.diffuse_texname.substr(Idx + 1);

			const u32 NewTextureIndex = AddTexture(FileName.c_str());
			MaterialToTexture[i] = BMR::LoadMaterial(NewTextureIndex, NewTextureIndex);
		}
	}

	std::vector<TestMesh> ModelMeshes;
	ModelMeshes.reserve(Shapes.size());

	std::unordered_map<BMR::BMREntityVertex, u32, std::hash<BMR::BMREntityVertex>, VertexEqual> uniqueVertices{ };

	for (const auto& Shape : Shapes)
	{
		TestMesh Tm;

		for (const auto& index : Shape.mesh.indices)
		{
			BMR::BMREntityVertex vertex{ };

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

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					Attrib.normals[3 * index.normal_index + 0],
					Attrib.normals[3 * index.normal_index + 1],
					Attrib.normals[3 * index.normal_index + 2]
				};
			}
			else
			{
				assert(false);
				// Fallback if normal is not available (optional)
				vertex.Normal = { 0.0f, 1.0f, 0.0f }; // Default to up-direction, or compute it later
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<u32>(Tm.vertices.size());
				Tm.vertices.push_back(vertex);
			}

			Tm.indices.push_back(uniqueVertices[vertex]);
		}

		Tm.MaterialIndex = MaterialToTexture[Shape.mesh.material_ids[0]];

		ModelMeshes.push_back(Tm);
	}
	
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, GrassMaterial));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, WhiteMaterialIndex));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, WhiteMaterialIndex));
	ModelMeshes.emplace_back(CreateCubeMesh(CubeObj, ContainerMaterialIndex));

	DrawEntities.resize(ModelMeshes.size());

	for (int i = 0; i < ModelMeshes.size(); ++i)
	{
		DrawEntities[i].VertexOffset = LoadVertices(ModelMeshes[i].vertices.data(),
			sizeof(BMR::BMREntityVertex), ModelMeshes[i].vertices.size());

		DrawEntities[i].IndexOffset = BMR::LoadIndices(ModelMeshes[i].indices.data(),
			ModelMeshes[i].indices.size());

		DrawEntities[i].IndicesCount = ModelMeshes[i].indices.size();
		DrawEntities[i].Model = glm::mat3(1);
		DrawEntities[i].MaterialIndex = ModelMeshes[i].MaterialIndex;
	}

	auto SkyBoxMesh = CreateCubeMesh(SkyBoxObj, ContainerMaterialIndex);

	SkyBox.VertexOffset = LoadVertices(SkyBoxMesh.vertices.data(),
		sizeof(BMR::BMREntityVertex), SkyBoxMesh.vertices.size());
	SkyBox.IndexOffset = BMR::LoadIndices(SkyBoxMesh.indices.data(), SkyBoxMesh.indices.size());
	SkyBox.IndicesCount = SkyBoxMesh.indices.size();
	SkyBox.MaterialIndex = SkyBoxMesh.MaterialIndex;

	{
		glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(500.0f));

		SkyBox.Model = model;
	}

	{
		glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));

		DrawEntities[ModelMeshes.size() - 4].Model = model;
	}

	{
		glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
		glm::mat4 Lightmodel = glm::mat4(1.0f);
		Lightmodel = glm::translate(Lightmodel, LightCubePos);
		Lightmodel = glm::scale(Lightmodel, glm::vec3(0.2f));

		DrawEntities[ModelMeshes.size() - 3].Model = Lightmodel;
	}

	{
		glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::scale(model, glm::vec3(1.0f));

		DrawEntities[ModelMeshes.size() - 2].Model = model;
	}

	{
		glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, CubePos);
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));

		DrawEntities[ModelMeshes.size() - 1].Model = model;
	}

}

void LoadShaders()
{
	std::vector<const char*> ShaderPaths;
	std::vector<BMR::BMRPipelineHandles> HandleToPath;
	std::vector<BMR::BMRShaderStages> StageToStages;

	ShaderPaths.reserve(BMR::BMRShaderNames::ShaderNamesCount);
	HandleToPath.reserve(BMR::BMRPipelineHandles::PipelineHandlesCount);
	StageToStages.reserve(BMR::BMRShaderStages::ShaderStagesCount);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Terrain);
	StageToStages.push_back(BMR::BMRShaderStages::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/TerrainGenerator_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Terrain);
	StageToStages.push_back(BMR::BMRShaderStages::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Entity);
	StageToStages.push_back(BMR::BMRShaderStages::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Entity);
	StageToStages.push_back(BMR::BMRShaderStages::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/second_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Deferred);
	StageToStages.push_back(BMR::BMRShaderStages::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/second_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::Deferred);
	StageToStages.push_back(BMR::BMRShaderStages::Fragment);

	ShaderPaths.push_back("./Resources/Shaders/SkyBox_vert.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::SkyBox);
	StageToStages.push_back(BMR::BMRShaderStages::Vertex);

	ShaderPaths.push_back("./Resources/Shaders/SkyBox_frag.spv");
	HandleToPath.push_back(BMR::BMRPipelineHandles::SkyBox);
	StageToStages.push_back(BMR::BMRShaderStages::Fragment);


	for (u32 i = 0; i < BMR::BMRShaderNames::ShaderNamesCount; ++i)
	{
		Util::OpenAndReadFileFull(ShaderPaths[i], ShaderCodes[i], "rb");
		Config.RenderShaders[i].Code = reinterpret_cast<u32*>(ShaderCodes[i].data());
		Config.RenderShaders[i].CodeSize = ShaderCodes[i].size();
		Config.RenderShaders[i].Handle = HandleToPath[i];
		Config.RenderShaders[i].Stage = StageToStages[i];
	}
}

int main()
{
	const u32 FrameAllocSize = 1024 * 1024;

	Memory::BmMemoryManagementSystem::Init(FrameAllocSize);

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

	Config.MaxTextures = 90;

	BMR::Init(Window, Config);
	LoadDrawEntities();

	BMR::BMRDrawScene Scene;
	Scene.SkyBox = SkyBox;
	Scene.DrawSkyBox = true;

	Scene.ViewProjection.Projection = glm::perspective(glm::radians(45.f),
		static_cast<f32>(1600) / static_cast<f32>(800), 0.1f, 1000.0f);
	Scene.ViewProjection.Projection[1][1] *= -1;
	Scene.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	
	BMR::BMRDrawTerrainEntity TestDrawTerrainEntity;
	TestDrawTerrainEntity.VertexOffset = LoadVertices(&TerrainVerticesData[0][0],
		sizeof(BMR::BMRTerrainVertex), NumRows * NumCols);
	TestDrawTerrainEntity.IndexOffset = BMR::LoadIndices(indices.data(), indices.size());
	TestDrawTerrainEntity.IndicesCount = indices.size();
	TestDrawTerrainEntity.MaterialIndex = TestMaterialIndex;

	Scene.DrawTerrainEntities = &TestDrawTerrainEntity;
	Scene.DrawTerrainEntitiesCount = 1;

	Scene.DrawEntities = DrawEntities.data();
	Scene.DrawEntitiesCount = DrawEntities.size();


	f64 DeltaTime = 0.0f;
	f64 LastTime = 0.0f;

	f32 Angle = 0.0f;

	Camera MainCamera;

	Memory::BmMemoryManagementSystem::FrameDealloc();

	BMR::BMRLightBuffer TestData;
	TestData.PointLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
	TestData.PointLight.Ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	TestData.PointLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.PointLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
	TestData.PointLight.Constant = 1.0f;
	TestData.PointLight.Linear = 0.09;
	TestData.PointLight.Quadratic = 0.032;

	TestData.DirectionLight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	TestData.DirectionLight.Ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	TestData.DirectionLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.DirectionLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

	TestData.SpotLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
	TestData.SpotLight.Ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	TestData.SpotLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	TestData.SpotLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
	TestData.SpotLight.Constant = 1.0f;
	TestData.SpotLight.Linear = 0.09;
	TestData.SpotLight.Quadratic = 0.032;
	TestData.SpotLight.CutOff = glm::cos(glm::radians(12.5f));
	TestData.SpotLight.OuterCutOff = glm::cos(glm::radians(17.5f));

	BMR::BMRMaterial Mat;
	Mat.Shininess = 32.f;
	BMR::UpdateMaterialBuffer(Mat);

	while (!glfwWindowShouldClose(Window) && !Close)
	{
		glfwPollEvents();

		const f64 CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = static_cast<f32>(CurrentTime);

		Angle += 0.5f * static_cast<f32>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		for (int i = 0; i < Scene.DrawEntitiesCount; ++i)
		{
			glm::mat4 TestMat = glm::rotate(Scene.DrawEntities[i].Model, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
			//Scene.DrawEntities[i].Model = TestMat;
		}

		MoveCamera(Window, DeltaTime, MainCamera);
		Scene.ViewProjection.View = glm::lookAt(MainCamera.CameraPosition, MainCamera.CameraPosition + MainCamera.CameraFront, MainCamera.CameraUp);

		TestData.SpotLight.Direction = MainCamera.CameraFront;
		TestData.SpotLight.Position = MainCamera.CameraPosition;

		UpdateLightBuffer(TestData);

		// TODO: Need to sort objects by distance to camera
		int LastOpaqueIndex = Scene.DrawEntitiesCount - 1;
		for (; LastOpaqueIndex >= 0; --LastOpaqueIndex)
		{
			if (Scene.DrawEntities[LastOpaqueIndex].MaterialIndex != GrassMaterial &&
				Scene.DrawEntities[LastOpaqueIndex].MaterialIndex != BlendWindowMaterial)
			{
				break;
			}
		}

		for (int i = 0; i < LastOpaqueIndex; ++i)
		{
			if (Scene.DrawEntities[i].MaterialIndex == GrassMaterial ||
				Scene.DrawEntities[i].MaterialIndex == BlendWindowMaterial)
			{
				std::swap(Scene.DrawEntities[i], Scene.DrawEntities[LastOpaqueIndex]);
				--LastOpaqueIndex;
			}
		}

		Draw(Scene);

		Memory::BmMemoryManagementSystem::FrameDealloc();
	}
	
	BMR::DeInit();


	glfwDestroyWindow(Window);

	glfwTerminate();

	Memory::BmMemoryManagementSystem::DeInit();

	if (Memory::BmMemoryManagementSystem::AllocateCounter != 0)
	{
		Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Memory::BmMemoryManagementSystem::AllocateCounter);
		assert(false);
	}

	return 0;
}
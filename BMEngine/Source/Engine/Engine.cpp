#include "Engine.h"

#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <thread>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

#include "Memory/MemoryManagmentSystem.h"
#include "Platform/WinPlatform.h"
#include "Util/Settings.h"
#include "Render/Render.h"
#include "Util/Util.h"
#include "ImguiIntegration.h"
#include "Systems/DynamicMapSystem.h"
#include "Systems/ResourceManager.h"
#include "Scene.h"
#include "Util/Math.h"

namespace std
{
	template<> struct hash<EntityVertex>
	{
		size_t operator()(EntityVertex const& vertex) const
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

	template<> struct hash<SkyBoxVertex>
	{
		size_t operator()(SkyBoxVertex const& vertex) const
		{
			size_t hashPosition = std::hash<glm::vec3>()(vertex.Position);
			size_t combinedHash = hashPosition;
			return combinedHash;
		}
	};
}

namespace Engine
{
	struct VertexEqual
	{
		bool operator()(const EntityVertex& lhs, const EntityVertex& rhs) const
		{
			return lhs.Position == rhs.Position && lhs.Color == rhs.Color && lhs.TextureCoords == rhs.TextureCoords;
		}

		bool operator()(const SkyBoxVertex& lhs, const SkyBoxVertex& rhs) const
		{
			return lhs.Position == rhs.Position;
		}
	};

	static bool Init();
	static bool InitSystems();
	static void DeInit();

	static void Update(f64 DeltaTime);

	static void LoadModel(const char* ModelPath, glm::mat4 Model, VkDescriptorSet CustomMaterial = nullptr);

	static void LoadTerrain();
	static void CreateSkyBoxMesh(VkDescriptorSet Material);

	static void SetUpScene();

	static void RenderLog(Render::BMRLogType LogType, const char* Format, va_list Args);
	static void GenerateTerrain(std::vector<u32>& Indices);

	static void MoveCamera(GLFWwindow* Window, f32 DeltaTime, DynamicMapSystem::MapCamera& MainCamera);

	static const char ModelsBaseDir[] = "./Resources/Models/";

	static Platform::BMRTMPWindowHandler* Window = nullptr;

	static DynamicMapSystem::MapCamera MainCamera;
	static bool Close = false;
	static bool FirstMouse = true;
	static f32 LastX = 400, LastY = 300;
	static f32 Yaw = -90.0f;
	static f32 Pitch = 0.0f;

	static f64 DeltaTime = 0.0f;
	static f64 LastTime = 0.0f;

	static const f32 Near = 0.00001f;
	static const f32 Far = 5.0f;

	static const u32 NumRows = 600;
	static const u32 NumCols = 600;
	static TerrainVertex TerrainVerticesData[NumRows][NumCols];
	static TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
	static u32 TerrainVerticesCount = NumRows * NumCols;


	static Render::DrawTerrainEntity TestDrawTerrainEntity;
	static std::vector<Render::DrawEntity> DrawEntities;
	static Render::DrawSkyBoxEntity SkyBox;
	static Render::LightBuffer LightData;

	static ImguiIntegration::GuiData GuiData;

	static glm::vec3 Eye = glm::vec3(0.0f, 10.0f, 0.0f);
	static glm::vec3 Up = glm::vec3(0.0f, 0.0f, -1.0f);

	static glm::vec3 CameraSphericalPosition = glm::vec3(0.0f, 0.0f, 6371.0f);
	static s32 Zoom = 4;

	int Main()
	{
		Init();

		Memory::BmMemoryManagementSystem::FrameFree();

		bool DrawImgui = true;
		std::thread ImguiThread([&]()
		{
			// TODO: FIX
			// UB race condition, 2 threads to variables
			ImguiIntegration::DrawLoop(DrawImgui, GuiData);
		});

		while (!glfwWindowShouldClose(Window) && !Close)
		{
			glfwPollEvents();

			const f64 CurrentTime = glfwGetTime();
			DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;

			Update(DeltaTime);
			Draw(&Scene);

			Memory::BmMemoryManagementSystem::FrameFree();
		}

		DrawImgui = false;
		ImguiThread.join();

		DeInit();

		return 0;
	}

	bool Init()
	{
		u32 WindowWidth = 1920;
		u32 WindowHeight = 1080;

		Window = Platform::CreatePlatformWindow(WindowWidth, WindowHeight);
		Platform::DisableCursor(Window);

		Platform::GetWindowSizes(glfwGetWin32Window(Window), &WindowWidth, &WindowHeight);

		LoadSettings(WindowWidth, WindowHeight);

		InitSystems();

		//CreateSkyBoxMesh(EngineMaterials["SkyBoxMaterial"]);
		//LoadModel("./Resources/Models/uh60.obj", glm::mat4(1.0f));
		//LoadTerrain();
	

		//{
		//	glm::vec3 CubePos(0.0f, -5.0f, 0.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(20.0f, 1.0f, 20.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(0.2f, 5.0f, 0.2f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["TestMaterial"]);
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::rotate(Model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//	Model = glm::scale(Model, glm::vec3(0.5f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["GrassMaterial"]);
		//}

		//{
		//	glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, LightCubePos);
		//	Model = glm::scale(Model, glm::vec3(0.2f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(1.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		//}

		//{
		//	glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::rotate(Model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//	Model = glm::scale(Model, glm::vec3(1.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["ContainerMaterial"]);
		//}

		SetUpScene();
		 
		return true;
	}

	bool InitSystems()
	{
		const u32 FrameAllocSize = 1024 * 1024;
		Memory::BmMemoryManagementSystem::Init(FrameAllocSize);

		Render::RenderConfig RenderConfig;
		RenderConfig.MaxTextures = 90;
		RenderConfig.LogHandler = RenderLog;

		RenderConfig.EnableValidationLayers = false;

#ifdef _DEBUG
		RenderConfig.EnableValidationLayers = true;
#endif

		Render::Init(glfwGetWin32Window(Window), &RenderConfig);
		ResourceManager::Init();
		DynamicMapSystem::Init();

		return true;
	}

	void DeInit()
	{
		DynamicMapSystem::DeInit();

		ResourceManager::DeInit();
		Render::DeInit();

		glfwDestroyWindow(Window);

		glfwTerminate();

		Memory::BmMemoryManagementSystem::DeInit();

		if (Memory::BmMemoryManagementSystem::AllocateCounter != 0)
		{
			Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Memory::BmMemoryManagementSystem::AllocateCounter);
			assert(false);
		}
	}

	void Update(f64 DeltaTime)
	{
		//MoveCamera(Window, DeltaTime, MainCamera);

		CameraSphericalPosition.z = DynamicMapSystem::CalculateCameraAltitude(Zoom);
		CameraSphericalPosition.z += 1.0f;

		//MainCamera.altitude = CameraSphericalPosition.z;

		MainCamera.Position = DynamicMapSystem::SphericalToMercator(CameraSphericalPosition);
		
		MainCamera.Front = glm::normalize(-MainCamera.Position);

		const glm::vec3 NorthPole(0.0f, 1.0f, 0.0f);
		const glm::vec3 Right = glm::normalize(glm::cross(NorthPole, MainCamera.Front));
		MainCamera.Up = glm::normalize(glm::cross(MainCamera.Front, Right));


		f32 AspectRatio = f32(MainScreenExtent.width) / f32(MainScreenExtent.height);
		DynamicMapSystem::Update(DeltaTime, MainCamera, Zoom);

		static f32 Angle = 0.0f;

		Angle += 0.5f * static_cast<f32>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		for (int i = 0; i < Scene.DrawEntitiesCount; ++i)
		{
			glm::mat4 TestMat = glm::rotate(Scene.DrawEntities[i].Model, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		Scene.ViewProjection.View = glm::lookAt(MainCamera.Position, MainCamera.Position + MainCamera.Front, MainCamera.Up);

		float NearPlane = 0.1f, FarPlane = 100.0f;
		float HalfSize = 30.0f;
		glm::mat4 LightProjection = glm::ortho(-HalfSize, HalfSize, -HalfSize, HalfSize, NearPlane, FarPlane);

		glm::vec3 Center = Eye + LightData.DirectionLight.Direction;
		glm::mat4 LightView = glm::lookAt(Eye, Center, Up);

		LightData.DirectionLight.LightSpaceMatrix = LightProjection * LightView;
		LightData.SpotLight.Direction = MainCamera.Front;
		LightData.SpotLight.Position = MainCamera.Position;
		LightData.SpotLight.Planes = glm::vec2(Near, Far);
		LightData.SpotLight.LightSpaceMatrix = Scene.ViewProjection.Projection * Scene.ViewProjection.View;

		Scene.LightEntity = &LightData;

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
	}

	void LoadModel(const char* ModelPath, glm::mat4 Model, VkDescriptorSet CustomMaterial)
	{
		tinyobj::attrib_t Attrib;
		std::vector<tinyobj::shape_t> Shapes;
		std::vector<tinyobj::material_t> Materials;
		std::string Warn, Err;

		if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, ModelPath, ModelsBaseDir))
		{
			assert(false);
		}

		std::vector<VkDescriptorSet> MaterialToTexture(Materials.size());

		if (CustomMaterial == nullptr)
		{
			for (size_t i = 0; i < Materials.size(); i++)
			{
				const tinyobj::material_t& Material = Materials[i];
				if (!Material.diffuse_texname.empty())
				{
					int Idx = Material.diffuse_texname.rfind("\\");
					const std::string FileName = Material.diffuse_texname.substr(Idx + 1);

					if (ResourceManager::FindTexture(FileName) == nullptr)
					{
						Render::RenderTexture NewTexture = ResourceManager::LoadTexture(FileName, std::vector<std::string>{FileName}, VK_IMAGE_VIEW_TYPE_2D);
						ResourceManager::CreateEntityMaterial(FileName, NewTexture.ImageView, NewTexture.ImageView, &MaterialToTexture[i]);
					}
				}
				else
				{
					MaterialToTexture[i] = ResourceManager::FindMaterial("BlendWindowMaterial");
				}
			}
		}

		std::unordered_map<EntityVertex, u32, std::hash<EntityVertex>, VertexEqual> uniqueVertices{ };

		for (const auto& Shape : Shapes)
		{
			std::vector<EntityVertex> vertices;
			std::vector<u32> indices;

			for (const auto& index : Shape.mesh.indices)
			{
				EntityVertex vertex{ };

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

				if (!uniqueVertices.count(vertex))
				{
					uniqueVertices[vertex] = static_cast<u32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}

			Render::DrawEntity DrawEntity;
			DrawEntity.VertexOffset = Render::LoadVertices(vertices.data(), sizeof(EntityVertex), vertices.size());
			DrawEntity.IndexOffset = Render::LoadIndices(indices.data(), indices.size());
			DrawEntity.IndicesCount = indices.size();
			DrawEntity.Model = Model;
			DrawEntity.TextureSet = CustomMaterial;

			if (CustomMaterial == nullptr)
			{
				DrawEntity.TextureSet = MaterialToTexture[Shape.mesh.material_ids[0]];
			}

			DrawEntities.push_back(DrawEntity);
		}
	}

	void LoadTerrain()
	{
		std::vector<u32> TerrainIndices;
		GenerateTerrain(TerrainIndices);

		TestDrawTerrainEntity.VertexOffset = Render::LoadVertices(&TerrainVerticesData[0][0],
			sizeof(TerrainVertex), NumRows * NumCols);
		TestDrawTerrainEntity.IndexOffset = Render::LoadIndices(TerrainIndices.data(), TerrainIndices.size());
		TestDrawTerrainEntity.IndicesCount = TerrainIndices.size();
		TestDrawTerrainEntity.TextureSet = ResourceManager::FindMaterial("TerrainMaterial"); // TODO load here
	}

	void SetUpScene()
	{
		MainCamera.Fov = 60.0f;
		MainCamera.AspectRatio = (float)MainScreenExtent.width / (float)MainScreenExtent.height;

		Scene.SkyBox = SkyBox;
		//Scene.DrawSkyBox = true;
		Scene.DrawSkyBox = false;

		Scene.ViewProjection.Projection = glm::perspective(glm::radians(MainCamera.Fov),
			MainCamera.AspectRatio, Near, Far);
		Scene.ViewProjection.Projection[1][1] *= -1;
		Scene.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		Scene.DrawTerrainEntities = &TestDrawTerrainEntity;
		Scene.DrawTerrainEntitiesCount = 0;

		Scene.DrawEntities = DrawEntities.data();
		Scene.DrawEntitiesCount = DrawEntities.size();

		LightData.PointLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
		LightData.PointLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
		LightData.PointLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		LightData.PointLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
		LightData.PointLight.Constant = 1.0f;
		LightData.PointLight.Linear = 0.09;
		LightData.PointLight.Quadratic = 0.032;

		LightData.DirectionLight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		LightData.DirectionLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
		LightData.DirectionLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		LightData.DirectionLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

		LightData.SpotLight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
		LightData.SpotLight.Ambient = glm::vec3(0.01f, 0.01f, 0.01f);
		LightData.SpotLight.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		LightData.SpotLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
		LightData.SpotLight.Constant = 1.0f;
		LightData.SpotLight.Linear = 0.09;
		LightData.SpotLight.Quadratic = 0.032;
		LightData.SpotLight.CutOff = glm::cos(glm::radians(12.5f));
		LightData.SpotLight.OuterCutOff = glm::cos(glm::radians(17.5f));

		Render::Material Mat;
		Mat.Shininess = 32.f;
		Render::UpdateMaterialBuffer(&Mat);

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
		GuiData.CameraMercatorPosition = &CameraSphericalPosition;
		GuiData.Zoom = &Zoom;
		GuiData.OnTestSetDownload = DynamicMapSystem::TestSetDownload;
	}

	void RenderLog(Render::BMRLogType LogType, const char* Format, va_list Args)
	{
		switch (LogType)
		{
			case Render::LogType_Error:
			{
				std::cout << "\033[31;5mError: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				assert(false);
				break;
			}
			case Render::LogType_Warning:
			{
				std::cout << "\033[33;5mWarning: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				break;
			}
			case Render::LogType_Info:
			{
				std::cout << "Info: ";
				vprintf(Format, Args);
				std::cout << '\n';
				break;
			}
		}
	}

	void GenerateTerrain(std::vector<u32>& Indices)
	{
		const f32 MaxAltitude = 0.0f;
		const f32 MinAltitude = -10.0f;
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

		Indices.reserve(NumRows * NumCols * 6);

		for (int row = 0; row < NumRows - 1; ++row)
		{
			for (int col = 0; col < NumCols - 1; ++col)
			{
				u32 topLeft = row * NumCols + col;
				u32 topRight = topLeft + 1;
				u32 bottomLeft = (row + 1) * NumCols + col;
				u32 bottomRight = bottomLeft + 1;

				// First triangle (Top-left, Bottom-left, Bottom-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomLeft);
				Indices.push_back(bottomRight);

				// Second triangle (Top-left, Bottom-right, Top-right)
				Indices.push_back(topLeft);
				Indices.push_back(bottomRight);
				Indices.push_back(topRight);
			}
		}
	}

	void MoveCamera(GLFWwindow* Window, f32 DeltaTime, DynamicMapSystem::MapCamera& MainCamera)
	{
		const f32 RotationSpeed = 0.1f;
		const f32 CameraSpeed = 10.0f;
		const f32 CameraDeltaSpeed = CameraSpeed * DeltaTime;

		// Handle camera movement with keys
		glm::vec3 movement = glm::vec3(0.0f);
		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) movement += MainCamera.Front;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) movement -= MainCamera.Front;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) movement -= glm::normalize(glm::cross(MainCamera.Front, MainCamera.Up));
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) movement += glm::normalize(glm::cross(MainCamera.Front, MainCamera.Up));
		if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS) movement += MainCamera.Up;
		if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) movement -= MainCamera.Up;

		MainCamera.Position += movement * CameraDeltaSpeed;

		if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) Close = true;

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

		Yaw += OffsetX * RotationSpeed;
		Pitch += OffsetY * RotationSpeed;

		Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

		glm::vec3 Front;
		Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front.y = sin(glm::radians(Pitch));
		Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		MainCamera.Front = glm::normalize(Front);
	}

	void CreateSkyBoxMesh(VkDescriptorSet Material)
	{
		const char* SkyBoxObj = "./Resources/Models/SkyBox.obj";

		tinyobj::attrib_t Attrib;
		std::vector<tinyobj::shape_t> Shapes;
		std::vector<tinyobj::material_t> Materials;
		std::string Warn, Err;

		if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, SkyBoxObj, ModelsBaseDir))
		{
			assert(false);
		}

		std::vector<SkyBoxVertex> vertices;
		std::vector<u32> indices;

		std::unordered_map<SkyBoxVertex, u32, std::hash<SkyBoxVertex>, VertexEqual> uniqueVertices{ };

		for (const auto& index : Shapes[0].mesh.indices)
		{
			SkyBoxVertex vertex{ };

			vertex.Position =
			{
				Attrib.vertices[3 * index.vertex_index + 0],
				Attrib.vertices[3 * index.vertex_index + 1],
				Attrib.vertices[3 * index.vertex_index + 2]
			};

			if (!uniqueVertices.count(vertex))
			{
				uniqueVertices[vertex] = static_cast<u32>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}

		SkyBox.VertexOffset = Render::LoadVertices(vertices.data(), sizeof(EntityVertex), vertices.size());
		SkyBox.IndexOffset = Render::LoadIndices(indices.data(), indices.size());
		SkyBox.IndicesCount = indices.size();
		SkyBox.TextureSet = Material;
	}
}
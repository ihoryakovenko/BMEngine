#include "BMEInterface.h"

#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <thread>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

#include "Memory/MemoryManagmentSystem.h"
#include "Platform/WinPlatform.h"
#include "Util/Settings.h"
#include "BMR/BMRInterface.h"
#include "Util/Util.h"
#include "ImguiIntegration.h"

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

namespace BME
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

	struct Camera
	{
		glm::vec3 CameraPosition = glm::vec3(0.0f, 0.0f, 20.0f);
		glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	};

	static bool Init();
	static bool InitSystems();
	static void DeInit();

	static void Update(f32 DeltaTime);

	static void LoadDefaultResources();
	static void LoadModel(const char* ModelPath, glm::mat4 Model, VkDescriptorSet CustomMaterial = nullptr);
	static BMR::BMRTexture LoadTexture(const std::vector<std::string>& PathNames);
	static void LoadTerrain();
	static void CreateSkyBoxMesh(VkDescriptorSet Material);

	void SetUpScene();

	static void RenderLog(BMR::BMRLogType LogType, const char* Format, va_list Args);
	static void GenerateTerrain(std::vector<u32>& Indices);

	static void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera);

	static Platform::BMRTMPWindowHandler* Window = nullptr;
	static BMR::BMRDrawScene Scene;
	static std::map<std::string, BMR::BMRTexture> Textures;

	static Camera MainCamera;
	static bool Close = false;
	static bool FirstMouse = true;
	static f32 LastX = 400, LastY = 300;
	static f32 Yaw = -90.0f;
	static f32 Pitch = 0.0f;

	static f64 DeltaTime = 0.0f;
	static f64 LastTime = 0.0f;

	static const f32 Near = 0.1f;
	static const f32 Far = 100.0f;

	// TODO: Replace VkDescriptorSet
	static std::map<std::string, VkDescriptorSet> EngineMaterials;

	static const u32 NumRows = 600;
	static const u32 NumCols = 600;
	static TerrainVertex TerrainVerticesData[NumRows][NumCols];
	static TerrainVertex* TerrainVerticesDataPointer = &(TerrainVerticesData[0][0]);
	static u32 TerrainVerticesCount = NumRows * NumCols;

	static std::vector<BMR::BMRDrawEntity> DrawEntities;
	static BMR::BMRDrawSkyBoxEntity SkyBox;
	static BMR::BMRDrawTerrainEntity TestDrawTerrainEntity;
	static BMR::BMRLightBuffer LightData;

	static ImguiIntegration::GuiData GuiData;

	static glm::vec3 Eye = glm::vec3(0.0f, 10.0f, 0.0f);
	static glm::vec3 Up = glm::vec3(0.0f, 0.0f, -1.0f);

	static const std::string TexturesPath = "./Resources/Textures/";
	static const char ModelsBaseDir[] = "./Resources/Models/";

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

		LoadDefaultResources();

		CreateSkyBoxMesh(EngineMaterials["SkyBoxMaterial"]);
		LoadModel("./Resources/Models/uh60.obj", glm::mat4(1.0f));
		LoadTerrain();
	

		{
			glm::vec3 CubePos(0.0f, -5.0f, 0.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(20.0f, 1.0f, 20.0f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(0.2f, 5.0f, 0.2f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["TestMaterial"]);
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::rotate(Model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			Model = glm::scale(Model, glm::vec3(0.5f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["GrassMaterial"]);
		}

		{
			glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, LightCubePos);
			Model = glm::scale(Model, glm::vec3(0.2f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		}

		{
			glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::scale(Model, glm::vec3(1.0f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["WhiteMaterial"]);
		}

		{
			glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, CubePos);
			Model = glm::rotate(Model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			Model = glm::scale(Model, glm::vec3(1.0f));
			LoadModel("./Resources/Models/cube.obj", Model, EngineMaterials["ContainerMaterial"]);
		}

		SetUpScene();
		 
		return true;
	}

	bool InitSystems()
	{
		const u32 FrameAllocSize = 1024 * 1024;
		Memory::BmMemoryManagementSystem::Init(FrameAllocSize);

		BMR::BMRConfig RenderConfig;
		RenderConfig.MaxTextures = 90;
		RenderConfig.LogHandler = RenderLog;
		RenderConfig.EnableValidationLayers = true;

		BMR::Init(glfwGetWin32Window(Window), &RenderConfig);

		return true;
	}

	void DeInit()
	{
		for (auto& Texture : Textures)
		{
			BMR::DestroyTexture(&Texture.second);
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
	}

	void Update(f32 DeltaTime)
	{
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

		MoveCamera(Window, DeltaTime, MainCamera);

		Scene.ViewProjection.View = glm::lookAt(MainCamera.CameraPosition, MainCamera.CameraPosition + MainCamera.CameraFront, MainCamera.CameraUp);

		float NearPlane = 0.1f, FarPlane = 100.0f;
		float HalfSize = 30.0f;
		glm::mat4 LightProjection = glm::ortho(-HalfSize, HalfSize, -HalfSize, HalfSize, NearPlane, FarPlane);

		glm::vec3 Center = Eye + LightData.DirectionLight.Direction;
		glm::mat4 LightView = glm::lookAt(Eye, Center, Up);

		LightData.DirectionLight.LightSpaceMatrix = LightProjection * LightView;
		LightData.SpotLight.Direction = MainCamera.CameraFront;
		LightData.SpotLight.Position = MainCamera.CameraPosition;
		LightData.SpotLight.Planes = glm::vec2(Near, Far);
		LightData.SpotLight.LightSpaceMatrix = Scene.ViewProjection.Projection * Scene.ViewProjection.View;

		Scene.LightEntity = &LightData;

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
	}

	void LoadDefaultResources()
	{
		BMR::BMRTexture TestTexture = LoadTexture(std::vector<std::string> {"1giraffe.jpg"});
		BMR::BMRTexture WhiteTexture = LoadTexture(std::vector<std::string> {"White.png"});
		BMR::BMRTexture ContainerTexture = LoadTexture(std::vector<std::string> {"container2.png"});
		BMR::BMRTexture ContainerSpecularTexture = LoadTexture(std::vector<std::string> {"container2_specular.png"});
		BMR::BMRTexture BlendWindow = LoadTexture(std::vector<std::string> {"blending_transparent_window.png"});
		BMR::BMRTexture GrassTexture = LoadTexture(std::vector<std::string> {"grass.png"});
		BMR::BMRTexture SkyBoxCubeTexture = LoadTexture(std::vector<std::string> {
			"skybox/right.jpg",
				"skybox/left.jpg",
				"skybox/top.jpg",
				"skybox/bottom.jpg",
				"skybox/front.jpg",
				"skybox/back.jpg", });

		Textures["grass"] = GrassTexture;
		Textures["blending_transparent_window"] = BlendWindow;
		Textures["container2_specular"] = ContainerSpecularTexture;
		Textures["container2"] = ContainerTexture;
		Textures["White"] = WhiteTexture;
		Textures["1giraffe"] = TestTexture;
		Textures["skybox"] = SkyBoxCubeTexture;

		VkDescriptorSet TestMaterial;
		VkDescriptorSet WhiteMaterial;
		VkDescriptorSet ContainerMaterial;
		VkDescriptorSet BlendWindowMaterial;
		VkDescriptorSet GrassMaterial;
		VkDescriptorSet SkyBoxMaterial;
		VkDescriptorSet TerrainMaterial;

		BMR::TestAttachEntityTexture(TestTexture.ImageView, TestTexture.ImageView, &TestMaterial);
		BMR::TestAttachEntityTexture(WhiteTexture.ImageView, WhiteTexture.ImageView, &WhiteMaterial);
		BMR::TestAttachEntityTexture(ContainerTexture.ImageView, ContainerSpecularTexture.ImageView, &ContainerMaterial);
		BMR::TestAttachEntityTexture(BlendWindow.ImageView, BlendWindow.ImageView, &BlendWindowMaterial);
		BMR::TestAttachEntityTexture(GrassTexture.ImageView, GrassTexture.ImageView, &GrassMaterial);
		BMR::TestAttachSkyNoxTerrainTexture(SkyBoxCubeTexture.ImageView, &SkyBoxMaterial);
		BMR::TestAttachSkyNoxTerrainTexture(TestTexture.ImageView, &TerrainMaterial);

		EngineMaterials["TestMaterial"] = TestMaterial;
		EngineMaterials["WhiteMaterial"] = WhiteMaterial;
		EngineMaterials["ContainerMaterial"] = ContainerMaterial;
		EngineMaterials["BlendWindowMaterial"] = BlendWindowMaterial;
		EngineMaterials["GrassMaterial"] = GrassMaterial;
		EngineMaterials["SkyBoxMaterial"] = SkyBoxMaterial;
		EngineMaterials["TerrainMaterial"] = TerrainMaterial;
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

					const BMR::BMRTexture NewTexture = LoadTexture(std::vector<std::string>{FileName});
					BMR::TestAttachEntityTexture(NewTexture.ImageView, NewTexture.ImageView, &MaterialToTexture[i]);

					if (Textures.count(FileName))
					{
						// TODO fix
						Textures[FileName + '1'] = NewTexture;
					}
					else
					{
						Textures[FileName] = NewTexture;
					}
				}
				else
				{
					MaterialToTexture[i] = EngineMaterials["BlendWindowMaterial"];
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

			BMR::BMRDrawEntity DrawEntity;
			DrawEntity.VertexOffset = BMR::LoadVertices(vertices.data(), sizeof(EntityVertex), vertices.size());
			DrawEntity.IndexOffset = BMR::LoadIndices(indices.data(), indices.size());
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

	BMR::BMRTexture LoadTexture(const std::vector<std::string>& PathNames)
	{
		assert(PathNames.size() > 0);
		std::vector<stbi_uc*> ImageData(PathNames.size());

		int Width = 0;
		int Height = 0;
		int Channels = 0;

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			ImageData[i] = stbi_load((TexturesPath + PathNames[i]).c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);

			if (ImageData[i] == nullptr)
			{
				assert(false);
			}
		}

		BMR::BMRTextureArrayInfo Info;
		Info.Width = Width;
		Info.Height = Height;
		Info.Format = STBI_rgb_alpha;
		Info.LayersCount = PathNames.size();
		Info.Data = ImageData.data();
		Info.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		Info.Flags = 0;

		// TODO: TMP solution
		if (Info.LayersCount > 1)
		{
			Info.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
			Info.Flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		BMR::BMRTexture Texture = BMR::CreateTexture(&Info);

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			stbi_image_free(ImageData[i]);
		};

		return Texture;
	}

	void LoadTerrain()
	{
		std::vector<u32> TerrainIndices;
		GenerateTerrain(TerrainIndices);

		TestDrawTerrainEntity.VertexOffset = BMR::LoadVertices(&TerrainVerticesData[0][0],
			sizeof(TerrainVertex), NumRows * NumCols);
		TestDrawTerrainEntity.IndexOffset = BMR::LoadIndices(TerrainIndices.data(), TerrainIndices.size());
		TestDrawTerrainEntity.IndicesCount = TerrainIndices.size();
		TestDrawTerrainEntity.TextureSet = EngineMaterials["TerrainMaterial"];
	}

	void SetUpScene()
	{
		const float Aspect = (float)MainScreenExtent.width / (float)MainScreenExtent.height;

		Scene.SkyBox = SkyBox;
		Scene.DrawSkyBox = true;

		Scene.ViewProjection.Projection = glm::perspective(glm::radians(45.f),
			Aspect, Near, Far);
		Scene.ViewProjection.Projection[1][1] *= -1;
		Scene.ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		Scene.DrawTerrainEntities = &TestDrawTerrainEntity;
		Scene.DrawTerrainEntitiesCount = 1;

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

		BMR::BMRMaterial Mat;
		Mat.Shininess = 32.f;
		BMR::UpdateMaterialBuffer(&Mat);

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
	}

	void RenderLog(BMR::BMRLogType LogType, const char* Format, va_list Args)
	{
		switch (LogType)
		{
			case BMR::LogType_Error:
			{
				std::cout << "\033[31;5mError: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				assert(false);
				break;
			}
			case BMR::LogType_Warning:
			{
				std::cout << "\033[33;5mWarning: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				break;
			}
			case BMR::LogType_Info:
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

		Indices.reserve(NumRows * NumCols);
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

	void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera)
	{
		const f32 RotationSpeed = 0.1f;
		const f32 CameraSpeed = 10.0f;
		const f32 CameraDeltaSpeed = CameraSpeed * DeltaTime;

		// Handle camera movement with keys
		glm::vec3 movement = glm::vec3(0.0f);
		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) movement += MainCamera.CameraFront;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) movement -= MainCamera.CameraFront;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) movement -= glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp));
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) movement += glm::normalize(glm::cross(MainCamera.CameraFront, MainCamera.CameraUp));
		if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS) movement += MainCamera.CameraUp;
		if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) movement -= MainCamera.CameraUp;

		MainCamera.CameraPosition += movement * CameraDeltaSpeed;

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
		MainCamera.CameraFront = glm::normalize(Front);
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

		SkyBox.VertexOffset = BMR::LoadVertices(vertices.data(), sizeof(EntityVertex), vertices.size());
		SkyBox.IndexOffset = BMR::LoadIndices(indices.data(), indices.size());
		SkyBox.IndicesCount = indices.size();
		SkyBox.TextureSet = Material;
	}
}
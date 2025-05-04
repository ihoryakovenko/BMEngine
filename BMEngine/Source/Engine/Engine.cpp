#include "Engine.h"

#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

#include "Memory/MemoryManagmentSystem.h"
#include "Platform/WinPlatform.h"
#include "Util/Settings.h"
#include "Render/Render.h"
#include "Util/Util.h"
#include "Systems/DynamicMapSystem.h"
#include "Systems/ResourceManager.h"
#include "Scene.h"
#include "Util/Math.h"
#include "Render/FrameManager.h"
#include "Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Systems/Render/LightningPass.h"
#include "Systems/Render/MainPass.h"
#include "Systems/Render/DeferredPass.h"
#include "Systems/Render/DebugUI.h"

namespace std
{
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
		bool operator()(const SkyBoxVertex& lhs, const SkyBoxVertex& rhs) const
		{
			return lhs.Position == rhs.Position;
		}
	};

	static bool Init();
	static bool InitSystems();
	static void DeInit();

	static void Update(f64 DeltaTime);

	static void CreateSkyBoxMesh(VkDescriptorSet Material);

	static void SetUpScene();

	static void RenderLog(VulkanInterface::LogType LogType, const char* Format, va_list Args);

	static void MoveCamera(GLFWwindow* Window, f32 DeltaTime, DynamicMapSystem::MapCamera& MainCamera);

	static Platform::BMRTMPWindowHandler* Window = nullptr;

	static DynamicMapSystem::MapCamera MainCamera;
	static bool Close = false;
	static bool FirstMouse = true;
	static f32 LastX = 400, LastY = 300;
	static f32 Yaw = -90.0f;
	static f32 Pitch = 0.0f;

	static f64 DeltaTime = 0.0f;
	static f64 LastTime = 0.0f;

	static const f32 Near = 0.1f;
	static const f32 Far = 5000.0f;

	static RenderResources::DrawEntity SkyBox;
	static Render::LightBuffer LightData;

	static DebugUi::GuiData GuiData;

	static glm::vec3 Eye = glm::vec3(0.0f, 10.0f, 0.0f);
	static glm::vec3 Up = glm::vec3(0.0f, 0.0f, -1.0f);

	static glm::vec3 CameraSphericalPosition = glm::vec3(0.0f, 0.0f, 6371.0f);
	static s32 Zoom = 4;

	static FrameManager::ViewProjectionBuffer ViewProjection;

	int Main()
	{
		Init();

		Memory::BmMemoryManagementSystem::FrameFree();

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

		DeInit();

		return 0;
	}

	bool Init()
	{
		u32 WindowWidth = 1920;
		u32 WindowHeight = 1080;

		Window = Platform::CreatePlatformWindow(WindowWidth, WindowHeight);
		//Platform::DisableCursor(Window);

		Platform::GetWindowSizes(glfwGetWin32Window(Window), &WindowWidth, &WindowHeight);

		LoadSettings(WindowWidth, WindowHeight);

		InitSystems();

		

		SetUpScene();
		 
		return true;
	}

	bool InitSystems()
	{
		const u32 FrameAllocSize = 1024 * 1024;
		Memory::BmMemoryManagementSystem::Init(FrameAllocSize);

		VulkanInterface::VulkanInterfaceConfig RenderConfig;
		//RenderConfig.MaxTextures = 90;
		RenderConfig.MaxTextures = 500; // TODO: FIX!!!!
		RenderConfig.LogHandler = RenderLog;


		 

		VulkanInterface::Init(glfwGetWin32Window(Window), RenderConfig);
		RenderResources::Init(MB32, MB32, 512, 256);
		FrameManager::Init();
		Render::Init();
		DeferredPass::Init();
		MainPass::Init();
		ResourceManager::Init();
		LightningPass::Init();
		TerrainRender::Init();
		//DynamicMapSystem::Init();
		StaticMeshRender::Init();
		DebugUi::Init(Window, &GuiData);

		return true;
	}

	void DeInit()
	{
		//DynamicMapSystem::DeInit();
		DebugUi::DeInit();
		TerrainRender::DeInit();
		StaticMeshRender::DeInit();

		RenderResources::DeInit();
		Render::DeInit();
		MainPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();

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
		MoveCamera(Window, DeltaTime, MainCamera);

		CameraSphericalPosition.z = DynamicMapSystem::CalculateCameraAltitude(Zoom);
		CameraSphericalPosition.z += 1.0f;

		MainCamera.Position = DynamicMapSystem::SphericalToMercator(CameraSphericalPosition);
		
		MainCamera.Front = glm::normalize(-MainCamera.Position);

		const glm::vec3 NorthPole(0.0f, 1.0f, 0.0f);
		const glm::vec3 Right = glm::normalize(glm::cross(NorthPole, MainCamera.Front));
		MainCamera.Up = glm::normalize(glm::cross(MainCamera.Front, Right));

		FrameManager::UpdateViewProjection(&ViewProjection);

		f32 AspectRatio = f32(MainScreenExtent.width) / f32(MainScreenExtent.height);
		//DynamicMapSystem::Update(DeltaTime, MainCamera, Zoom);

		static f32 Angle = 0.0f;

		Angle += 0.5f * static_cast<f32>(DeltaTime);
		if (Angle > 360.0f)
		{
			Angle -= 360.0f;
		}

		//for (int i = 0; i < Scene.DrawEntitiesCount; ++i)
		//{
		//	glm::mat4 TestMat = glm::rotate(Scene.DrawEntities[i].Model, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
		//}

		ViewProjection.View = glm::lookAt(MainCamera.Position, MainCamera.Position + MainCamera.Front, MainCamera.Up);

		float NearPlane = 0.1f, FarPlane = 100.0f;
		float HalfSize = 30.0f;
		glm::mat4 LightProjection = glm::ortho(-HalfSize, HalfSize, -HalfSize, HalfSize, NearPlane, FarPlane);

		glm::vec3 Center = Eye + LightData.DirectionLight.Direction;
		glm::mat4 LightView = glm::lookAt(Eye, Center, Up);

		LightData.DirectionLight.LightSpaceMatrix = LightProjection * LightView;
		LightData.SpotLight.Direction = MainCamera.Front;
		LightData.SpotLight.Position = MainCamera.Position;
		LightData.SpotLight.Planes = glm::vec2(Near, Far);
		LightData.SpotLight.LightSpaceMatrix = ViewProjection.Projection * ViewProjection.View;

		Scene.LightEntity = &LightData;

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
	}

	void SetUpScene()
	{
		MainCamera.Fov = 60.0f;
		MainCamera.AspectRatio = (float)MainScreenExtent.width / (float)MainScreenExtent.height;

		MainCamera.Position = glm::vec3(0.0f, 0.0f, 20.0f);
		MainCamera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
		MainCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);

		Scene.SkyBox = SkyBox;
		//Scene.DrawSkyBox = true;
		Scene.DrawSkyBox = false;

		ViewProjection.Projection = glm::perspective(glm::radians(MainCamera.Fov),
			MainCamera.AspectRatio, Near, Far);
		ViewProjection.Projection[1][1] *= -1;
		ViewProjection.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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

		GuiData.DirectionLightDirection = &LightData.DirectionLight.Direction;
		GuiData.Eye = &Eye;
		GuiData.CameraMercatorPosition = &CameraSphericalPosition;
		GuiData.Zoom = &Zoom;
		GuiData.OnTestSetDownload = DynamicMapSystem::TestSetDownload;
	}

	void RenderLog(VulkanInterface::LogType LogType, const char* Format, va_list Args)
	{
		switch (LogType)
		{
			case VulkanInterface::LogType::BMRVkLogType_Error:
			{
				std::cout << "\033[31;5mError: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				assert(false);
				break;
			}
			case VulkanInterface::LogType::BMRVkLogType_Warning:
			{
				std::cout << "\033[33;5mWarning: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				break;
			}
			case VulkanInterface::LogType::BMRVkLogType_Info:
			{
				std::cout << "Info: ";
				vprintf(Format, Args);
				std::cout << '\n';
				break;
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
		/*const char* SkyBoxObj = "./Resources/Models/SkyBox.obj";

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

		SkyBox.VertexOffset = Render::LoadVertices(vertices.data(), sizeof(StaticMeshVertex), vertices.size());
		SkyBox.IndexOffset = Render::LoadIndices(indices.data(), indices.size());
		SkyBox.IndicesCount = indices.size();
		SkyBox.TextureSet = Material;*/
	}
}
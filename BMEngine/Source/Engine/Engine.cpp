#include "Engine.h"

#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <thread>
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/UI/UI.h"
#include "Util/Settings.h"
#include "Engine/Systems/Render/Render.h"
#include "Util/Util.h"
#include "Util/Math.h"
#include "Deprecated/FrameManager.h"
#include "Engine/Systems/EngineResources.h"
#include "Engine/Systems/Render/TransferSystem.h"
#include "Engine/Systems/Concurrency/TaskSystem.h"

#include <gli/gli.hpp>

namespace Engine
{
	struct Camera
	{
		f32 Fov;
		f32 AspectRatio;
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
	};



	static bool Init();
	static bool InitSystems();
	static void DeInit();

	static void Update(f64 DeltaTime);

	static void SetUpScene();

	static void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera);

	static GLFWwindow* Window = nullptr;
	static bool IsMinimized = false;

	static Camera MainCamera;
	static bool Close = false;
	static bool FirstMouse = true;
	static f32 LastX = 400, LastY = 300;
	static f32 Yaw = -90.0f;
	static f32 Pitch = 0.0f;

	static f64 DeltaTime = 0.0f;
	static f64 LastTime = 0.0f;

	static const f32 Near = 0.1f;
	static const f32 Far = 5000.0f;

	static Render::DrawEntity SkyBox;
	static Render::LightBuffer LightData;

	static UI::GuiData GuiData;

	static glm::vec3 Eye = glm::vec3(0.0f, 10.0f, 0.0f);
	static glm::vec3 Up = glm::vec3(0.0f, 0.0f, -1.0f);

	static glm::vec3 CameraSphericalPosition = glm::vec3(0.0f, 0.0f, 6371.0f);
	static s32 Zoom = 4;

	static FrameManager::ViewProjectionBuffer ViewProjection;



	static Render::DrawScene Scene;



	void WindowIconifyCallback(GLFWwindow* window, int iconified)
	{
		if (iconified)
		{
			IsMinimized = true;
		}
		else
		{
			IsMinimized = false;
		}
	}

	int Main()
	{
		Init();

		TaskSystem::TaskGroup RenderGroup;

		while (!glfwWindowShouldClose(Window) && !Close)
		{
			glfwPollEvents();

			const f64 CurrentTime = glfwGetTime();
			DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;

			Update(DeltaTime);

			if (!IsMinimized)
			{
				TaskSystem::AddTask(TransferSystem::Transfer, &RenderGroup, 1);
				TaskSystem::AddTask([] () { Render::Draw(&Scene); }, &RenderGroup, 1);
				TaskSystem::WaitForGroup(&RenderGroup, 1);
			}
		}

		DeInit();

		return 0;
	}

	bool Init()
	{
		s32 WindowWidth = 1920;
		s32 WindowHeight = 1080;

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
		//glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwSetWindowIconifyCallback(Window, WindowIconifyCallback);
		glfwGetFramebufferSize(Window, &WindowWidth, &WindowHeight);

		LoadSettings(WindowWidth, WindowHeight);

		InitSystems();

		SetUpScene();
		 
		return true;
	}

	bool InitSystems()
	{
		Render::TmpInitFrameMemory();

		TaskSystem::Init();

		UI::Init(&GuiData);

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/RenderResources.yaml");

		RenderResources::Init(Window);
		RenderResources::CreateVertices(Util::GetVertices(Root));
		RenderResources::CreateShaders(Util::GetShaders(Root));
		RenderResources::CreateSamplers(Util::GetSamplers(Root));
		RenderResources::CreateDescriptorLayouts(Util::GetDescriptorSetLayouts(Root));
		RenderResources::PostCreateInit();

		TransferSystem::Init();
		Render::Init(Window);

		Scene.DrawEntities = Memory::AllocateArray<Render::DrawEntity>(512);

		Yaml::Node TestScene;
		Yaml::Parse(TestScene, "./Resources/Scenes/TestScene.yaml");
		Yaml::Node& SceneResourcesNode = Util::GetSceneResources(TestScene);

		EngineResources::Init(SceneResourcesNode, &Scene);

		return true;
	}

	void DeInit()
	{
		Render::DeInit();
		TransferSystem::DeInit();
		RenderResources::DeInit();
		EngineResources::DeInit();
		UI::DeInit();

		Memory::FreeArray(&Scene.DrawEntities);

		glfwDestroyWindow(Window);

		glfwTerminate();

		TaskSystem::DeInit();
		Memory::MemoryManagementSystem::DeInit();
	}

	void Update(f64 DeltaTime)
	{
		MoveCamera(Window, DeltaTime, MainCamera);

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

		Scene.ViewProjection = ViewProjection;

		UI::Update();
	}

	void SetUpScene()
	{
		MainCamera.Fov = 45.0f;
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
	}

	void MoveCamera(GLFWwindow* Window, f32 DeltaTime, Camera& MainCamera)
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
}
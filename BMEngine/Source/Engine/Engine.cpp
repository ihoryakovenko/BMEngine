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

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Util/Settings.h"
#include "Engine/Systems/Render/Render.h"
#include "Util/Util.h"
#include "Systems/DynamicMapSystem.h"
#include "Systems/ResourceManager.h"
#include "Scene.h"
#include "Util/Math.h"
#include "Render/FrameManager.h"
#include "Engine/Systems/Render/DebugUI.h"

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

	static void MoveCamera(GLFWwindow* Window, f32 DeltaTime, DynamicMapSystem::MapCamera& MainCamera);

	static GLFWwindow* Window = nullptr;

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

	static Render::DrawEntity SkyBox;
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

		Memory::MemoryManagementSystem::FrameFree();

		while (!glfwWindowShouldClose(Window) && !Close)
		{
			glfwPollEvents();

			const f64 CurrentTime = glfwGetTime();
			DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;

			Update(DeltaTime);
			Render::Draw(&ViewProjection);

			Memory::MemoryManagementSystem::FrameFree();
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

		glfwGetFramebufferSize(Window, &WindowWidth, &WindowHeight);

		LoadSettings(WindowWidth, WindowHeight);

		InitSystems();

		

		SetUpScene();
		 
		return true;
	}

	bool InitSystems()
	{
		const u32 FrameAllocSize = 1024 * 1024;
		Memory::MemoryManagementSystem::Init(FrameAllocSize);


		//Memory::RingBuffer<u8> Test = Memory::AllocateRingBuffer<u8>(4);
		//auto p = Memory::RingAlloc(&Test, 1u);
		//auto p1 = Memory::RingAlloc(&Test, 1u);
		//auto p2 = Memory::RingAlloc(&Test, 1u);
		//auto p22 = Memory::RingAlloc(&Test, 1u);
		//Memory::RingFree(&Test, p, 1);
		//Memory::RingFree(&Test, p1, 1);
		//auto p3 = Memory::RingAlloc(&Test, 2u);
		//Memory::RingFree(&Test, p2, 1);
		//auto p4 = Memory::RingAlloc(&Test, 1u);
		//Memory::RingFree(&Test, p3, 2);

		Memory::RingBufferControl Test{ };
		Test.Capacity = 5;
		auto p = Memory::RingAlloc(&Test, 1u);
		auto p1 = Memory::RingAlloc(&Test, 1u);
		auto p2 = Memory::RingAlloc(&Test, 1u);
		auto p22 = Memory::RingAlloc(&Test, 1u);
		Memory::RingFree(&Test, 1);
		Memory::RingFree(&Test, 1);
		auto p3 = Memory::RingAlloc(&Test, 2u);
		Memory::RingFree(&Test, 1);
		Memory::RingFree(&Test, 1);
		Memory::RingFree(&Test, 2);




		Render::Init(Window, &GuiData);

		
		ResourceManager::Init();
		//ResourceManager::LoadTextures(".\\Resources\\Textures");

		//VulkanInterface::WaitDevice();

		ResourceManager::LoadModel(".\\Resources\\Models\\uh60.model", ".\\Resources\\Textures");
		
		

		//{
		//	glm::vec3 CubePos(0.0f, -5.0f, 0.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(20.0f, 1.0f, 20.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
		//	//LoadModel("./Resources/Models/cube.obj", Model, ResourceManager::FindMaterial("WhiteMaterial"));
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 0.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(0.2f, 5.0f, 0.2f));
		//	LoadModel("./Resources/Models/cube.obj", Model, TestMaterial);
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 8.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::rotate(Model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//	Model = glm::scale(Model, glm::vec3(0.5f));
		//	LoadModel("./Resources/Models/cube.obj", Model, GrassMaterial);
		//}

		//{
		//	glm::vec3 LightCubePos(0.0f, 0.0f, 10.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, LightCubePos);
		//	Model = glm::scale(Model, glm::vec3(0.2f));
		//	LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
		//}

		//{
		//	glm::vec3 CubePos(0.0f, 0.0f, 15.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::scale(Model, glm::vec3(1.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, WhiteMaterial);
		//}

		//{
		//	glm::vec3 CubePos(5.0f, 0.0f, 10.0f);
		//	glm::mat4 Model = glm::mat4(1.0f);
		//	Model = glm::translate(Model, CubePos);
		//	Model = glm::rotate(Model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//	Model = glm::scale(Model, glm::vec3(1.0f));
		//	LoadModel("./Resources/Models/cube.obj", Model, ContainerMaterial);
		//}

		return true;
	}

	void DeInit()
	{
		Render::DeInit();

		glfwDestroyWindow(Window);

		glfwTerminate();

		Memory::MemoryManagementSystem::DeInit();

		if (Memory::MemoryManagementSystem::AllocateCounter != 0)
		{
			Util::Log::Error("AllocateCounter in not equal 0, counter is {}", Memory::MemoryManagementSystem::AllocateCounter);
			assert(false);
		}
	}

	void Update(f64 DeltaTime)
	{
		MoveCamera(Window, DeltaTime, MainCamera);

		//CameraSphericalPosition.z = DynamicMapSystem::CalculateCameraAltitude(Zoom);
		//CameraSphericalPosition.z += 1.0f;

		//MainCamera.Position = DynamicMapSystem::SphericalToMercator(CameraSphericalPosition);
		//
		//MainCamera.Front = glm::normalize(-MainCamera.Position);

		//const glm::vec3 NorthPole(0.0f, 1.0f, 0.0f);
		//const glm::vec3 Right = glm::normalize(glm::cross(NorthPole, MainCamera.Front));
		//MainCamera.Up = glm::normalize(glm::cross(MainCamera.Front, Right));

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
		GuiData.OnTestSetDownload = DynamicMapSystem::TestSetDownload;
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
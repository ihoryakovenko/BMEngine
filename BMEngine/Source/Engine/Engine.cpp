#include "Engine.h"

#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <thread>
#include <filesystem>

#include <SharedLib.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/UI/UI.h"
#include "Util/Settings.h"
#include "Engine/Systems/Render/Render.h"
#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/Math.h"
#include "Engine/Systems/EngineResources.h"
#include "Engine/Systems/Render/TransferSystem.h"
#include "Engine/Systems/Concurrency/TaskSystem.h"
#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

#include <gli/gli.hpp>

// Global resource maps
std::unordered_map<std::string, Util::VertexBinding_depr> VBindings;
std::unordered_map<std::string, BmRender_Sampler> Samplers;
std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
std::unordered_map<std::string, BmRender_Shader> Shaders;
std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;
std::unordered_map<std::string, BmRender_PushConstant> PushConstants;

namespace Engine
{
	static void ParseAndCreateVertices(Yaml::Node& VerticesNode)
	{
		for (auto VertexIt = VerticesNode.Begin(); VertexIt != VerticesNode.End(); VertexIt++)
		{
			Yaml::Node& VertexNode = (*VertexIt).second;

			Util::VertexBinding_depr Binding = Util::ParseVertexBindingNode(Util::GetVertexBindingNode(VertexNode));

			Yaml::Node& AttributesNode = Util::GetVertexAttributesNode(VertexNode);

			u32 Offset = 0;
			u32 Stride = 0;
			for (auto AttributeIt = AttributesNode.Begin(); AttributeIt != AttributesNode.End(); AttributeIt++)
			{
				Yaml::Node& TypeNode = Util::GetVertexAttributeTypeNode((*AttributeIt).second);
				std::string TypeStr = TypeNode.As<std::string>();

				VertexAttribute Attribute = { };
				std::string AttributeName;
				Util::ParseVertexAttributeNode((*AttributeIt).second, &Attribute, &AttributeName);

				u32 Size = Util::GetAttributeTypeSize(Attribute.Type);

				Attribute.Offset = Offset;
				Offset += Size;
				Stride += Size;

				Binding.Attributes[AttributeName] = Attribute;
			}

			Binding.Stride = Stride;
			VBindings[(*VertexIt).first] = Binding;
		}
	}

	static void ParseAndCreateShaders(Yaml::Node& ShadersNode)
	{
		for (auto It = ShadersNode.Begin(); It != ShadersNode.End(); It++)
		{
			std::string ShaderPath = Util::ParseShaderNode((*It).second);
			BmRender_PipelineShaderStage ShaderStage = Util::ParseShaderPipelineStage((*It).second);

			std::vector<char> ShaderCode;
			if (Util::OpenAndReadFileFull(ShaderPath.c_str(), ShaderCode, "rb"))
			{
				BmRender_ShaderDescription ShaderDesc = {};
				ShaderDesc.Code = reinterpret_cast<const u32*>(ShaderCode.data());
				ShaderDesc.CodeSize = ShaderCode.size();
				ShaderDesc.Stage = ShaderStage;
				Shaders[(*It).first] = BmRender_CreateShader(&ShaderDesc);
			}
			else
			{
				assert(false);
			}
		}
	}

	static void ParseAndCreateSamplers(Yaml::Node& SamplersNode)
	{
		for (auto It = SamplersNode.Begin(); It != SamplersNode.End(); It++)
		{
			BmRHI_SamplerDescription Data = Util::ParseSamplerNode((*It).second);
			Samplers[(*It).first] = BmRender_CreateSampler(&Data);
		}
	}

	static void ParseAndCreateDescriptorSetLayouts(Yaml::Node& DescriptorSetLayoutsNode)
	{
		std::vector<Util::DescriptorSetLayout> Layouts = Util::ParseDescriptorSetLayouts(DescriptorSetLayoutsNode);
		
		for (const auto& Layout : Layouts)
		{
			std::vector<BmRender_DescriptorSetLayoutBinding> Bindings;
			
			// Convert our simple structs to Vulkan structures
			for (u32 i = 0; i < Layout.Bindings.size(); ++i)
			{
				const auto& Binding = Layout.Bindings[i];
				
				BmRender_DescriptorSetLayoutBinding VkBinding = {};
				VkBinding.StageFlags = Binding.StageFlags;
				
				// Map shader types to Vulkan descriptor types
				switch (Binding.Type)
				{
				case Util::ShaderType::Uniform:
					VkBinding.DescriptorType = (Binding.MemoryFlag == MemoryPropertyFlag::HostCompatible) ?
						BmRender_DescriptorType::UniformBufferDynamic : BmRender_DescriptorType::UniformBuffer;
					VkBinding.DescriptorCount = 1;

					break;
				case Util::ShaderType::Buffer:
					VkBinding.DescriptorType = (Binding.MemoryFlag == MemoryPropertyFlag::HostCompatible) ?
						BmRender_DescriptorType::StorageBufferDynamic : BmRender_DescriptorType::StorageBuffer;
					VkBinding.DescriptorCount = 1;

						break;
					case Util::ShaderType::Sampler2D:
						VkBinding.DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
						VkBinding.DescriptorCount = 1;

						break;
					case Util::ShaderType::Sampler2DArray:
						VkBinding.DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
						VkBinding.DescriptorCount = 64;

						break;
				}
				
				Bindings.push_back(VkBinding);
			}
			
			DescriptorSetLayouts[Layout.Name] = BmRender_CreateDescriptorSetLayout(Bindings.data(), static_cast<u32>(Bindings.size()));
		}
	}

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
	static LightCastersData LightData;

	static UI::GuiData GuiData;

	static glm::vec3 Eye = glm::vec3(0.0f, 10.0f, 0.0f);
	static glm::vec3 Up = glm::vec3(0.0f, 0.0f, -1.0f);

	static glm::vec3 CameraSphericalPosition = glm::vec3(0.0f, 0.0f, 6371.0f);
	static s32 Zoom = 4;

	static UboViewProjection ViewProjection;



	static Render::DrawScene Scene;

	static Render::DescriptorSetHandles DescriptorSets;

	BmRender_GPUBufferBinding VpRegion[3];
	BmRender_GPUBufferBinding EntityLightRegion[3];
	
	// Buffer handles
	BmRender_GPUBuffer VertexStageBuffer;
	BmRender_GPUBuffer InstanceBuffer;
	BmRender_GPUBuffer FrameDataBuffer;
	BmRender_GPUBuffer MaterialBuffer;

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

		TaskSystem::TaskGroup Group;
		Group.TasksInGroup = 0;

		u32 LastTransfer = 0;

		while (!glfwWindowShouldClose(Window) && !Close)
		{
			glfwPollEvents();

			const f64 CurrentTime = glfwGetTime();
			DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;

			Update(DeltaTime);

			if (!IsMinimized)
			{			
				EngineResources::Update(&Scene, DescriptorSets.BindlesTexturesSet);

				//TaskSystem::TaskLambda Task = [&]() { TransferSystem::Transfer(); };
				//TaskSystem::AddTask(&Task, &Group);
				TransferSystem::Transfer();
				Render::Draw(&Scene, LastTransfer);

				//TaskSystem::WaitForGroup(&Group);
			}

			Memory_LinearAllocator_FreeMemory(Memory::GetGeneralFrameMemory());
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
		Memory::Init(true);

		TaskSystem::Init();
		//TaskSystem::SetConcurencyEnabled(false);

		UI::Init(&GuiData);

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/RenderResources.yaml");

		BmRender_Init(Window);
		
		// Create MainPool using stack array
		const u32 PoolSizeCount = 11;
		BmRender_DescriptorPoolSize TotalPassPoolSizes[PoolSizeCount];
		u32 TotalDescriptorLayouts = 21;
		TotalPassPoolSizes[0] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[1] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[2] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[3] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[4] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[5] = { BmRender_DescriptorType::InputAttachment, 3 };
		TotalPassPoolSizes[6] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[7] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[8] = { BmRender_DescriptorType::UniformBuffer, 3 };
		TotalPassPoolSizes[9] = { BmRender_DescriptorType::CombinedImageSampler, 256 };
		TotalPassPoolSizes[10] = { BmRender_DescriptorType::UniformBufferDynamic, 3 };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * 3;
		TotalDescriptorCount += 256;

		BmRender_DescriptorPool MainPool = BmRender_CreateDescriptorPool(TotalPassPoolSizes, TotalDescriptorCount, PoolSizeCount, BmRender_DescriptorPoolType::UpdateAfterBind);
		VertexStageBuffer = BmRender_CreateVertexStageBuffer(MB4, MemoryPropertyFlag::GPULocal);
		InstanceBuffer = BmRender_CreateInstanceBuffer(MB4, MemoryPropertyFlag::GPULocal);
		FrameDataBuffer = BmRender_CreateUniformBuffer(65536, MemoryPropertyFlag::HostCompatible);
		MaterialBuffer = BmRender_CreateStorageBuffer(MB4, MemoryPropertyFlag::GPULocal);

		const u32 ViewProjectionBufferSize = sizeof(UboViewProjection);

		VpRegion[0] = { FrameDataBuffer, 0, ViewProjectionBufferSize };
		VpRegion[1] = { FrameDataBuffer, ViewProjectionBufferSize, ViewProjectionBufferSize };
		VpRegion[2] = { FrameDataBuffer, ViewProjectionBufferSize * 2, ViewProjectionBufferSize };

		const u32 LightBufferSize = sizeof(LightCastersData);

		EntityLightRegion[0] = { FrameDataBuffer, ViewProjectionBufferSize * 3, LightBufferSize };
		EntityLightRegion[1] = { FrameDataBuffer, ViewProjectionBufferSize * 3 + LightBufferSize, LightBufferSize };
		EntityLightRegion[2] = { FrameDataBuffer, ViewProjectionBufferSize * 3 + LightBufferSize + LightBufferSize, LightBufferSize };

		ParseAndCreateVertices(Util::GetVertices(Root));
		ParseAndCreateShaders(Util::GetShaders(Root));
		ParseAndCreateSamplers(Util::GetSamplers(Root));
		ParseAndCreateDescriptorSetLayouts(Util::GetDescriptorSetLayouts(Root));
		Util::ParseAndCreatePushConstants(Util::GetPushConstantsFromResources(Root));

		DescriptorSets = Render::DescriptorSetHandles();
		
		{
			BmRender_DescriptorSetBinding Binding;
			Binding.BufferRegions = VpRegion;
			Binding.BindingCount = 1;
			Binding.DstArrayElement = 0;

			DescriptorSets.VpSet = BmRender_CreateDescriptorSet(DescriptorSetLayouts["FrameDataLayout"], MainPool);
			BmRender_UpdateDescriptorSet(DescriptorSets.VpSet, &Binding, 1);
		}

		{
			BmRender_DescriptorSetBinding Binding;
			Binding.BufferRegions = EntityLightRegion;
			Binding.BindingCount = 1;
			Binding.DstArrayElement = 0;

			DescriptorSets.StaticMeshLightSet = BmRender_CreateDescriptorSet(DescriptorSetLayouts["FrameDataLayout"], MainPool);
			BmRender_UpdateDescriptorSet(DescriptorSets.StaticMeshLightSet, &Binding, 1);
		}

		{
			BmRender_GPUBufferBinding MaterialBufferRegion = { MaterialBuffer, 0, VK_WHOLE_SIZE };

			BmRender_DescriptorSetBinding Binding;
			Binding.BufferRegions = &MaterialBufferRegion;
			Binding.BindingCount = 1;
			Binding.DstArrayElement = 0;

			DescriptorSets.MaterialSet = BmRender_CreateDescriptorSet(DescriptorSetLayouts["MaterialLayout"], MainPool);
			BmRender_UpdateDescriptorSet(DescriptorSets.MaterialSet, &Binding, 1);
		}

		{
			DescriptorSets.BindlesTexturesSet = BmRender_CreateDescriptorSet(DescriptorSetLayouts["BindlesTexturesLayout"], MainPool);
		}

		TransferSystem::Init();
		Render::Init(Window, VpRegion, EntityLightRegion, DescriptorSets, MainPool);

		EngineResources::Init(DescriptorSets.BindlesTexturesSet, VertexStageBuffer, InstanceBuffer, FrameDataBuffer, MaterialBuffer);

		Yaml::Node TestScene;
		Yaml::Parse(TestScene, "./Resources/Scenes/TestScene.yaml");
		Yaml::Node& SceneResourcesNode = Util::GetSceneResources(TestScene);

		Yaml::Node& TexturesNode = Util::GetTextures(SceneResourcesNode);
		for (auto It = TexturesNode.Begin(); It != TexturesNode.End(); It++)
		{
			EngineResources::RegisterTextureAsset((*It).first, (*It).second.As<std::string>());
		}

		Yaml::Node& ModelsNode = Util::GetModels(SceneResourcesNode);
		for (auto It = ModelsNode.Begin(); It != ModelsNode.End(); It++)
		{
			std::string ModelName = (*It).first;
			Yaml::Node& ModelNode = (*It).second;

			EngineResources::ModelLoadRequest Request;
			Request.Path = Util::GetModelPath(ModelNode);
			Request.Position = Util::GetModelPosition(ModelNode);

			EngineResources::RequestModelLoad(Request);
		}

		return true;
	}

	void DeInit()
	{
		Render::DeInit();
		TransferSystem::DeInit();
		EngineResources::DeInit();
		UI::DeInit();

		// Destroy GPUBuffers
		BmRender_DestroyGPUBuffer(VertexStageBuffer);
		BmRender_DestroyGPUBuffer(InstanceBuffer);
		BmRender_DestroyGPUBuffer(FrameDataBuffer);
		BmRender_DestroyGPUBuffer(MaterialBuffer);

		for (auto& [name, layout] : DescriptorSetLayouts)
		{
			BmRender_DestroyDescriptorSetLayout(layout);
		}
		DescriptorSetLayouts.clear();

		for (auto& [name, shader] : Shaders)
		{
			BmRender_DestroyShader(shader);
		}
		Shaders.clear();

		BmRender_DeInit();

		glfwDestroyWindow(Window);

		glfwTerminate();

		TaskSystem::DeInit();
		Memory::DeInit();
	}

	void Update(f64 DeltaTime)
	{
		Memory::Update();

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

		glm::vec3 Center = Eye + LightData.directionLight.Direction;
		glm::mat4 LightView = glm::lookAt(Eye, Center, Up);

		LightData.directionLight.LightSpaceMatrix = LightProjection * LightView;
		LightData.spotlight.Direction = MainCamera.Front;
		LightData.spotlight.Position = MainCamera.Position;
		LightData.spotlight.Planes = glm::vec2(Near, Far);
		LightData.spotlight.LightSpaceMatrix = ViewProjection.Projection * ViewProjection.View;

		Scene.LightEntity = &LightData;

		GuiData.DirectionLightDirection = &LightData.directionLight.Direction;
		GuiData.Eye = &Eye;

		Scene.ViewProjection = ViewProjection;

		UI::Update();
	}

	void SetUpScene()
	{
		MainCamera.Fov = 45.0f;
		MainCamera.AspectRatio = (float)MainScreenExtent.Width / (float)MainScreenExtent.Height;

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

		LightData.pointlight.Position = glm::vec4(0.0f, 0.0f, 2.0f, 1.0f);
		LightData.pointlight.Color = glm::vec3(1.0f, 1.0f, 1.0f);

		LightData.directionLight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		LightData.directionLight.Color = glm::vec3(1.0f, 1.0f, 1.0f);

		LightData.spotlight.Position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
		LightData.spotlight.Color = glm::vec3(1.0f, 1.0f, 1.0f);
		LightData.spotlight.CutOff = glm::cos(glm::radians(12.5f));
		LightData.spotlight.OuterCutOff = glm::cos(glm::radians(17.5f));

		GuiData.DirectionLightDirection = &LightData.directionLight.Direction;
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
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Render/VulkanInterface/VulkanInterface.h"

#include "Util/EngineTypes.h"
#include "Render/FrameManager.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/StaticMeshRender.h"

#include "Render/FrameManager.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace DebugUi
{
	struct GuiData;
}

namespace Render
{
	struct StaticMesh
	{
		u64 VertexOffset;
		u32 IndexOffset;
		u32 IndicesCount;
	};

	struct Texture
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size;
		u64 Alignment;
	};

	struct MeshTexture2D
	{
		Texture MeshTexture;
		VkImageView View;
	};

	struct Material
	{
		u32 AlbedoTexIndex;
		u32 SpecularTexIndex;
		float Shininess;
	};

	struct DrawEntity
	{
		u64 StaticMeshIndex;
		u32 MaterialIndex;
		glm::mat4 Model;
	};

	struct GPUBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		u64 Alignment;
		u64 Offset;
	};

	struct StagingPool
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		u64 Offset;
		u64 Alignment;
	};

	struct StaticMeshStorage
	{
		GPUBuffer VertexBuffer;
		GPUBuffer IndexBuffer;

		Memory::DynamicArray<StaticMesh> StaticMeshes;
	};

	enum TransferTaskState
	{
		TRANSFER_IDLE,
		TRANSFER_IN_FLIGHT,
		TRANSFER_COMPLETE
	};

	struct MaterialStorage
	{
		VkBuffer MaterialBuffer;
		VkDeviceMemory MaterialBufferMemory;
		VkDescriptorSetLayout MaterialLayout;
		VkDescriptorSet MaterialSet;
		u32 MaterialIndex;
	};

	struct TextureStorage
	{
		VkSampler DiffuseSampler;
		VkSampler SpecularSampler;
		VkDescriptorSetLayout BindlesTexturesLayout;
		VkDescriptorSet BindlesTexturesSet;
		MeshTexture2D* Textures;
		std::unordered_map<u64, u32> TexturesPhysicalIndexes;
		u32 TextureIndex;
	};

	struct TransferTask
	{
		DrawEntity Entity;
		TransferTaskState State;
	};

	static const u32 MaxTasks = 512;

	struct TaskQueue
	{
		Memory::RingBuffer<TransferTask> TaskBuffer;
		std::mutex Mutex;
	};

	struct TransferFrames
	{
		VkFence Fences[3];
		VkCommandBuffer CommandBuffers[3];
		TaskQueue Tasks[3];
	};

	struct DataTransferState
	{
		VkCommandPool TransferCommandPool;
		StagingPool TransferStagingPool;
		TransferFrames Frames;
	};

	struct RenderState
	{
		StaticMeshStorage Meshes;
		TextureStorage Textures;
		MaterialStorage Materials;

		Memory::DynamicArray<DrawEntity> DrawEntities;

		DataTransferState TransferState;
		std::mutex QueueSubmitMutex;	
	};

	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData);
	void DeInit();

	u64 CreateStaticMesh(const StaticMeshRender::StaticMeshVertex* Vertices, u64 VerticesCount, const u32* Indices, u64 IndicesCount, u32 FrameIndex);
	u32 CreateMaterial(Material* Mat, u32 FrameIndex);
	u32 CreateEntity(const DrawEntity* Entity, u32 FrameIndex);
	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height, u32 FrameIndex);

	void Draw(const FrameManager::ViewProjectionBuffer* Data);

	u32 GetTexture2DSRGBIndex(u64 Hash);
	RenderState* GetRenderState();

	

	





	struct TextureData
	{
		void* RawData;
		u32 Width;
		u32 Height;
		u64 Size;
	};

	struct VertexData
	{
		void* RawData;
		u64 Size;
	};







	struct PointLight
	{
		glm::vec4 Position;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
	};

	struct DirectionLight
	{
		glm::mat4 LightSpaceMatrix;
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct SpotLight
	{
		glm::mat4 LightSpaceMatrix;
		glm::vec3 Position;
		f32 CutOff;
		glm::vec3 Direction;
		f32 OuterCutOff;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
		alignas(16) glm::vec2 Planes;
	};

	struct LightBuffer
	{
		PointLight PointLight;
		DirectionLight DirectionLight;
		SpotLight SpotLight;

		glm::vec4 tmp;
		glm::vec4 tmp2;
	};

	struct DrawScene
	{
		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawEntity SkyBox;
		bool DrawSkyBox = false;

		DrawEntity MapEntity;

		LightBuffer* LightEntity = nullptr;
	};
}
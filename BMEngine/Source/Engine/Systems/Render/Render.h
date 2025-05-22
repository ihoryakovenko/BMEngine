#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Render/VulkanInterface/VulkanInterface.h"

#include "Util/EngineTypes.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Render/FrameManager.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/StaticMeshRender.h"

#include "Render/FrameManager.h"

#include <atomic>
#include <mutex>

namespace DebugUi
{
	struct GuiData;
}

namespace Render
{
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

	struct Texture
	{
		VkImage Image;
		VkDevice Memory;
		u64 Size;
		u64 Alignment;
	};

	struct StaticMesh
	{
		u64 VertexOffset;
		u32 IndexOffset;
		u32 IndicesCount;
	};

	struct GPUBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		u64 Alignment;
		u64 Offset;
	};

	struct StaticMeshStorage
	{
		GPUBuffer VertexBuffer;
		GPUBuffer IndexBuffer;

		Memory::DynamicArray<StaticMesh> StaticMeshes;
	};

	struct TexturesLoadQueue
	{
		Memory::DynamicArray<TextureData*> Textures;
		VkFence Fence;
	};

	struct TexturesReadyQueue
	{
		Memory::DynamicArray<Texture> Textures;
		VkFence Fence;
	};

	enum TransferTaskState
	{
		TRANSFER_IDLE,
		TRANSFER_IN_FLIGHT,
		TRANSFER_COMPLETE
	};

	struct TransferTask
	{
		RenderResources::DrawEntity Entity;
		TransferTaskState State;
	};

	static const u32 MaxTasks = 512;

	struct TaskQueue
	{
		TransferTask Tasks[MaxTasks];
		u32 Head;
		u32 Tail;
		u32 Count;
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
		TransferFrames Frames;
	};

	struct RenderState
	{
		DataTransferState TransferState;
		std::mutex QueueSubmitMutex;

		StaticMeshStorage StaticMeshLinearStorage;

		Memory::DynamicArray<RenderResources::DrawEntity> DrawEntities;

		TexturesLoadQueue TexturesLoad;
		TexturesReadyQueue TextureReady;
	};



	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData);
	void DeInit();

	u64 LoadStaticMesh(const StaticMeshRender::StaticMeshVertex* Vertices, u64 VerticesCount, const u32* Indices, u64 IndicesCount, u32 FrameIndex);
	u32 CreateEntity(const StaticMeshRender::StaticMeshVertex* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 MaterialIndex, u32 FrameIndex);

	void Draw(const FrameManager::ViewProjectionBuffer* Data);

	RenderState* GetRenderState();




	void Transfer(u32 FrameIndex);
	void QueueBufferDataLoad(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void QueueImageDataLoad(VkImage Image, u32 Width, u32 Height, void* Data);

	u64 TmpGetTransferTimelineValue();
	VkSemaphore TmpGetTransferCompleted();





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
		RenderResources::DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		RenderResources::DrawEntity SkyBox;
		bool DrawSkyBox = false;

		RenderResources::DrawEntity MapEntity;

		LightBuffer* LightEntity = nullptr;
	};
}
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/EngineTypes.h"
#include "Deprecated/FrameManager.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanCoreContext.h"

#include "Deprecated/FrameManager.h"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace DebugUi
{
	struct GuiData;
}

namespace Render
{
	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct StaticMesh
	{
		u64 VertexOffset;
		u32 IndexOffset;
		u32 IndicesCount;
		u64 VertexDataSize;
		bool IsLoaded;
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
		bool IsLoaded;
	};

	struct Material
	{
		u32 AlbedoTexIndex;
		u32 SpecularTexIndex;
		float Shininess;
		bool IsLoaded;
	};

	struct InstanceData
	{
		glm::mat4 ModelMatrix;
		u32 MaterialIndex;
		bool IsLoaded;
		uint32_t padding[2]; // tmp
	};

	struct DrawEntity
	{
		u64 StaticMeshIndex;
		u32 InstanceDataIndex;
		u32 Instances;
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
		Memory::RingBufferControl ControlBlock;
	};

	struct StaticMeshStorage
	{
		GPUBuffer VertexStageData;
		GPUBuffer GPUInstances;
		Memory::DynamicHeapArray<InstanceData> MeshInstances;
		Memory::DynamicHeapArray<StaticMesh> StaticMeshes;
	};

	struct MaterialStorage
	{
		Memory::DynamicHeapArray<Material> Materials;
		VkBuffer MaterialBuffer;
		VkDeviceMemory MaterialBufferMemory;
		VkDescriptorSetLayout MaterialLayout;
		VkDescriptorSet MaterialSet;
	};

	struct TextureStorage
	{
		VkSampler DiffuseSampler;
		VkSampler SpecularSampler;

		VkDescriptorSetLayout BindlesTexturesLayout;
		VkDescriptorSet BindlesTexturesSet;

		Memory::DynamicHeapArray<MeshTexture2D> Textures;
	};

	enum TransferTaskType
	{
		TransferTaskType_Texture,
		TransferTaskType_Mesh,
		TransferTaskType_Material,
		TransferTaskType_Instance,
	};

	struct TextureTaskDescription
	{
		VkImage DstImage;
		u32 Width;
		u32 Height;
	};

	struct DataTaskDescription
	{
		VkBuffer DstBuffer;
		u64 DstOffset;
	};

	struct TransferTask
	{
		TransferTaskType Type;

		union
		{
			TextureTaskDescription TextureDescr;
			DataTaskDescription DataDescr;
		};

		void* RawData;
		u64 DataSize;

		u32 ResourceIndex;
	};

	struct TaskQueue
	{
		Memory::HeapRingBuffer<TransferTask> TasksBuffer;
		std::mutex Mutex;
	};

	static const u32 MAX_DRAW_FRAMES = 3;

	struct DrawFrames
	{
		VkFence Fences[MAX_DRAW_FRAMES];
		VkCommandBuffer CommandBuffers[MAX_DRAW_FRAMES];
		VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
		VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	};

	struct TransferFrames
	{
		VkFence Fences[MAX_DRAW_FRAMES];
		VkCommandBuffer CommandBuffers[MAX_DRAW_FRAMES];
	};

	struct ResourceStorage
	{
		StaticMeshStorage Meshes;
		TextureStorage Textures;
		MaterialStorage Materials;
		Memory::DynamicHeapArray<DrawEntity> DrawEntities;
	};

	struct DrawState
	{
		VkCommandPool GraphicsCommandPool;
		VkDescriptorPool MainPool;

		DrawFrames Frames;
		u32 CurrentFrame;
		u32 CurrentImageIndex;
	};

	struct DataTransferState
	{
		const u64 MaxTransferSizePerFrame = MB4;

		Memory::HeapRingBuffer<u8> TransferMemory;

		VkCommandPool TransferCommandPool;
		StagingPool TransferStagingPool;

		VkSemaphore TransferSemaphore;
		u64 TasksInFly;
		u64 CompletedTransfer;

		TaskQueue PendingTransferTasksQueue;
		std::condition_variable PendingCv;
		TaskQueue ActiveTransferTasksQueue;

		TransferFrames Frames;
		u32 CurrentFrame;
	};

	struct StaticMeshPipeline
	{
		VulkanHelper::RenderPipeline Pipeline;
		VkSampler ShadowMapArraySampler;

		VkDescriptorSetLayout StaticMeshLightLayout;
		VkDescriptorSetLayout ShadowMapArrayLayout;

		FrameManager::UniformMemoryHnadle EntityLightBufferHandle;

		VkImageView ShadowMapArrayImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkPushConstantRange PushConstants;

		VkDescriptorSet StaticMeshLightSet;
		VkDescriptorSet ShadowMapArraySet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct RenderState
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		ResourceStorage RenderResources;
		DrawState RenderDrawState;
		DataTransferState TransferState;
		std::mutex QueueSubmitMutex;	

		StaticMeshPipeline MeshPipeline;
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
		FrameManager::ViewProjectionBuffer ViewProjection;

		Memory::DynamicHeapArray<DrawEntity> Entities;

		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawEntity SkyBox;
		bool DrawSkyBox = false;

		LightBuffer* LightEntity = nullptr;
	};

	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData);
	void DeInit();

	u64 CreateStaticMesh(void* MeshVertexData, u64 VertexSize, u64 VerticesCount, u64 IndicesCount);
	u32 CreateMaterial(Material* Mat);
	u32 CreateEntity(const DrawEntity* Entity);
	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height);
	u32 CreateStaticMeshInstance(InstanceData* Data);

	void Draw(const DrawScene* Data);
	void NotifyTransfer();

	RenderState* GetRenderState();
}
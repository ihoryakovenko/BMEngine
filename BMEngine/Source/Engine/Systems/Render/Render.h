#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/EngineTypes.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanCoreContext.h"
#include "RenderInterface.h"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>

#include "RenderInterface.h"

namespace Render
{
	struct ViewProjectionBuffer
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct DrawEntity
	{
		u64 VertexOffset;
		u64 InstanceOffset;
		u32 IndexOffset;
		u32 IndicesCount;
		u32 Instances;
		
		std::vector<BmRender_Image> ImageDependency;
		std::vector<BmRender_GPUBufferEntry> ResourceDependency;
	};

	struct DrawFrames
	{
		VkFence Fences[VulkanHelper::MAX_DRAW_FRAMES];
		VkCommandBuffer CommandBuffers[VulkanHelper::MAX_DRAW_FRAMES];
		VkSemaphore ImagesAvailable[VulkanHelper::MAX_DRAW_FRAMES];
		VkSemaphore RenderFinished[VulkanHelper::MAX_DRAW_FRAMES];
	};

	struct DrawState
	{
		VkCommandPool GraphicsCommandPool;

		DrawFrames Frames;
		u32 CurrentFrame;
		u32 CurrentImageIndex;

		u64 WaitSemaphoreValueCount;
	};

	struct StaticMeshPipeline
	{
		BmRender_GPUBufferEntry* EntityLightBufferHandle;

		BmRender_ImageView ShadowMapArrayImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkPushConstantRange PushConstants;

		BmRender_DescriptorSet ShadowMapArraySet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct DescriptorSetHandles
	{
		BmRender_DescriptorSet VpSet;
		BmRender_DescriptorSet BindlesTexturesSet;
		BmRender_DescriptorSet StaticMeshLightSet;
		BmRender_DescriptorSet MaterialSet;
	};

	struct RenderState
	{
		DrawState RenderDrawState;	
		StaticMeshPipeline MeshPipeline;
		DescriptorSetHandles DescriptorSets;
		BmRender_DescriptorPool MainPool;
		VkDescriptorPool DebugUiPool; // TODO: ?
		Memory::FrameMemory FrameMemory;
		BmRender_GPUBufferEntry* VpHandle;
		
		// Buffer handles
		BmRender_GPUBuffer VertexStageBuffer;
		BmRender_GPUBuffer InstanceBuffer;
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
		ViewProjectionBuffer ViewProjection;

		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawEntity SkyBox;
		bool DrawSkyBox = false;

		LightBuffer* LightEntity = nullptr;

		std::mutex TempLock;
		std::vector<DrawEntity> DrawEntities;
	};

	void TmpInitFrameMemory();

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferEntry* VpRegion, BmRender_GPUBufferEntry* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool, BmRender_GPUBuffer VertexStageBuffer, BmRender_GPUBuffer InstanceBuffer);
	void DeInit();

	void* FrameAlloc(u32 Size);
	void* GetHead();

	void Draw(DrawScene* Data, u64 WaitSemaphoreValue);

	RenderState* GetRenderState();
}

namespace DeferredPass
{
	void Init(BmRender_DescriptorPool MainPool);
	void Draw();

	void BeginPass();
	void EndPass();

	BmRender_ImageView* TestDeferredInputColorImageInterface();
	BmRender_ImageView* TestDeferredInputDepthImageInterface();

	BmRender_Image* TestDeferredInputColorImage();
	BmRender_Image* TestDeferredInputDepthImage();

	AttachmentData* GetAttachmentData();
}

namespace LightningPass
{
	void Init(BmRender_DescriptorPool MainPool);

	void Draw(Render::DrawScene* Scene);
}

namespace MainPass
{
	void Init();

	void BeginPass();
	void EndPass();

	AttachmentData* GetAttachmentData();
}
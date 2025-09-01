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

namespace Render
{
	struct DrawEntity
	{
		u64 StaticMeshIndex;
		u32 InstanceDataIndex;
		u32 Instances;
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
	};

	struct StaticMeshPipeline
	{
		VulkanHelper::RenderPipeline Pipeline;

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
		DrawState RenderDrawState;	
		StaticMeshPipeline MeshPipeline;
		VkDescriptorPool DebugUiPool; // TODO: ?
		Memory::FrameMemory FrameMemory;
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

		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawEntity SkyBox;
		bool DrawSkyBox = false;

		LightBuffer* LightEntity = nullptr;

		Memory::DynamicHeapArray<DrawEntity> DrawEntities;
	};

	void TmpInitFrameMemory();

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void* FrameAlloc(u32 Size);

	void Draw(const DrawScene* Data);

	RenderState* GetRenderState();
}

namespace DeferredPass
{
	void Init();
	void DeInit();

	void Draw();

	void BeginPass();
	void EndPass();

	VkImageView* TestDeferredInputColorImageInterface();
	VkImageView* TestDeferredInputDepthImageInterface();

	VulkanInterface::UniformImage* TestDeferredInputColorImage();
	VulkanInterface::UniformImage* TestDeferredInputDepthImage();

	VulkanHelper::AttachmentData* GetAttachmentData();
}

namespace LightningPass
{
	void Init();
	void DeInit();

	void Draw(const Render::DrawScene* Scene);

	VulkanInterface::UniformImage* GetShadowMapArray();
}

namespace MainPass
{
	void Init();
	void DeInit();

	void BeginPass();
	void EndPass();

	VulkanHelper::AttachmentData* GetAttachmentData();
}

namespace TerrainRender
{
	void Init();
	void DeInit();

	void Draw();
}
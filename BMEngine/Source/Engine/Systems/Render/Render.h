#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <ShortTypes.h>

#include <RenderInterface.h>

#include "Util/EngineTypes.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
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
		BmRender_GPUBufferBinding VertexBufferEntry;
		BmRender_GPUBufferBinding IndexBufferEntry;
		BmRender_GPUBufferBinding InstanceBufferEntry;
		u32 IndicesCount;
		u32 Instances;
		
		std::vector<BmRender_Image> ImageDependency;
		std::vector<BmRender_GPUBufferBinding> ResourceDependency;
	};

	struct StaticMeshPipeline
	{
		BmRender_ImageView ShadowMapArrayImageInterface[MAX_DRAW_FRAMES];

		VkPushConstantRange PushConstants;

		BmRender_DescriptorSet ShadowMapArraySet[MAX_DRAW_FRAMES];
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
		BmRender_CommandWorker GraphicsCommandWorker;
		StaticMeshPipeline MeshPipeline;
		DescriptorSetHandles DescriptorSets;
		BmRender_DescriptorPool MainPool;
		BmRender_DescriptorPool DebugUiPool; // TODO: ?
		BmRender_GPUBufferBinding* VpHandle;
		BmRender_GPUBufferBinding* EntityLightBufferHandle;
	};

	struct alignas(16) PointLight
	{
		glm::vec4 Position;

		glm::vec3 Color;
	};

	struct alignas(16) DirectionLight
	{
		glm::mat4 LightSpaceMatrix;

		glm::vec3 Direction;
		f32 pad1;

		glm::vec3 Color;
	};

	struct alignas(16) SpotLight
	{
		glm::mat4 LightSpaceMatrix;

		glm::vec3 Position;
		f32 CutOff;

		glm::vec3 Direction;
		f32 OuterCutOff;

		glm::vec3 Color;
		f32 pad1;

		glm::vec2 Planes;
	};

	struct alignas(16) LightBuffer
	{
		PointLight PointLight;
		DirectionLight DirectionLight;
		SpotLight SpotLight;
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

	void Init(GLFWwindow* WindowHandler, BmRender_GPUBufferBinding* VpRegion, BmRender_GPUBufferBinding* EntityLightRegion, const DescriptorSetHandles& DescriptorSets, BmRender_DescriptorPool MainPool);
	void DeInit();

	void Draw(DrawScene* Data, u64 WaitSemaphoreValue);

	RenderState* GetRenderState();

	struct DrawEntityBatchConfig
	{
		BmRender_Pipeline Pipeline;
		BmRender_PipelineLayout PipelineLayout;
		const BmRender_DescriptorSet* DescriptorSets;
		u32 DescriptorSetCount;
		u32 DynamicOffsetCount;
		const u32* DynamicOffsets;
		BmRender_PushConstant PushConstant;
		const void* PushConstantData;
	};

	void DrawEntityBatch(BmRender_CommandBuffer CmdBuffer, DrawScene* Scene, const DrawEntityBatchConfig& Config);

	// DeferredPass functions
	void DeferredPassInit(BmRender_DescriptorPool MainPool);
	void DeferredPassDeInit();
	void DeferredPassDraw();
	void DeferredPassBeginPass();
	void DeferredPassEndPass();
	BmRender_ImageView* TestDeferredInputColorImageInterface();
	BmRender_ImageView* TestDeferredInputDepthImageInterface();
	BmRender_Image* TestDeferredInputColorImage();
	BmRender_Image* TestDeferredInputDepthImage();
	AttachmentData* DeferredPassGetAttachmentData();

	// LightningPass functions
	void LightningPassInit(BmRender_DescriptorPool MainPool);
	void LightningPassDeInit();
	void LightningPassDraw(DrawScene* Scene);

	// MainPass functions
	void MainPassInit();
	void MainPassBeginPass();
	void MainPassEndPass();
	AttachmentData* MainPassGetAttachmentData();
}
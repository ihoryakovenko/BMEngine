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

#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

#include "RenderInterface.h"

namespace Render
{
	struct DrawEntity
	{
		BmRender_GPUBufferBinding VertexBufferEntry;
		BmRender_GPUBufferBinding IndexBufferEntry;
		BmRender_GPUBufferBinding InstanceBufferEntry;
		u32 IndicesCount;
		u32 Instances;
	};

	struct StaticMeshPipeline
	{
		BmRender_ImageView ShadowMapArrayImageInterface[MAX_DRAW_FRAMES];

		VkPushConstantRange PushConstants;

		BmRender_DescriptorSet ShadowMapArraySet[MAX_DRAW_FRAMES];
	};

	struct DescriptorSetHandles
	{
		BmRender_DescriptorSet MaterialsSet;
		BmRender_DescriptorSet FrameBufferSet;
		BmRender_DescriptorSet BindlesTexturesSet;
		BmRender_DescriptorSet EmptySet;
	};

	struct RenderState
	{
		BmRender_CommandWorker GraphicsCommandWorker;
		StaticMeshPipeline MeshPipeline;
		DescriptorSetHandles DescriptorSets;
		BmRender_DescriptorPool MainPool;
		BmRender_DescriptorPool DebugUiPool; // TODO: ?
	};

	struct DrawScene
	{
		Shader_FrameData FrameDataBuffer;

		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawEntity SkyBox;
		bool DrawSkyBox = false;

		std::mutex TempLock;
		std::vector<DrawEntity> DrawEntities;
	};

	void Init(GLFWwindow* WindowHandler);
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

	Render::DescriptorSetHandles* GetHandles();
	BmRender_GPUBuffer GetVertexBuffer();
	BmRender_GPUBuffer GetIndexBuffer();
	BmRender_GPUBuffer GetInstanceBuffer();
	BmRender_GPUBuffer GetMaterialBuffer();
}
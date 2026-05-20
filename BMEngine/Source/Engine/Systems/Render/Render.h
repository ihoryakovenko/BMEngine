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


namespace Render
{
	struct DrawEntity
	{
		BmRender_GPUBufferUpdateData VertexBufferEntry;
		BmRender_GPUBufferUpdateData IndexBufferEntry;
		BmRender_GPUBufferUpdateData InstanceBufferEntry;
		u32 IndicesCount;
		u32 Instances;
	};

	struct StaticMeshPipelineDepr
	{
		BmRender_ImageView ShadowMapArrayImageInterface[MAX_DRAW_FRAMES];

		VkPushConstantRange PushConstants;

		BmRender_DescriptorSet ShadowMapArraySet[MAX_DRAW_FRAMES];
	};

	struct DescriptorSetHandles
	{
		BmRender_DescriptorSet FrameBufferSet;
	};

	struct RenderState
	{
		BmRender_CommandWorker GraphicsCommandWorker;
		StaticMeshPipelineDepr MeshPipeline;
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
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <ShortTypes.h>

#include <RenderInterface.h>

#include "Util/EngineTypes.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

struct GLFWwindow;

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

	std::vector<DrawEntity> DrawEntities;
};

void Render_Init(GLFWwindow* WindowHandle);
void Render_DeInit();

void Render_Draw(DrawScene* Data, u64 WaitSemaphoreValue);

RenderState* GetRenderState();

BmRender_ImageView* TestDeferredInputColorImageInterface();
BmRender_ImageView* TestDeferredInputDepthImageInterface();
BmRender_Image* TestDeferredInputColorImage();
BmRender_Image* TestDeferredInputDepthImage();

DescriptorSetHandles* GetHandles();
BmRender_GPUBuffer* GetVertexBuffer();
BmRender_GPUBuffer* GetIndexBuffer();
BmRender_GPUBuffer* GetInstanceBuffer();
BmRender_GPUBuffer* GetMaterialBuffer();

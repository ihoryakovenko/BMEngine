#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <ShortTypes.h>

#include <RenderInterface.h>

#include "Util/EngineTypes.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include <Engine/Systems/Render/Shaders/ShaderTypes.h>

struct GLFWwindow;

struct StaticMeshPipelineDepr
{
	BmRender_ImageView ShadowMapArrayImageInterface[MAX_DRAW_FRAMES];
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

	bool MeshletDebugView = false;
	glm::mat4 CullLightSpaceMatrices[MAX_SHADOW_TEXTURES];
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
BmRender_GPUBuffer* GetInstanceBuffer();
BmRender_GPUBuffer* GetMaterialBuffer();
BmRender_GPUBuffer* GetMeshletBuffer();
BmRender_GPUBuffer* GetMeshletVertexBuffer();
BmRender_GPUBuffer* GetMeshletTriangleBuffer();

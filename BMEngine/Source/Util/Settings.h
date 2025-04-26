#pragma once

#include "Render/Render.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct SkyBoxVertex
{
	glm::vec3 Position;
};

struct QuadSphereVertex
{
	glm::vec3 Position;
	glm::vec2 TextureCoords;
};

extern VulkanInterface::VertexInput SkyBoxVertexInput;
extern VulkanInterface::VertexInput DepthVertexInput;
extern VulkanInterface::VertexInput QuadSphereVertexInput;

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

extern VulkanInterface::PipelineSettings DeferredPipelineSettings;
extern VulkanInterface::PipelineSettings SkyBoxPipelineSettings;
extern VulkanInterface::PipelineSettings DepthPipelineSettings;

static const u32 MainPathPipelinesCount = 1;

extern VulkanInterface::RenderPassSettings MainRenderPassSettings;

extern VkClearValue MainPassClearValues[3];

extern VkImageCreateInfo ShadowMapArrayCreateInfo;

extern VulkanInterface::UniformImageInterfaceCreateInfo DeferredInputDepthUniformInterfaceCreateInfo;
extern VulkanInterface::UniformImageInterfaceCreateInfo DeferredInputUniformColorInterfaceCreateInfo;
extern VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement1InterfaceCreateInfo;
extern VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement2InterfaceCreateInfo;

extern VkBufferCreateInfo VpBufferInfo;

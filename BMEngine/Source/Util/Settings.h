#pragma once

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/VulkanHelper.h"

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

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

extern VulkanHelper::PipelineSettings DeferredPipelineSettings;
extern VulkanHelper::PipelineSettings SkyBoxPipelineSettings;
extern VulkanHelper::PipelineSettings DepthPipelineSettings;
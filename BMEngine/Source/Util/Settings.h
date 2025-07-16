#pragma once

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/VulkanHelper.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct SkyBoxVertex
{
	glm::vec3 Position;
};

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

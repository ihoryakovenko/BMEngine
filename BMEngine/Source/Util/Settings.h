#pragma once

#include "Engine/Systems/Render/Render.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct SkyBoxVertex
{
	glm::vec3 Position;
};

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern BmRender_Format ColorFormat;
extern BmRender_Format DepthFormat;

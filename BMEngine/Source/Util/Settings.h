#pragma once

#include "Engine/Systems/Render/Render.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct SkyBoxVertex
{
	glm::vec3 Position;
};

extern BmRender_Dimensions MainScreenExtent;
extern BmRender_Dimensions DepthViewportExtent;

extern BmRender_Format ColorFormat;
extern BmRender_Format DepthFormat;

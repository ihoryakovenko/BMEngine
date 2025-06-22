#pragma once

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/Render.h"

namespace LightningPass
{
	void Init();
	void DeInit();

	void Draw(const Render::DrawScene* Scene);

	VulkanInterface::UniformImage* GetShadowMapArray();
}
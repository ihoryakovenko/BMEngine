#pragma once

#include "Render/VulkanInterface/VulkanInterface.h"

namespace LightningPass
{
	void Init();
	void DeInit();

	void Draw();

	VulkanInterface::UniformImage* GetShadowMapArray();
}
#pragma once

namespace LightningPass
{
	void Init();
	void DeInit();

	void Draw();

	VulkanInterface::UniformImage* GetShadowMapArray();
}
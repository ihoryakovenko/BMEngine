#pragma once

#include "Util/EngineTypes.h"

#include <vulkan/vulkan.h>

namespace TerrainRender
{
	void Init();
	void DeInit();

	void Draw();

	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach);
}
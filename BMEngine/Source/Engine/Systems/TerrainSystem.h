#pragma once

#include "Util/EngineTypes.h"

#include <vulkan/vulkan.h>

namespace TerrainSystem
{
	void Init();
	void DeInit();

	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach);
}
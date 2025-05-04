#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "Util/EngineTypes.h"

#include "Render/Render.h"

namespace ResourceManager
{
	extern u64 DefaultTextureHash;
	extern const u8 DefaultTextureData[];
	extern u64 DefaultTextureDataCount;

	void Init();
}
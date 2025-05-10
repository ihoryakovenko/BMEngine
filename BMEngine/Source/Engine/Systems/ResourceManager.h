#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "Util/EngineTypes.h"

#include "Render/Render.h"

namespace ResourceManager
{
	void Init();
	void LoadModel(const char* FilePath);
	void LoadTextures(const char* Directory);
}
#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "Util/EngineTypes.h"

#include "Engine/Systems/Render/Render.h"

namespace ResourceManager
{
	void Init();
	void LoadModel(const char* FilePath, const char* Directory);
	void LoadTextures(const char* Directory);
}
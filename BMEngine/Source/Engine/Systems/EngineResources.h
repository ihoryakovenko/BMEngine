#pragma once

#include <mini-yaml/yaml/Yaml.hpp>
#include <unordered_map>
#include <string>
#include "Util/EngineTypes.h"

namespace EngineResources
{
	struct TextureAsset
	{
		std::string Path;
		u32 RenderTextureIndex;
		bool IsRenderResourceCreated;
	};

	void Init(Yaml::Node& Root);
	void DeInit();
}
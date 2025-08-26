#pragma once

#include <mini-yaml/yaml/Yaml.hpp>
#include <unordered_map>
#include <string>
#include "Util/EngineTypes.h"

namespace Render
{
	struct DrawScene;
}

namespace EngineResources
{
	struct TextureAsset
	{
		u32 RenderTextureIndex;
	};

	void Init(Yaml::Node& Root, struct Render::DrawScene* TmpScene);
	void DeInit();
}
#pragma once

#include <mini-yaml/yaml/Yaml.hpp>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>
#include "Util/EngineTypes.h"

namespace Render
{
	struct DrawScene;
}

namespace EngineResources
{
	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct TextureAsset
	{
		u32 RenderTextureIndex;
	};

	void Init(Yaml::Node& Root, struct Render::DrawScene* TmpScene);
	void DeInit();
}
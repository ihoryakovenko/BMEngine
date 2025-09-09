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
		std::string TexturePath;
		u32 RenderTextureIndex;
		bool IsCreated;
	};

	struct ModelLoadRequest
	{
		glm::vec3 Position;
		std::string Path;
	};

	void Init();
	void DeInit();

	void Update(Render::DrawScene* TmpScene);

	void RegisterTextureAsset(const std::string& Name, const std::string& Path);
	void RequestModelLoad(const ModelLoadRequest& Request);
}
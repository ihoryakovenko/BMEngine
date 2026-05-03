#pragma once

#include <mini-yaml/yaml/Yaml.hpp>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>
#include "Util/EngineTypes.h"

#include <RenderInterface.h>

namespace Render
{
	struct DrawScene;
}

namespace EngineResources
{
	struct TextureAsset
	{
		std::string TexturePath;
		BmRender_Image RenderImageHandle;
		BmRender_ImageView RenderViewHandle;
		u32 TextureGPUIndex;
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
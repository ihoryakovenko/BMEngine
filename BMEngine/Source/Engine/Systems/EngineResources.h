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
	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct Material
	{
		u32 AlbedoTexIndex;
		u32 SpecularTexIndex;
		f32 Shininess;
	};

	struct InstanceData
	{
		glm::mat4 ModelMatrix;
		u32 MaterialIndex;
	};

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
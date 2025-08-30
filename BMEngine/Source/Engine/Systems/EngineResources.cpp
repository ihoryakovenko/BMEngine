#include "EngineResources.h"

#include "Util/Util.h"
#include "Engine/Systems/Render/Render.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;

	void Init(Yaml::Node& SceneResourcesNode, Render::DrawScene* TmpScene)
	{
		const u64 DefaultTextureDataCount = sizeof(DefaultTextureData) / sizeof(DefaultTextureData[0]);
		gli::texture DefaultTexture = gli::load((char const*)DefaultTextureData, DefaultTextureDataCount);
		if (DefaultTexture.empty())
		{
			assert(false);
		}
		const u64 DefaultAssetId = std::hash<std::string>{ }("Default");
		const glm::tvec3<u32> DefaultAssetExtent = DefaultTexture.extent();

		TextureAsset DefaultAsset;
		DefaultAsset.RenderTextureIndex = RenderResources::CreateTexture2DSRGB(DefaultAssetId, DefaultTexture.data(), DefaultAssetExtent.x, DefaultAssetExtent.y);

		TextureAssets[DefaultAssetId] = DefaultAsset;
		
		Yaml::Node& TexturesNode = Util::GetTextures(SceneResourcesNode);
		for (auto It = TexturesNode.Begin(); It != TexturesNode.End(); It++)
		{
			const std::string& TextureName = (*It).first;
			const std::string& TexturePath = (*It).second.As<std::string>();

			gli::texture Texture = gli::load(TexturePath);
			if (Texture.empty())
			{
				assert(false);
			}

			const glm::tvec3<u32> Extent = Texture.extent();
			const u64 Id = std::hash<std::string>{ }(TextureName);
			
			TextureAsset Asset;
			Asset.RenderTextureIndex = RenderResources::CreateTexture2DSRGB(Id, Texture.data(), Extent.x, Extent.y);
			
			TextureAssets[Id] = Asset;
		}

		Yaml::Node& ModelsNode = Util::GetModels(SceneResourcesNode);
		for (auto It = ModelsNode.Begin(); It != ModelsNode.End(); It++)
		{
			std::string ModelName = (*It).first;
			std::string ModelPath = (*It).second.As<std::string>();
			
			Util::Model3DData ModelData = Util::LoadModel3DData(ModelPath.c_str());
			Util::Model3D Model = Util::ParseModel3D(ModelData);

			u64 ModelVertexByteOffset = 0;
			for (u32 i = 0; i < Model.MeshCount; i++)
			{
				const u64 VerticesCount = Model.VerticesCounts[i];
				const u32 IndicesCount = Model.IndicesCounts[i];

				u32 TextureIndex = 0;

				auto it = TextureAssets.find(Model.DiffuseTexturesHashes[i]);
				if (it != TextureAssets.end())
				{
					TextureIndex = it->second.RenderTextureIndex;
				}

				Render::Material Mat;
				Mat.AlbedoTexIndex = TextureIndex;
				Mat.SpecularTexIndex = TextureIndex;
				Mat.Shininess = 32.0f;

				Render::InstanceData Instance;
				Instance.MaterialIndex = RenderResources::CreateMaterial(&Mat);
				Instance.ModelMatrix = glm::mat4(1.0f);

				Render::DrawEntity Entity = { };
				Entity.StaticMeshIndex = RenderResources::CreateStaticMesh(Model.VertexData + ModelVertexByteOffset, sizeof(Render::StaticMeshVertex), VerticesCount, IndicesCount);
				Entity.Instances = 1;
				Entity.InstanceDataIndex = RenderResources::CreateStaticMeshInstance(&Instance);

				Memory::PushBackToArray(&TmpScene->DrawEntities, &Entity);

				ModelVertexByteOffset += VerticesCount * sizeof(Render::StaticMeshVertex) + IndicesCount * sizeof(u32);
			}

			Util::ClearModel3DData(ModelData);
		}
	}

	void DeInit()
	{
		TextureAssets.clear();
	}
}
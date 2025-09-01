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

		RenderResources::TextureDescription DefaultTextureDescription;
		DefaultTextureDescription.Data = DefaultTexture.data();
		DefaultTextureDescription.Width = DefaultAssetExtent.x;
		DefaultTextureDescription.Height = DefaultAssetExtent.y;

		TextureAsset DefaultAsset;
		DefaultAsset.RenderTextureIndex = RenderResources::CreateTexture2DSRGB(&DefaultTextureDescription);

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

			RenderResources::TextureDescription TextureDescription;
			TextureDescription.Data = Texture.data();
			TextureDescription.Width = Extent.x;
			TextureDescription.Height = Extent.y;
			
			TextureAsset Asset;
			Asset.RenderTextureIndex = RenderResources::CreateTexture2DSRGB(&TextureDescription);
			
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
			for (u32 i = 0; i < Model.Header.MeshCount; i++)
			{
				const u64 VerticesCount = Model.VerticesCounts[i];
				const u32 IndicesCount = Model.IndicesCounts[i];

				u32 AlbedoTextureIndex = 0;
				u32 SpecularTextureIndex = 0;

				if (Model.Header.MaterialCount > 0)
				{
					const u32 MaterialIndex = Model.MaterialIndices[i];
					const Util::Model3DMaterial& material = Model.Materials[MaterialIndex];

					auto it = TextureAssets.find(material.DiffuseTextureHash);
					if (it != TextureAssets.end())
					{
						AlbedoTextureIndex = it->second.RenderTextureIndex;
						SpecularTextureIndex = AlbedoTextureIndex;
					}

					it = TextureAssets.find(material.SpecularTextureHash);
					if (it != TextureAssets.end())
					{
						SpecularTextureIndex = it->second.RenderTextureIndex;
					}
				}

				RenderResources::Material Mat;
				Mat.AlbedoTexIndex = AlbedoTextureIndex;
				Mat.SpecularTexIndex = SpecularTextureIndex;
				Mat.Shininess = 32.0f;

				RenderResources::InstanceData Instance;
				Instance.MaterialIndex = RenderResources::CreateMaterial(&Mat);
				Instance.ModelMatrix = glm::mat4(1.0f);

				RenderResources::MeshDescription Mesh;
				Mesh.IndicesCount = IndicesCount;
				Mesh.MeshVertexData = Model.VertexData + ModelVertexByteOffset;
				Mesh.VertexSize = sizeof(StaticMeshVertex);
				Mesh.VerticesCount = VerticesCount;

				Render::DrawEntity Entity = { };
				Entity.StaticMeshIndex = RenderResources::CreateStaticMesh(&Mesh);
				Entity.Instances = 1;
				Entity.InstanceDataIndex = RenderResources::CreateStaticMeshInstance(&Instance);

				Memory::PushBackToArray(&TmpScene->DrawEntities, &Entity);

				ModelVertexByteOffset += VerticesCount * sizeof(StaticMeshVertex) + IndicesCount * sizeof(u32);
			}

			Util::ClearModel3DData(ModelData);
		}
	}

	void DeInit()
	{
		TextureAssets.clear();
	}
}
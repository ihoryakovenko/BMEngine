#include "EngineResources.h"

#include "Util/Util.h"
#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/TransferSystem.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;

	static u32 CreateTexture(const std::string& Path)
	{
		gli::texture Texture = gli::load(Path);
		if (Texture.empty())
		{
			assert(false);
		}

		const glm::tvec3<u32> Extent = Texture.extent();

		RenderResources::TextureDescription TextureDescription;
		TextureDescription.Width = Extent.x;
		TextureDescription.Height = Extent.y;

		return RenderResources::CreateTexture2DSRGB(&TextureDescription, Texture.data());
	}

	void Init(Yaml::Node& SceneResourcesNode, Render::DrawScene* TmpScene)
	{
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
			DefaultTextureDescription.Width = DefaultAssetExtent.x;
			DefaultTextureDescription.Height = DefaultAssetExtent.y;

			TextureAsset DefaultAsset;
			DefaultAsset.RenderTextureIndex = RenderResources::CreateTexture2DSRGB(&DefaultTextureDescription, DefaultTexture.data());
			DefaultAsset.IsCreated = true;

			TextureAssets[DefaultAssetId] = DefaultAsset;
		}
		
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
			
			TextureAsset Asset;
			Asset.TexturePath = TexturePath;
			Asset.RenderTextureIndex = 0;
			Asset.IsCreated = false;
			
			TextureAssets[std::hash<std::string>{ }(TextureName)] = Asset;
		}

		glm::mat4 ModelMatrix = glm::mat4(1);

		for (u32 i = 0; i < 50; ++i)
		{
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
							if (!it->second.IsCreated)
							{
								it->second.RenderTextureIndex = CreateTexture(it->second.TexturePath);
								it->second.IsCreated = true;
							}

							AlbedoTextureIndex = it->second.RenderTextureIndex;
							SpecularTextureIndex = AlbedoTextureIndex;
						}

						it = TextureAssets.find(material.SpecularTextureHash);
						if (it != TextureAssets.end())
						{
							if (!it->second.IsCreated)
							{
								it->second.RenderTextureIndex = CreateTexture(it->second.TexturePath);
								it->second.IsCreated = true;
							}

							SpecularTextureIndex = it->second.RenderTextureIndex;
						}
					}

					const u64 VertexDataSize = VerticesCount * sizeof(StaticMeshVertex) + IndicesCount * sizeof(u32);

					RenderResources::Material Mat;
					Mat.AlbedoTexIndex = AlbedoTextureIndex;
					Mat.SpecularTexIndex = SpecularTextureIndex;
					Mat.Shininess = 32.0f;

					RenderResources::InstanceData Instance;
					Instance.MaterialIndex = RenderResources::CreateMaterial(&Mat);
					Instance.ModelMatrix = ModelMatrix;

					RenderResources::MeshDescription Mesh;
					Mesh.IndicesCount = IndicesCount;
					Mesh.VertexSize = sizeof(StaticMeshVertex);
					Mesh.VerticesCount = VerticesCount;

					Render::DrawEntity Entity = { };
					Entity.StaticMeshIndex = RenderResources::CreateStaticMesh(&Mesh, Model.VertexData + ModelVertexByteOffset);
					Entity.Instances = 1;
					Entity.InstanceDataIndex = RenderResources::CreateStaticMeshInstance(&Instance);

					std::unique_lock Lock(TmpScene->TempLock);
					Memory::PushBackToArray(&TmpScene->DrawEntities, &Entity);
					Lock.unlock();

					ModelVertexByteOffset += VertexDataSize;
					//std::this_thread::sleep_for(std::chrono::microseconds(2));
				}

				Util::ClearModel3DData(ModelData);
			}

			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, -1, 0));
		}
	}

	void DeInit()
	{
		TextureAssets.clear();
	}
}
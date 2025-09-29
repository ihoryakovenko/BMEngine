#include "EngineResources.h"

#include "Util/Util.h"
#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/TransferSystem.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;
	static std::queue<ModelLoadRequest> ModelLoadRequests;
	static std::mutex ModelLoadMutex;

	static RenderResources::ResourceHandle CreateTexture(const std::string& Path)
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
		TextureDescription.Format = Util::GliFormatToVkFormat(Texture.format());

		return RenderResources::CreateTexture(&TextureDescription, Texture.data());
	}

	void Init()
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
		DefaultTextureDescription.Format = Util::GliFormatToVkFormat(DefaultTexture.format());

		TextureAsset DefaultAsset;
		DefaultAsset.RenderTextureIndex = RenderResources::CreateTexture(&DefaultTextureDescription, DefaultTexture.data());
		DefaultAsset.IsCreated = true;

		TextureAssets[DefaultAssetId] = DefaultAsset;
	}

	void DeInit()
	{
		TextureAssets.clear();

		std::lock_guard Lock(ModelLoadMutex);
		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequests.pop();
		}
	}

	void Update(Render::DrawScene* TmpScene)
	{
		std::lock_guard Lock(ModelLoadMutex);
		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequest Request = ModelLoadRequests.front();
			ModelLoadRequests.pop();

			Util::Model3DData ModelData = Util::LoadModel3DData(Request.Path.c_str());
			Util::Model3D Model = Util::ParseModel3D(ModelData);

			u64 ModelVertexByteOffset = 0;
			for (u32 i = 0; i < Model.Header.MeshCount; i++)
			{
				const u64 VerticesCount = Model.VerticesCounts[i];
				const u32 IndicesCount = Model.IndicesCounts[i];

				u32 AlbedoTextureHandle = 0;
				u32 SpecularTextureHandle = 0;

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

						AlbedoTextureHandle = it->second.RenderTextureIndex;
						SpecularTextureHandle = AlbedoTextureHandle;
					}

					it = TextureAssets.find(material.SpecularTextureHash);
					if (it != TextureAssets.end())
					{
						if (!it->second.IsCreated)
						{
							it->second.RenderTextureIndex = CreateTexture(it->second.TexturePath);
							it->second.IsCreated = true;
						}

						SpecularTextureHandle = it->second.RenderTextureIndex;
					}
				}

				const u64 VertexDataSize = VerticesCount * sizeof(StaticMeshVertex) + IndicesCount * sizeof(u32);

				RenderResources::MeshDescription Mesh;
				Mesh.IndicesCount = IndicesCount;
				Mesh.VertexSize = sizeof(StaticMeshVertex);
				Mesh.VerticesCount = VerticesCount;

				RenderResources::ResourceHandle MeshHandle = RenderResources::CreateStaticMesh(&Mesh, Model.VertexData + ModelVertexByteOffset, "VertexStageData");

				Material Mat;
				Mat.AlbedoTexIndex = RenderResources::GetResourceGPUIndex(AlbedoTextureHandle);
				Mat.SpecularTexIndex = RenderResources::GetResourceGPUIndex(SpecularTextureHandle);
				Mat.Shininess = 32.0f;

				RenderResources::ResourceDependency TextureHandles[] = { AlbedoTextureHandle, SpecularTextureHandle };
				RenderResources::ResourceDependency TextureDep = RenderResources::CreateResourceDependency(TextureHandles, 2);

				const RenderResources::ResourceHandle MaterialHandle = RenderResources::CreateStorageBufferResource(sizeof(Mat), "MaterialBuffer", TextureDep, 2);
				RenderResources::UpdateStorageBufferResource(MaterialHandle, &Mat, sizeof(Mat), "MaterialBuffer");

				InstanceData Instance;
				Instance.MaterialIndex = RenderResources::GetResourceGPUIndex(MaterialHandle);
				Instance.ModelMatrix = glm::translate(glm::mat4(1), Request.Position);

				RenderResources::ResourceDependency InstantDepHandles[] = { MeshHandle, MaterialHandle };
				RenderResources::ResourceDependency InstanceDep = RenderResources::CreateResourceDependency(InstantDepHandles, 2);

				const RenderResources::ResourceHandle InstanceHandle = RenderResources::CreateStorageBufferResource(sizeof(Instance), "GPUInstances", InstanceDep, 2);
				RenderResources::UpdateStorageBufferResource(InstanceHandle, &Instance, sizeof(Instance), "GPUInstances");

				Render::DrawEntity Entity = { };
				Entity.StaticMeshHandle = MeshHandle;
				Entity.Instances = 1;
				Entity.InstanceDataHandle = InstanceHandle;

				std::unique_lock Lock(TmpScene->TempLock);
				Memory::PushBackToArray(&TmpScene->DrawEntities, &Entity);
				Lock.unlock();

				ModelVertexByteOffset += VertexDataSize;
			}

			Util::ClearModel3DData(ModelData);
		}
	}

	void RegisterTextureAsset(const std::string& Name, const std::string& Path)
	{
		TextureAsset Asset;
		Asset.TexturePath = Path;
		Asset.RenderTextureIndex = 0;
		Asset.IsCreated = false;

		TextureAssets[std::hash<std::string>{ }(Name)] = Asset;
	}

	void RequestModelLoad(const ModelLoadRequest& Request)
	{
		std::lock_guard Lock(ModelLoadMutex);
		ModelLoadRequests.push(Request);
	}
}
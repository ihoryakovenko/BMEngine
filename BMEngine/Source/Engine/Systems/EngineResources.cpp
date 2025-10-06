#include "EngineResources.h"

#include "Util/Util.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;
	static std::queue<ModelLoadRequest> ModelLoadRequests;
	static std::mutex ModelLoadMutex;

	static void CreateTexture(TextureAsset& Asset)
	{
		gli::texture Texture = gli::load(Asset.TexturePath);
		if (Texture.empty())
		{
			assert(false);
		}

		const glm::tvec3<u32> Extent = Texture.extent();

		BmRender_ImageDescription TextureDescription;
		TextureDescription.Width = Extent.x;
		TextureDescription.Height = Extent.y;
		TextureDescription.Format = Util::GliFormatToVkFormat(Texture.format());
		TextureDescription.ArrayLayers = 1;
		TextureDescription.Type = ImageType::TransferSampled;

		Asset.RenderImageHandle =  RenderResources::CreateImageResource(&TextureDescription);
		RenderResources::UpdateImageResource(Asset.RenderImageHandle, &TextureDescription, Texture.data());
		Asset.RenderViewHandle = RenderResources::CreateImageView(Asset.RenderImageHandle, TextureDescription.Format);
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

		BmRender_ImageDescription DefaultTextureDescription;
		DefaultTextureDescription.Width = DefaultAssetExtent.x;
		DefaultTextureDescription.Height = DefaultAssetExtent.y;
		DefaultTextureDescription.Format = Util::GliFormatToVkFormat(DefaultTexture.format());
		DefaultTextureDescription.ArrayLayers = 1;
		DefaultTextureDescription.Type = ImageType::TransferSampled;

		TextureAsset DefaultAsset;
		DefaultAsset.RenderImageHandle = RenderResources::CreateImageResource(&DefaultTextureDescription);
		DefaultAsset.IsCreated = true;

		RenderResources::UpdateImageResource(DefaultAsset.RenderImageHandle, &DefaultTextureDescription, DefaultTexture.data());
		DefaultAsset.RenderViewHandle = RenderResources::CreateImageView(DefaultAsset.RenderImageHandle, DefaultTextureDescription.Format);

		BmRender_ImageViewBindingDescription DiffuseDescription;
		DiffuseDescription.Sampler = "DiffuseTexture";
		DiffuseDescription.ArrayElement = 0;
		DiffuseDescription.BindingIndex = 0;

		BmRender_ImageViewBindingDescription SpecularDescription;
		SpecularDescription.Sampler = "SpecularTexture";
		SpecularDescription.ArrayElement = 0;
		SpecularDescription.BindingIndex = 1;

		BmRender_ImageViewBindingDescription Descriptions[] = { DiffuseDescription, SpecularDescription };

		RenderResources::BindImageView(DefaultAsset.RenderViewHandle, "BindlesTexturesSet", Descriptions, 2);

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

		u32 TexturesGPUIndexCounter = 1;

		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequest Request = ModelLoadRequests.front();
			ModelLoadRequests.pop();

			Util::Model3DData ModelData = Util::LoadModel3DData(Request.Path.c_str());
			Util::Model3D Model = Util::ParseModel3D(ModelData);

			u32 MateriaIndex = 0;
			u32 InstanceIndex = 0;
			u64 ModelVertexByteOffset = 0;
			for (u32 i = 0; i < Model.Header.MeshCount; i++)
			{
				const u64 VerticesCount = Model.VerticesCounts[i];
				const u32 IndicesCount = Model.IndicesCounts[i];

				BmRender_ImageResource AlbedoTextureHandle = 0;
				BmRender_ImageResource SpecularTextureHandle = 0;
				u32 TextureGPUIndex = 0;

				if (Model.Header.MaterialCount > 0)
				{
					const u32 MaterialIndex = Model.MaterialIndices[i];
					const Util::Model3DMaterial& material = Model.Materials[MaterialIndex];

					auto it = TextureAssets.find(material.DiffuseTextureHash);
					if (it != TextureAssets.end())
					{
						if (!it->second.IsCreated)
						{
							TextureGPUIndex = TexturesGPUIndexCounter;

							CreateTexture(it->second);
							it->second.IsCreated = true;
							it->second.TextureGPUIndex = TextureGPUIndex;

							AlbedoTextureHandle = it->second.RenderImageHandle;
							SpecularTextureHandle = AlbedoTextureHandle;

							BmRender_ImageViewBindingDescription DiffuseDescription;
							DiffuseDescription.Sampler = "DiffuseTexture";
							DiffuseDescription.ArrayElement = TexturesGPUIndexCounter;
							DiffuseDescription.BindingIndex = 0;

							BmRender_ImageViewBindingDescription SpecularDescription;
							SpecularDescription.Sampler = "SpecularTexture";
							SpecularDescription.ArrayElement = TexturesGPUIndexCounter;
							SpecularDescription.BindingIndex = 1;

							BmRender_ImageViewBindingDescription Descriptions[] = { DiffuseDescription, SpecularDescription };

							RenderResources::BindImageView(it->second.RenderViewHandle, "BindlesTexturesSet", Descriptions, 2);

							++TexturesGPUIndexCounter;
						}
						else
						{
							TextureGPUIndex = it->second.TextureGPUIndex;
						}
					}

					//it = TextureAssets.find(material.SpecularTextureHash);
					//if (it != TextureAssets.end())
					//{
					//	if (!it->second.IsCreated)
					//	{
					//		CreateTexture(it->second);
					//		++TextureAssetsIndex;
					//		it->second.IsCreated = true;
					//	}

					//	SpecularTextureHandle = it->second.RenderImageHandle;
					//}
				}

				const u64 VertexDataSize = VerticesCount * sizeof(StaticMeshVertex) + IndicesCount * sizeof(u32);
				
				BmRender_BufferRegion MeshHandle = RenderResources::CreateBufferRegion(ModelVertexByteOffset, "VertexStageData");
				RenderResources::UpdateBufferRegion(MeshHandle, 0, Model.VertexData + ModelVertexByteOffset, VertexDataSize);

				Material Mat;
				Mat.AlbedoTexIndex = TextureGPUIndex;
				Mat.SpecularTexIndex = TextureGPUIndex;
				Mat.Shininess = 32.0f;

				const BmRender_BufferRegion MaterialHandle = RenderResources::CreateBufferRegion(MateriaIndex * sizeof(Mat), "MaterialBuffer");
				RenderResources::UpdateBufferRegion(MaterialHandle, 0, &Mat, sizeof(Mat));

				InstanceData Instance;
				Instance.MaterialIndex = MateriaIndex;
				Instance.ModelMatrix = glm::translate(glm::mat4(1), Request.Position);

				++MateriaIndex;

				const u64 InstanceOffset = InstanceIndex * sizeof(Instance);
				const BmRender_BufferRegion InstanceHandle = RenderResources::CreateBufferRegion(InstanceOffset, "GPUInstances");
				RenderResources::UpdateBufferRegion(InstanceHandle, 0, &Instance, sizeof(Instance));

				++InstanceIndex;

				const u64 VerticesSize = sizeof(StaticMeshVertex) * VerticesCount;

				Render::DrawEntity Entity = { };
				Entity.VertexOffset = ModelVertexByteOffset;
				Entity.IndexOffset = ModelVertexByteOffset + VerticesSize;
				Entity.IndicesCount = IndicesCount;
				Entity.VertexDataSize = VertexDataSize;
				Entity.StaticMeshHandle = MeshHandle;
				Entity.Instances = 1;
				Entity.InstanceOffset = InstanceOffset;
				Entity.ImageDependency.push_back(AlbedoTextureHandle);
				Entity.ImageDependency.push_back(SpecularTextureHandle);
				Entity.ResourceDependency.push_back(MaterialHandle);
				Entity.ResourceDependency.push_back(InstanceHandle);

				std::unique_lock Lock(TmpScene->TempLock);
				TmpScene->DrawEntities.push_back(Entity);
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
		Asset.RenderImageHandle = 0;
		Asset.IsCreated = false;

		TextureAssets[std::hash<std::string>{ }(Name)] = Asset;
	}

	void RequestModelLoad(const ModelLoadRequest& Request)
	{
		std::lock_guard Lock(ModelLoadMutex);
		ModelLoadRequests.push(Request);
	}
}
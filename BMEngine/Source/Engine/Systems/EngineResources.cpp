#include "EngineResources.h"

#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Extern declarations for global resource maps
extern std::unordered_map<std::string, BmRender_Sampler> Samplers;

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;
	static std::queue<ModelLoadRequest> ModelLoadRequests;
	static std::mutex ModelLoadMutex;
	static TextureAsset DefaultAsset;
	
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
		TextureDescription.Type = BmRender_ImageType::TransferSampled;
		TextureDescription.SampleCount = BmRender_SampleCount::Count1;

		BmRender_Image ImageHandle = BmRender_CreateImage2D(Extent.x, Extent.y, Util::GliFormatToVkFormat(Texture.format()), BmRender_ImageType::TransferSampled, BmRender_SampleCount::Count1);
		Asset.RenderImageHandle = ImageHandle;
		RenderResources::UpdateImageResource(Asset.RenderImageHandle, &TextureDescription, Texture.data());
		BmRender_ImageView ViewHandle = BmRender_CreateImageView2D(Asset.RenderImageHandle);
		Asset.RenderViewHandle = ViewHandle;
	}

	void Init()
	{
		// Store buffer handles
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
		DefaultTextureDescription.Type = BmRender_ImageType::TransferSampled;
		DefaultTextureDescription.SampleCount = BmRender_SampleCount::Count1;

		BmRender_Image DefaultImageHandle = BmRender_CreateImage2D(DefaultAssetExtent.x, DefaultAssetExtent.y, Util::GliFormatToVkFormat(DefaultTexture.format()), BmRender_ImageType::TransferSampled, BmRender_SampleCount::Count1);
		DefaultAsset.RenderImageHandle = DefaultImageHandle;
		DefaultAsset.IsCreated = true;

		RenderResources::UpdateImageResource(DefaultAsset.RenderImageHandle, &DefaultTextureDescription, DefaultTexture.data());
		BmRender_ImageView DefaultViewHandle = BmRender_CreateImageView2D(DefaultAsset.RenderImageHandle);
		DefaultAsset.RenderViewHandle = DefaultViewHandle;

		BmRender_DescriptorSetUpdateData DiffuseBinding;
		DiffuseBinding.ImageBinding.Sampler = Samplers["DiffuseTexture"];
		DiffuseBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
		DiffuseBinding.ImageBinding.ImageView = DefaultAsset.RenderViewHandle;
		DiffuseBinding.DstArrayElement = 0;
		DiffuseBinding.BindingCount = 1;
		DiffuseBinding.DstBinding = 3;

		BmRender_DescriptorSetUpdateData Bindings[] = { DiffuseBinding };

		BmRender_UpdateDescriptorSet(GetHandles()->FrameBufferSet, Bindings, 1);
	}

	void DeInit()
	{
		for (auto& [id, asset] : TextureAssets)
		{
			if (asset.IsCreated)
			{
				BmRender_DestroyImageView(asset.RenderViewHandle);
				BmRender_DestroyImage(asset.RenderImageHandle);
			}
		}

		if (DefaultAsset.IsCreated)
		{
			BmRender_DestroyImageView(DefaultAsset.RenderViewHandle);
			BmRender_DestroyImage(DefaultAsset.RenderImageHandle);
		}

		TextureAssets.clear();

		std::lock_guard Lock(ModelLoadMutex);
		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequests.pop();
		}
	}

	void Update(DrawScene* TmpScene)
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
			u64 VertexBufferOffset = 0;
			u64 IndexBufferOffset = 0;

			for (u32 i = 0; i < Model.Header.MeshCount; i++)
			{
				const u64 VerticesCount = Model.VerticesCounts[i];
				const u32 IndicesCount = Model.IndicesCounts[i];

				BmRender_Image AlbedoTextureHandle = DefaultAsset.RenderImageHandle;
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

							BmRender_DescriptorSetUpdateData DiffuseBinding;
							DiffuseBinding.ImageBinding.Sampler = Samplers["DiffuseTexture"];
							DiffuseBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
							DiffuseBinding.ImageBinding.ImageView = it->second.RenderViewHandle;
							DiffuseBinding.DstArrayElement = TexturesGPUIndexCounter;
							DiffuseBinding.BindingCount = 1;
							DiffuseBinding.DstBinding = 3;

							BmRender_DescriptorSetUpdateData Bindings[] = { DiffuseBinding };

							BmRender_UpdateDescriptorSet(GetHandles()->FrameBufferSet, Bindings, 1);

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

				const u64 VertexDataSize = VerticesCount * sizeof(Shader_StaticMeshVertex);
				const u64 VerticesSize = sizeof(Shader_StaticMeshVertex) * VerticesCount;
				const u64 IndicesSize = IndicesCount * sizeof(u32);
					
				BmRender_GPUBufferUpdateData VertexHandle = { GetVertexBuffer(), VertexBufferOffset, VertexDataSize };
				BmRender_GPUBufferUpdateData IndexHandle = { GetIndexBuffer(), IndexBufferOffset, IndicesSize };

				RenderResources::UpdateBufferRegion(VertexHandle, 0, Model.VertexData + ModelVertexByteOffset, VertexDataSize);
				RenderResources::UpdateBufferRegion(IndexHandle, 0, Model.VertexData + ModelVertexByteOffset + VertexDataSize, IndicesSize);

				Shader_Material Mat;
				Mat.AlbedoTexIndex = TextureGPUIndex;
				Mat.SpecularTexIndex = TextureGPUIndex;
				Mat.Shininess = 32.0f;

				const BmRender_GPUBufferUpdateData MaterialHandle = { GetMaterialBuffer(), MateriaIndex * sizeof(Mat), sizeof(Mat) };
				RenderResources::UpdateBufferRegion(MaterialHandle, 0, &Mat, sizeof(Mat));

				Shader_StaticMeshInstance Instance;
				Instance.MaterialIndex = MateriaIndex;
				Instance.ModelMatrix = glm::translate(glm::mat4(1), Request.Position);

				++MateriaIndex;

				const u64 InstanceOffset = InstanceIndex * sizeof(Instance);
				const BmRender_GPUBufferUpdateData InstanceHandle = { GetInstanceBuffer(), InstanceOffset, sizeof(Instance) };
				RenderResources::UpdateBufferRegion(InstanceHandle, 0, &Instance, sizeof(Instance));

				++InstanceIndex;

				DrawEntity Entity = { };
				Entity.VertexBufferEntry = VertexHandle;
				Entity.IndexBufferEntry = IndexHandle;
				Entity.InstanceBufferEntry = InstanceHandle;
				Entity.IndicesCount = IndicesCount;
				Entity.Instances = 1;

				std::unique_lock Lock(TmpScene->TempLock);
				TmpScene->DrawEntities.push_back(Entity);
				Lock.unlock();

				VertexBufferOffset += VertexDataSize;
				IndexBufferOffset += IndicesSize;
				ModelVertexByteOffset += VertexDataSize + IndicesSize;
			}

			Util::ClearModel3DData(ModelData);
		}
	}

	void RegisterTextureAsset(const std::string& Name, const std::string& Path)
	{
		TextureAsset Asset;
		Asset.TexturePath = Path;
		Asset.RenderImageHandle;
		Asset.IsCreated = false;

		TextureAssets[std::hash<std::string>{ }(Name)] = Asset;
	}

	void RequestModelLoad(const ModelLoadRequest& Request)
	{
		std::lock_guard Lock(ModelLoadMutex);
		ModelLoadRequests.push(Request);
	}
}
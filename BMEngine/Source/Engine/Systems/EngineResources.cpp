#include "EngineResources.h"

#include "Util/Util.h"
#include "Util/YamlParsing.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern BmRender_Sampler DiffuseTextureSampler;

namespace EngineResources
{
	static std::unordered_map<u64, TextureAsset> TextureAssets;
	static std::queue<ModelLoadRequest> ModelLoadRequests;
	static TextureAsset DefaultAsset;

	static u32 TexturesGPUIndexCounter = 1;
	static u32 MateriaIndex = 0;
	static u32 InstanceIndex = 0;
	static u32 VertexCount = 0;
	static u32 MeshletCount = 0;
	static u32 MeshletVertexCount = 0;
	static u32 MeshletTriangleCount = 0;
	
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
		RenderResources::UpdateImageResource(&Asset.RenderImageHandle, &TextureDescription, Texture.data());
		BmRender_ImageView ViewHandle = BmRender_CreateImageView2D(&Asset.RenderImageHandle);
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

		BmRender_Image DefaultImageHandle = BmRender_CreateImage2D(DefaultAssetExtent.x, DefaultAssetExtent.y, Util::GliFormatToVkFormat(DefaultTexture.format()), BmRender_ImageType::TransferSampled, BmRender_SampleCount::Count1, "Default texture");
		DefaultAsset.RenderImageHandle = DefaultImageHandle;
		DefaultAsset.IsCreated = true;

		RenderResources::UpdateImageResource(&DefaultAsset.RenderImageHandle, &DefaultTextureDescription, DefaultTexture.data());
		BmRender_ImageView DefaultViewHandle = BmRender_CreateImageView2D(&DefaultAsset.RenderImageHandle);
		DefaultAsset.RenderViewHandle = DefaultViewHandle;

		BmRender_DescriptorSetUpdateData DiffuseBinding;
		DiffuseBinding.ImageBinding.Sampler = &DiffuseTextureSampler;
		DiffuseBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
		DiffuseBinding.ImageBinding.ImageView = &DefaultAsset.RenderViewHandle;
		DiffuseBinding.DstArrayElement = 0;
		DiffuseBinding.BindingCount = 1;
		DiffuseBinding.DstBinding = 1;

		BmRender_DescriptorSetUpdateData Bindings[] = { DiffuseBinding };

		BmRender_UpdateDescriptorSet(&GetHandles()->FrameBufferSet, Bindings, 1);
	}

	void DeInit()
	{
		for (auto& [id, asset] : TextureAssets)
		{
			if (asset.IsCreated)
			{
				BmRender_DestroyImageView(asset.RenderViewHandle);
				BmRender_DestroyImage(&asset.RenderImageHandle);
			}
		}

		if (DefaultAsset.IsCreated)
		{
			BmRender_DestroyImageView(DefaultAsset.RenderViewHandle);
			BmRender_DestroyImage(&DefaultAsset.RenderImageHandle);
		}

		TextureAssets.clear();

		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequests.pop();
		}
	}

	void Update(DrawScene* TmpScene)
	{
		while (!ModelLoadRequests.empty())
		{
			ModelLoadRequest Request = ModelLoadRequests.front();
			ModelLoadRequests.pop();

			Util::Model3DData ModelData = Util::LoadModel3DData(Request.Path.c_str());
			Util::Model3D Model = Util::ParseModel3D(ModelData);

			std::vector<Shader_Meshlet> Meshlets(Model.Meshlets, Model.Meshlets + Model.Header.MeshletCount);
			std::vector<u32> MeshletVertices(Model.MeshletVertices, Model.MeshletVertices + Model.Header.MeshletVertexCount);

			u32 ModelMeshletIndex = 0;

			for (u32 i = 0; i < Model.Header.MeshCount; i++)
			{
				BmRender_Image AlbedoTextureHandle = DefaultAsset.RenderImageHandle;
				u32 TextureGPUIndex = 0;

				const u32 MaterialIndex = Model.MaterialIndices[i];
				if (MaterialIndex < Model.Header.MaterialCount)
				{
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
							DiffuseBinding.ImageBinding.Sampler = nullptr;
							DiffuseBinding.ImageBinding.ImageLayout = BmRender_ImageLayout::ShaderReadOnlyOptimal;
							DiffuseBinding.ImageBinding.ImageView = &it->second.RenderViewHandle;
							DiffuseBinding.DstArrayElement = TexturesGPUIndexCounter;
							DiffuseBinding.BindingCount = 1;
							DiffuseBinding.DstBinding = 1;

							BmRender_DescriptorSetUpdateData Bindings[] = { DiffuseBinding };

							BmRender_UpdateDescriptorSet(&GetHandles()->FrameBufferSet, Bindings, 1);

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

				for (u32 j = 0; j < Model.MeshletCounts[i]; j++, ModelMeshletIndex++)
				{
					Shader_Meshlet& Meshlet = Meshlets[ModelMeshletIndex];
					Meshlet.InstanceIndex = InstanceIndex;
					Meshlet.VertexOffset += MeshletVertexCount;
					Meshlet.TriangleOffset += MeshletTriangleCount;
				}

				++InstanceIndex;
			}

			for (u32& MeshletVertex : MeshletVertices)
			{
				MeshletVertex += VertexCount;
			}

			const u64 VerticesSize = Model.Header.VertexCount * sizeof(Shader_StaticMeshVertex);
			const BmRender_GPUBufferUpdateData VertexHandle = { GetVertexBuffer(), VertexCount * sizeof(Shader_StaticMeshVertex), VerticesSize };
			RenderResources::UpdateBufferRegion(VertexHandle, 0, Model.Vertices, VerticesSize);

			const u64 MeshletsSize = Meshlets.size() * sizeof(Shader_Meshlet);
			const BmRender_GPUBufferUpdateData MeshletHandle = { GetMeshletBuffer(), MeshletCount * sizeof(Shader_Meshlet), MeshletsSize };
			RenderResources::UpdateBufferRegion(MeshletHandle, 0, Meshlets.data(), MeshletsSize);

			const u64 MeshletVerticesSize = MeshletVertices.size() * sizeof(u32);
			const BmRender_GPUBufferUpdateData MeshletVertexHandle = { GetMeshletVertexBuffer(), MeshletVertexCount * sizeof(u32), MeshletVerticesSize };
			RenderResources::UpdateBufferRegion(MeshletVertexHandle, 0, MeshletVertices.data(), MeshletVerticesSize);

			const u64 MeshletTrianglesSize = Model.Header.MeshletTriangleCount * sizeof(u32);
			const BmRender_GPUBufferUpdateData MeshletTriangleHandle = { GetMeshletTriangleBuffer(), MeshletTriangleCount * sizeof(u32), MeshletTrianglesSize };
			RenderResources::UpdateBufferRegion(MeshletTriangleHandle, 0, Model.MeshletTriangles, MeshletTrianglesSize);

			VertexCount += Model.Header.VertexCount;
			MeshletCount += Model.Header.MeshletCount;
			MeshletVertexCount += Model.Header.MeshletVertexCount;
			MeshletTriangleCount += Model.Header.MeshletTriangleCount;

			TmpScene->FrameDataBuffer.MeshletCount = MeshletCount;

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
		ModelLoadRequests.push(Request);
	}
}
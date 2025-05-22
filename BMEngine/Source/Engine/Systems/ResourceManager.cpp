#include "ResourceManager.h"

#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>

#include <map>

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Util/Util.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>

#include <thread>

namespace ResourceManager
{
	void Init()
	{
		const u64 DefaultTextureDataCount = sizeof(DefaultTextureData) / sizeof(DefaultTextureData[0]);

		std::hash<std::string> Hasher;

		gli::texture Texture = gli::load((char const*)DefaultTextureData, DefaultTextureDataCount);
		if (Texture.empty())
		{
			assert(false);
		}

		//VkFormat Format = static_cast<VkFormat>(gli::internal_format(Texture.format()));
		//u32 MipLevels = static_cast<uint32_t>(Texture.levels());
		glm::tvec3<u32> Extent = Texture.extent();

		RenderResources::CreateTexture2DSRGB(Hasher("Default"), Texture.data(), Extent.x, Extent.y);
	}

	void LoadModel(const char* FilePath)
	{
		Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
		Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		u64 ModelVertexByteOffset = 0;
		u32 ModelIndexCountOffset = 0;
		for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		{
			const u64 VerticesCount = Uh60Model.VerticesCounts[i];
			const u32 IndicesCount = Uh60Model.IndicesCounts[i];
			const u32 TextureIndex = RenderResources::GetTexture2DSRGBIndex(Uh60Model.DiffuseTexturesHashes[i]);

			RenderResources::Material Mat;
			Mat.AlbedoTexIndex = TextureIndex;
			Mat.SpecularTexIndex = TextureIndex;
			Mat.Shininess = 32.0f;
			const u32 MaterialIndex = RenderResources::CreateMaterial(&Mat);

			ModelVertexByteOffset += VerticesCount;
			ModelIndexCountOffset += IndicesCount;

			Render::Transfer(VulkanInterface::TestGetImageIndex());
		}

		VulkanInterface::WaitDevice();

		Util::ClearModel3DData(ModelData);

		u32 Index = VulkanInterface::TestGetImageIndex();

		std::thread LoadThread(
			[FilePath, Index]()
			{
				Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
				Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

				u64 ModelVertexByteOffset = 0;
				u32 ModelIndexCountOffset = 0;
				for (u32 i = 0; i < Uh60Model.MeshCount; i++)
				{
					const u64 VerticesCount = Uh60Model.VerticesCounts[i];
					const u32 IndicesCount = Uh60Model.IndicesCounts[i];

					Render::CreateEntity((StaticMeshRender::StaticMeshVertex*)Uh60Model.VertexData + ModelVertexByteOffset, sizeof(StaticMeshRender::StaticMeshVertex), VerticesCount,
						Uh60Model.IndexData + ModelIndexCountOffset, IndicesCount, 0, Index);

					ModelVertexByteOffset += VerticesCount;
					ModelIndexCountOffset += IndicesCount;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}

				std::this_thread::sleep_for(std::chrono::seconds(5));
				Util::ClearModel3DData(ModelData);
			}
		);

		LoadThread.detach();

		//{
		//	Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
		//	Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		//	u64 ModelVertexByteOffset = 0;
		//	u32 ModelIndexCountOffset = 0;
		//	for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		//	{
		//		const u64 VerticesCount = Uh60Model.VerticesCounts[i];
		//		const u32 IndicesCount = Uh60Model.IndicesCounts[i];

		//		Render::CreateEntity((StaticMeshRender::StaticMeshVertex*)Uh60Model.VertexData + ModelVertexByteOffset, sizeof(StaticMeshRender::StaticMeshVertex), VerticesCount,
		//			Uh60Model.IndexData + ModelIndexCountOffset, IndicesCount, 0, Index);

		//		ModelVertexByteOffset += VerticesCount;
		//		ModelIndexCountOffset += IndicesCount;
		//	}

		//	std::this_thread::sleep_for(std::chrono::seconds(5));
		//	Util::ClearModel3DData(ModelData);
		//}

		//Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
		//Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		//u64 ModelVertexByteOffset = 0;
		//u32 ModelIndexCountOffset = 0;
		//for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		//{
		//	const u64 VerticesCount = Uh60Model.VerticesCounts[i];
		//	const u32 IndicesCount = Uh60Model.IndicesCounts[i];
		//	const u32 TextureIndex = RenderResources::GetTexture2DSRGBIndex(Uh60Model.DiffuseTexturesHashes[i]);

		//	RenderResources::Material Mat;
		//	Mat.AlbedoTexIndex = TextureIndex;
		//	Mat.SpecularTexIndex = TextureIndex;
		//	Mat.Shininess = 32.0f;
		//	const u32 MaterialIndex = RenderResources::CreateMaterial(&Mat);

		//	RenderResources::CreateEntity((StaticMeshRender::StaticMeshVertex*)Uh60Model.VertexData + ModelVertexByteOffset, sizeof(StaticMeshRender::StaticMeshVertex), VerticesCount,
		//		Uh60Model.IndexData + ModelIndexCountOffset, IndicesCount, MaterialIndex);

		//	ModelVertexByteOffset += VerticesCount;
		//	ModelIndexCountOffset += IndicesCount;

		//	TransferSystem::Transfer();
		//}

		//Util::ClearModel3DData(ModelData);
	}

	void LoadTextures(const char* Directory)
	{
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind;

		char SearchPath[MAX_PATH];
		snprintf(SearchPath, sizeof(SearchPath), "%s\\*", Directory);

		hFind = FindFirstFileA(SearchPath, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			assert(false);
			return;
		}

		do
		{
			const char* FileName = FindFileData.cFileName;

			if (strcmp(FileName, ".") == 0 || strcmp(FileName, "..") == 0)
			{
				continue;
			}

			char FullPath[MAX_PATH];
			snprintf(FullPath, sizeof(FullPath), "%s\\%s", Directory, FileName);

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				LoadTextures(FullPath);
			}
			else
			{
				char FileNameNoExt[MAX_PATH];
				strncpy(FileNameNoExt, FileName, sizeof(FileNameNoExt));
				FileNameNoExt[sizeof(FileNameNoExt) - 1] = '\0';
				PathRemoveExtensionA(FileNameNoExt);

				std::hash<std::string> Hasher;

				gli::texture Texture = gli::load(FullPath);
				if (Texture.empty())
				{
					assert(false);
				}

				//VkFormat Format = static_cast<VkFormat>(gli::internal_format(Texture.format()));
				//u32 MipLevels = static_cast<uint32_t>(Texture.levels());
				glm::tvec3<u32> Extent = Texture.extent();

				RenderResources::CreateTexture2DSRGB(Hasher(FileNameNoExt), Texture.data(), Extent.x, Extent.y);
			}
		}
		while (FindNextFileA(hFind, &FindFileData));

		FindClose(hFind);
	}
}
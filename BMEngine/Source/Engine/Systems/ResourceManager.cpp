#include "ResourceManager.h"

#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>

#include <map>

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
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

		gli::texture DefaultTexture = gli::load((char const*)DefaultTextureData, DefaultTextureDataCount);
		if (DefaultTexture.empty())
		{
			assert(false);
		}

		//VkFormat Format = static_cast<VkFormat>(gli::internal_format(Texture.format()));
		//u32 MipLevels = static_cast<uint32_t>(Texture.levels());
		glm::tvec3<u32> Extent = DefaultTexture.extent();

		Render::CreateTexture2DSRGB(Hasher("Default"), DefaultTexture.data(), Extent.x, Extent.y);
	}

	void LoadModel(const char* FilePath, const char* Directory)
	{
		LoadTextures(Directory);

		Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
		Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		u64 ModelVertexByteOffset = 0;
		for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		{
			const u64 VerticesCount = Uh60Model.VerticesCounts[i];
			const u32 IndicesCount = Uh60Model.IndicesCounts[i];

			const u32 TextureIndex = Render::GetTexture2DSRGBIndex(Uh60Model.DiffuseTexturesHashes[i]);

			Render::Material Mat;
			Mat.AlbedoTexIndex = TextureIndex;
			Mat.SpecularTexIndex = TextureIndex;
			Mat.Shininess = 32.0f;
			const u32 MaterialIndex = Render::CreateMaterial(&Mat);

			Render::DrawEntity Entity = { };
			Entity.StaticMeshIndex = Render::CreateStaticMesh(Uh60Model.VertexData + ModelVertexByteOffset,
				sizeof(StaticMeshRender::StaticMeshVertex), VerticesCount, IndicesCount);
			Entity.MaterialIndex = MaterialIndex;
			Entity.Model = glm::mat4(1.0f);

			Render::CreateEntity(&Entity);

			ModelVertexByteOffset += VerticesCount * sizeof(StaticMeshRender::StaticMeshVertex) + IndicesCount * sizeof(u32);
		}

		Util::ClearModel3DData(ModelData);
		Render::NotifyTransfer();
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

				Render::CreateTexture2DSRGB(Hasher(FileNameNoExt), Texture.data(), Extent.x, Extent.y);
			}
		}
		while (FindNextFileA(hFind, &FindFileData));

		FindClose(hFind);
	}
}
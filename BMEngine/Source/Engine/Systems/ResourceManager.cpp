#include "ResourceManager.h"

#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>

#include <map>

#include "Engine/Systems/Render/TerrainRender.h"
#include "Util/Util.h"
#include "Util/DefaultTextureData.h"
#include <gli/gli.hpp>

#include <thread>

namespace ResourceManager
{
	struct TextureAsset
	{
		u64 Id;
		std::string Path;
		u32 RenderTextureIndex;
		bool IsRenderResourceCreated;
	};

	static std::map<u64, TextureAsset> TextureAssets;

	void Init()
	{
		const u64 DefaultTextureDataCount = sizeof(DefaultTextureData) / sizeof(DefaultTextureData[0]);

		std::hash<std::string> Hasher;

		gli::texture DefaultTexture = gli::load((char const*)DefaultTextureData, DefaultTextureDataCount);
		if (DefaultTexture.empty())
		{
			assert(false);
		}

		glm::tvec3<u32> Extent = DefaultTexture.extent();

		TextureAsset DefaultTextureAsset;
		DefaultTextureAsset.Id = Hasher("Default");
		DefaultTextureAsset.IsRenderResourceCreated = false;
		DefaultTextureAsset.RenderTextureIndex = Render::CreateTexture2DSRGB(DefaultTextureAsset.Id, DefaultTexture.data(), Extent.x, Extent.y);

		TextureAssets.emplace(DefaultTextureAsset.Id, std::move(DefaultTextureAsset));
	}

	void LoadModel(const char* FilePath, const char* Directory)
	{
		InitTextureAssets(Directory);

		Util::Model3DData ModelData = Util::LoadModel3DData(FilePath);
		Util::Model3D Uh60Model = Util::ParseModel3D(ModelData);

		u64 ModelVertexByteOffset = 0;
		for (u32 i = 0; i < Uh60Model.MeshCount; i++)
		{
			const u64 VerticesCount = Uh60Model.VerticesCounts[i];
			const u32 IndicesCount = Uh60Model.IndicesCounts[i];

			u32 TextureIndex = 0;

			auto it = TextureAssets.find(Uh60Model.DiffuseTexturesHashes[i]);
			if (it != TextureAssets.end())
			{
				if (it->second.IsRenderResourceCreated)
				{
					TextureIndex = it->second.RenderTextureIndex;
				}
				else
				{
					gli::texture Texture = gli::load(it->second.Path);
					if (Texture.empty())
					{
						assert(false);
					}

					glm::tvec3<u32> Extent = Texture.extent();
					TextureIndex = Render::CreateTexture2DSRGB(it->second.Id, Texture.data(), Extent.x, Extent.y);

					it->second.RenderTextureIndex = TextureIndex;
					it->second.IsRenderResourceCreated = true;
				}
			}

			Render::Material Mat;
			Mat.AlbedoTexIndex = TextureIndex;
			Mat.SpecularTexIndex = TextureIndex;
			Mat.Shininess = 32.0f;
			const u32 MaterialIndex = Render::CreateMaterial(&Mat);

			Render::DrawEntity Entity = { };
			Entity.StaticMeshIndex = Render::CreateStaticMesh(Uh60Model.VertexData + ModelVertexByteOffset,
				sizeof(Render::StaticMeshVertex), VerticesCount, IndicesCount);
			Entity.MaterialIndex = MaterialIndex;
			Entity.Model = glm::mat4(1.0f);

			Render::CreateEntity(&Entity);

			ModelVertexByteOffset += VerticesCount * sizeof(Render::StaticMeshVertex) + IndicesCount * sizeof(u32);
		}

		Util::ClearModel3DData(ModelData);
		Render::NotifyTransfer();
	}

	void InitTextureAssets(const char* Directory)
	{
		// Check on resource leak
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
				InitTextureAssets(FullPath);
			}
			else
			{
				char FileNameNoExt[MAX_PATH];
				strncpy(FileNameNoExt, FileName, sizeof(FileNameNoExt));
				FileNameNoExt[sizeof(FileNameNoExt) - 1] = '\0';
				PathRemoveExtensionA(FileNameNoExt);

				std::hash<std::string> Hasher;

				TextureAsset DefaultTextureAsset;
				DefaultTextureAsset.Id = Hasher(FileNameNoExt);
				DefaultTextureAsset.Path = FullPath;
				DefaultTextureAsset.IsRenderResourceCreated = false;
				DefaultTextureAsset.RenderTextureIndex = 0;

				TextureAssets.emplace(DefaultTextureAsset.Id, std::move(DefaultTextureAsset));
			}
		}
		while (FindNextFileA(hFind, &FindFileData));

		FindClose(hFind);
	}
}
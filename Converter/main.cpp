#include "Engine/Assets.h"
#include "Util/EngineTypes.h"
#include "Engine/Systems/StaticMeshSystem.h"

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

struct VertexEqual
{
	bool operator()(const StaticMeshSystem::StaticMeshVertex& lhs, const StaticMeshSystem::StaticMeshVertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.TextureCoords == rhs.TextureCoords;
	}
};

template<> struct std::hash<StaticMeshSystem::StaticMeshVertex>
{
	size_t operator()(StaticMeshSystem::StaticMeshVertex const& vertex) const
	{
		size_t hashPosition = std::hash<glm::vec3>()(vertex.Position);
		size_t hashTextureCoords = std::hash<glm::vec2>()(vertex.TextureCoords);
		size_t hashNormal = std::hash<glm::vec3>()(vertex.Normal);

		size_t combinedHash = hashPosition;
		combinedHash ^= (hashTextureCoords << 1);
		combinedHash ^= (hashNormal << 1);

		return combinedHash;
	}
};

void CreateDirectoryRecursively(const char* Path)
{
	char FolderName[MAX_PATH];

	for (u32 i = 0; Path[i]; ++i)
	{
		FolderName[i] = Path[i];

		if (Path[i] == '\\' || Path[i] == '/')
		{
			FolderName[i] = '\0';

			if (GetFileAttributes(FolderName) == INVALID_FILE_ATTRIBUTES)
			{
				CreateDirectory(FolderName, NULL);
			}

			FolderName[i] = '\\';
		}
	}
}

void ObjModelToEngineStaticMeshModel(const char* FilePath, const char* OutputPath)
{
	char BaseDir[MAX_PATH];
	strcpy(BaseDir, FilePath);
	PathRemoveFileSpec(BaseDir);

	char NewAssetPath[MAX_PATH];
	strcpy(NewAssetPath, OutputPath);
	strcat(NewAssetPath, PathFindFileName(FilePath));
	PathRemoveExtension(NewAssetPath);
	strcat(NewAssetPath, ".model");

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, FilePath, BaseDir))
	{
		assert(false);
	}

	Assets::Model3D Model;
	Model.VertexData = nullptr;
	Model.VertexOffsets = (u64*)malloc(Shapes.size() * sizeof(u64));
	Model.IndexData = nullptr;
	Model.IndexOffsets = (u32*)malloc(Shapes.size() * sizeof(u32));
	Model.IndicesCounts = (u32*)malloc(Shapes.size() * sizeof(u32));
	Model.DiffuseTextures = nullptr;

	std::unordered_map<StaticMeshSystem::StaticMeshVertex, u32,
		std::hash<StaticMeshSystem::StaticMeshVertex>, VertexEqual> uniqueVertices{ };

	std::string Textures;
	std::vector<StaticMeshSystem::StaticMeshVertex> AllVertices;
	std::vector<u32> AllIndices;

	std::vector<StaticMeshSystem::StaticMeshVertex> Vertices;
	std::vector<u32> Indices;

	u64 VertexOffset = 0;
	u64 IndexOffset = 0;

	for (u32 i = 0; i < Shapes.size(); i++)
	{
		Vertices.clear();
		Indices.clear();

		const tinyobj::shape_t* Shape = Shapes.data() + i;

		for (u32 j = 0; j < Shape->mesh.indices.size(); j++)
		{
			tinyobj::index_t Index = Shape->mesh.indices[j];

			StaticMeshSystem::StaticMeshVertex vertex = { };

			vertex.Position =
			{
				Attrib.vertices[3 * Index.vertex_index + 0],
				Attrib.vertices[3 * Index.vertex_index + 1],
				Attrib.vertices[3 * Index.vertex_index + 2]
			};

			vertex.TextureCoords =
			{
				Attrib.texcoords[2 * Index.texcoord_index + 0],
				Attrib.texcoords[2 * Index.texcoord_index + 1]
			};

			if (Index.normal_index >= 0)
			{
				vertex.Normal =
				{
					Attrib.normals[3 * Index.normal_index + 0],
					Attrib.normals[3 * Index.normal_index + 1],
					Attrib.normals[3 * Index.normal_index + 2]
				};
			}

			if (!uniqueVertices.count(vertex))
			{
				uniqueVertices[vertex] = static_cast<u32>(Vertices.size());
				Vertices.push_back(vertex);
			}

			Indices.push_back(uniqueVertices[vertex]);
		}

		const std::string* DiffuseTexture = &Materials[Shape->mesh.material_ids[0]].diffuse_texname;

		Textures += "{" + std::to_string(DiffuseTexture->size()) + "}" + *DiffuseTexture;
		Model.VertexOffsets[i] = VertexOffset;
		Model.IndexOffsets[i] = IndexOffset;
		Model.IndicesCounts[i] = static_cast<u32>(Indices.size());

		VertexOffset += Vertices.size();
		IndexOffset += Indices.size();

		AllVertices.insert(AllVertices.end(), Vertices.begin(), Vertices.end());
		AllIndices.insert(AllIndices.end(), Indices.begin(), Indices.end());
	}

	Textures += "{}";

	HANDLE hFile = CreateFileA(NewAssetPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		assert(false);
		return;
	}

	DWORD BytesWritten;
	BOOL Result = 1;

	Assets::Model3DFileHeader Header;
	Header.MeshCount = Shapes.size();
	Header.VerticesCount = AllVertices.size();
	Header.IndicesCount = AllIndices.size();
	Header.DiffuseTexturesStringSize = Textures.size();

	Result &= WriteFile(hFile, &Header, sizeof(Header), &BytesWritten, NULL);
	Result &= WriteFile(hFile, AllVertices.data(), Header.VerticesCount * sizeof(AllVertices[0]), &BytesWritten, NULL);
	Result &= WriteFile(hFile, Model.VertexOffsets, Header.MeshCount * sizeof(Model.VertexOffsets[0]), &BytesWritten, NULL);
	Result &= WriteFile(hFile, AllIndices.data(), Header.IndicesCount * sizeof(AllIndices[0]), &BytesWritten, NULL);
	Result &= WriteFile(hFile, Model.IndexOffsets, Header.MeshCount * sizeof(Model.IndexOffsets[0]), &BytesWritten, NULL);
	Result &= WriteFile(hFile, Model.IndicesCounts, Header.MeshCount * sizeof(Model.IndicesCounts[0]), &BytesWritten, NULL);
	Result &= WriteFile(hFile, Textures.data(), Header.DiffuseTexturesStringSize, &BytesWritten, NULL);

	CloseHandle(hFile);

	free(Model.VertexOffsets);
	free(Model.IndexOffsets);

	assert(Result);
}

int main(u32 argc, const char* argv[])
{
	if (argc < 2)
	{
		argc = 3;
		argv[0] = "-m";
		argv[1] = "D:\\Code\\BMEngine\\BMEngine\\Resources/Assets/Models/";
		argv[2] = "D:\\Code\\BMEngine\\BMEngine\\Resources\\Models\\uh60.obj";
	}

	for (u32 i = 0; i < argc; i++)
	{
		const char* Command = argv[i];
		if (strcmp(Command, "-m") == 0)
		{
			const char* OutputPath = argv[1];
			CreateDirectoryRecursively(OutputPath);

			printf("Output path: %s\n", OutputPath);

			for (u32 j = 2; j < argc; j++)
			{
				const char* File = argv[j];
				printf("  %s\n", File);

				if (GetFileAttributesA(File) != INVALID_FILE_ATTRIBUTES)
				{
					ObjModelToEngineStaticMeshModel(File, OutputPath);
				}
				else
				{
					printf("  -> File does not exist: %s\n", argv[j]);
				}
			}
			break;
		}
	}

	return 0;
}
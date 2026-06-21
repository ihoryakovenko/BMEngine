#include "Util/Util.h"

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

#include <ShortTypes.h>

#include <filesystem>
#include <unordered_set>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct VertexEqual
{
	bool operator()(const Shader_StaticMeshVertex& lhs, const Shader_StaticMeshVertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.TextureCoords == rhs.TextureCoords;
	}
};

template<> struct std::hash<Shader_StaticMeshVertex>
{
	size_t operator()(Shader_StaticMeshVertex const& vertex) const
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

void ObjToModel3D(const char* FilePath, const char* OutputPath)
{
	namespace fs = std::filesystem;

	fs::path filePath = FilePath;
	fs::path outputPath = OutputPath;

	fs::path BaseDir = filePath.parent_path();

	fs::path filenameWithoutExt = filePath.stem();
	fs::path newAssetPath = outputPath / (filenameWithoutExt.string() + ".model");

	// Optionally convert back to std::string if needed
	std::string baseDirStr = BaseDir.string();
	std::string newAssetPathStr = newAssetPath.string();

	tinyobj::attrib_t Attrib;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Warn, Err;

	if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, FilePath, baseDirStr.c_str()))
	{
		assert(false);
	}


	u64* VerticesCounts = (u64*)malloc(Shapes.size() * sizeof(u64));
	u32* IndicesCounts = (u32*)malloc(Shapes.size() * sizeof(u32));

	std::unordered_map<Shader_StaticMeshVertex, u32,
		std::hash<Shader_StaticMeshVertex>, VertexEqual> uniqueVertices{ };

	std::hash<std::string> Hasher;


	std::vector<Util::Model3DMaterial> uniqueMaterials;
	std::vector<u32> meshMaterialIndices;
	std::vector<u64> uniqueTextureHashes;
	std::vector<u8> VerticesAndIndices;
	std::vector<Shader_StaticMeshVertex> Vertices;
	std::vector<u32> Indices;

	std::unordered_set<u64> textureHashes;

	u64 VertexShift = 0;

	uniqueMaterials.reserve(Materials.size());
	uniqueTextureHashes.reserve(Materials.size());

	for (u32 i = 0; i < Materials.size(); i++)
	{
		Util::Model3DMaterial NewMaterial;

		std::string diffuseName = Materials[i].diffuse_texname.empty() ? "" : fs::path(Materials[i].diffuse_texname).stem().string();
		std::string specularName = Materials[i].specular_texname.empty() ? "" : fs::path(Materials[i].specular_texname).stem().string();

		NewMaterial.DiffuseTextureHash = diffuseName.empty() ? 0 : Hasher(diffuseName);
		NewMaterial.SpecularTextureHash = specularName.empty() ? 0 : Hasher(specularName);

		if (textureHashes.find(NewMaterial.DiffuseTextureHash) == textureHashes.end())
		{
			textureHashes.insert(NewMaterial.DiffuseTextureHash);
			uniqueTextureHashes.push_back(NewMaterial.DiffuseTextureHash);
		}

		if (textureHashes.find(NewMaterial.SpecularTextureHash) == textureHashes.end())
		{
			textureHashes.insert(NewMaterial.SpecularTextureHash);
			uniqueTextureHashes.push_back(NewMaterial.SpecularTextureHash);
		}

		uniqueMaterials.push_back(NewMaterial);
	}

	meshMaterialIndices.reserve(Shapes.size());

	for (u32 i = 0; i < Shapes.size(); i++)
	{
		Vertices.clear();
		Indices.clear();

		const tinyobj::shape_t* Shape = Shapes.data() + i;

		for (u32 j = 0; j < Shape->mesh.indices.size(); j++)
		{
			tinyobj::index_t Index = Shape->mesh.indices[j];

			Shader_StaticMeshVertex vertex = { };

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

			Indices.push_back(uniqueVertices[vertex] + VertexShift);
			//Indices.push_back(uniqueVertices[vertex]);
		}

		VertexShift += Vertices.size();
		VerticesCounts[i] = Vertices.size();
		IndicesCounts[i] = Indices.size();

		u64 VertexBytes = Vertices.size() * sizeof(Shader_StaticMeshVertex);
		u64 IndexBytes = Indices.size() * sizeof(u32);

		u64 CurrentOffset = VerticesAndIndices.size();
		VerticesAndIndices.resize(CurrentOffset + VertexBytes + IndexBytes);

		std::memcpy(VerticesAndIndices.data() + CurrentOffset, Vertices.data(), VertexBytes);
		std::memcpy(VerticesAndIndices.data() + CurrentOffset + VertexBytes, Indices.data(), IndexBytes);

		if (Shape->mesh.material_ids[0] != -1)
		{
			meshMaterialIndices.push_back(Shape->mesh.material_ids[0]);
		}
	}

	std::ofstream outFile(newAssetPathStr, std::ios::binary);
	if (!outFile)
	{
		assert(false);
		return;
	}

	// Write the header
	Util::Model3DFileHeader Header;
	Header.MeshCount = Shapes.size();
	Header.VertexDataSize = VerticesAndIndices.size();
	Header.MaterialCount = uniqueMaterials.size();
	Header.UniqueTextureCount = uniqueTextureHashes.size();

	outFile.write(reinterpret_cast<const char*>(&Header), sizeof(Header));
	outFile.write(reinterpret_cast<const char*>(VerticesAndIndices.data()), Header.VertexDataSize);
	outFile.write(reinterpret_cast<const char*>(VerticesCounts), Header.MeshCount * sizeof(VerticesCounts[0]));
	outFile.write(reinterpret_cast<const char*>(IndicesCounts), Header.MeshCount * sizeof(IndicesCounts[0]));
	outFile.write(reinterpret_cast<const char*>(meshMaterialIndices.data()), meshMaterialIndices.size() * sizeof(meshMaterialIndices[0]));
	outFile.write(reinterpret_cast<const char*>(uniqueMaterials.data()), Header.MaterialCount * sizeof(Util::Model3DMaterial));
	outFile.write(reinterpret_cast<const char*>(uniqueTextureHashes.data()), Header.UniqueTextureCount * sizeof(u64));

	free(VerticesCounts);
	free(IndicesCounts);
}

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

int main(u32 argc, const char* argv[])
{
	if (argc < 2)
	{
		argc = 3;
		argv[0] = "-m";
		argv[1] = "E:\\Code\\BMEngine\\BMEngine\\Resources\\Models/";
		argv[2] = "E:\\Code\\BMEngine\\BMEngine\\Resources\\Models\\uh60.obj";
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
					ObjToModel3D(File, OutputPath);
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
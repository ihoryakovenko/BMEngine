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

#include <meshoptimizer.h>

static const float MeshletConeWeight = 0.25f;

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


	std::unordered_map<Shader_StaticMeshVertex, u32,
		std::hash<Shader_StaticMeshVertex>, VertexEqual> uniqueVertices{ };

	std::hash<std::string> Hasher;


	std::vector<Util::Model3DMaterial> uniqueMaterials;
	std::vector<u32> meshMaterialIndices;
	std::vector<u64> uniqueTextureHashes;
	std::vector<Shader_StaticMeshVertex> Vertices;
	std::vector<u32> Indices;

	std::unordered_set<u64> textureHashes;

	std::vector<Shader_StaticMeshVertex> ModelVertices;
	std::vector<Shader_Meshlet> ModelMeshlets;
	std::vector<u32> ModelMeshletVertices;
	std::vector<u32> ModelMeshletTriangles;
	std::vector<u32> MeshletCounts;

	std::vector<meshopt_Meshlet> Meshlets;
	std::vector<u32> MeshletVertices;
	std::vector<u8> MeshletTriangles;

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
	MeshletCounts.reserve(Shapes.size());

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

			Indices.push_back(uniqueVertices[vertex]);
		}

		u64 MeshletCount = 0;

		if (!Indices.empty())
		{
			Meshlets.resize(meshopt_buildMeshletsBound(Indices.size(), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES));
			MeshletVertices.resize(Indices.size());
			MeshletTriangles.resize(Indices.size());

			MeshletCount = meshopt_buildMeshlets(Meshlets.data(), MeshletVertices.data(), MeshletTriangles.data(),
				Indices.data(), Indices.size(), &Vertices[0].Position.x, Vertices.size(), sizeof(Shader_StaticMeshVertex),
				MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES, MeshletConeWeight);
		}

		for (u64 j = 0; j < MeshletCount; j++)
		{
			const meshopt_Meshlet& Meshlet = Meshlets[j];
			u32* LocalVertices = MeshletVertices.data() + Meshlet.vertex_offset;
			u8* LocalTriangles = MeshletTriangles.data() + Meshlet.triangle_offset;

			meshopt_optimizeMeshlet(LocalVertices, LocalTriangles, Meshlet.triangle_count, Meshlet.vertex_count);

			const meshopt_Bounds Bounds = meshopt_computeMeshletBounds(LocalVertices, LocalTriangles, Meshlet.triangle_count,
				&Vertices[0].Position.x, Vertices.size(), sizeof(Shader_StaticMeshVertex));

			Shader_Meshlet NewMeshlet = { };
			NewMeshlet.BoundsCenter = { Bounds.center[0], Bounds.center[1], Bounds.center[2] };
			NewMeshlet.BoundsRadius = Bounds.radius;
			NewMeshlet.ConeAxis = { Bounds.cone_axis[0], Bounds.cone_axis[1], Bounds.cone_axis[2] };
			NewMeshlet.ConeCutoff = Bounds.cone_cutoff;
			NewMeshlet.VertexOffset = static_cast<u32>(ModelMeshletVertices.size());
			NewMeshlet.TriangleOffset = static_cast<u32>(ModelMeshletTriangles.size());
			NewMeshlet.VertexCount = Meshlet.vertex_count;
			NewMeshlet.TriangleCount = Meshlet.triangle_count;
			ModelMeshlets.push_back(NewMeshlet);

			for (u32 k = 0; k < Meshlet.vertex_count; k++)
			{
				ModelMeshletVertices.push_back(LocalVertices[k] + static_cast<u32>(ModelVertices.size()));
			}

			for (u32 k = 0; k < Meshlet.triangle_count; k++)
			{
				const u8* Triangle = LocalTriangles + k * 3;
				ModelMeshletTriangles.push_back(Triangle[0] | (Triangle[1] << 8) | (Triangle[2] << 16));
			}
		}

		ModelVertices.insert(ModelVertices.end(), Vertices.begin(), Vertices.end());
		MeshletCounts.push_back(static_cast<u32>(MeshletCount));
		meshMaterialIndices.push_back(static_cast<u32>(Shape->mesh.material_ids[0]));
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
	Header.MaterialCount = uniqueMaterials.size();
	Header.UniqueTextureCount = uniqueTextureHashes.size();
	Header.VertexCount = ModelVertices.size();
	Header.MeshletCount = ModelMeshlets.size();
	Header.MeshletVertexCount = ModelMeshletVertices.size();
	Header.MeshletTriangleCount = ModelMeshletTriangles.size();

	outFile.write(reinterpret_cast<const char*>(&Header), sizeof(Header));
	outFile.write(reinterpret_cast<const char*>(ModelVertices.data()), Header.VertexCount * sizeof(Shader_StaticMeshVertex));
	outFile.write(reinterpret_cast<const char*>(ModelMeshlets.data()), Header.MeshletCount * sizeof(Shader_Meshlet));
	outFile.write(reinterpret_cast<const char*>(ModelMeshletVertices.data()), Header.MeshletVertexCount * sizeof(u32));
	outFile.write(reinterpret_cast<const char*>(ModelMeshletTriangles.data()), Header.MeshletTriangleCount * sizeof(u32));
	outFile.write(reinterpret_cast<const char*>(MeshletCounts.data()), Header.MeshCount * sizeof(u32));
	outFile.write(reinterpret_cast<const char*>(meshMaterialIndices.data()), Header.MeshCount * sizeof(u32));
	outFile.write(reinterpret_cast<const char*>(uniqueMaterials.data()), Header.MaterialCount * sizeof(Util::Model3DMaterial));
	outFile.write(reinterpret_cast<const char*>(uniqueTextureHashes.data()), Header.UniqueTextureCount * sizeof(u64));
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
	if (argc < 4 || strcmp(argv[1], "-m") != 0)
	{
		printf("Usage: %s -m <OUTPUT_DIR> <FILE.obj>...\n", argv[0]);
		return 1;
	}

	const char* OutputPath = argv[2];
	CreateDirectoryRecursively(OutputPath);

	printf("Output path: %s\n", OutputPath);

	for (u32 i = 3; i < argc; i++)
	{
		const char* File = argv[i];
		printf("  %s\n", File);

		if (GetFileAttributesA(File) != INVALID_FILE_ATTRIBUTES)
		{
			ObjToModel3D(File, OutputPath);
		}
		else
		{
			printf("  -> File does not exist: %s\n", File);
		}
	}

	return 0;
}
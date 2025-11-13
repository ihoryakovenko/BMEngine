#include "Util.h"

#include <mini-yaml/yaml/Yaml.hpp>
#include <vector>
#include <algorithm>

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
FORGE_MEMORY_DEBUG
#include <forge_memory_debugger.h>

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Engine/Systems/Render/RenderTypes.h"
#include "Engine/Systems/EngineResources.h"
#include "gli/gli.hpp"

// Extern declarations for global resource maps
extern std::unordered_map<std::string, BmRender_Sampler> Samplers;
extern std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
extern std::unordered_map<std::string, BmRender_Shader> Shaders;
extern std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
extern std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;
extern 	std::unordered_map<std::string, BmRender_PushConstant> PushConstants;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>

#include <filesystem>
#include <string>
#include <cstdarg>
#include <unordered_set>
#include <iostream>

struct VertexEqual
{
	bool operator()(const EngineResources::StaticMeshVertex& lhs, const EngineResources::StaticMeshVertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.TextureCoords == rhs.TextureCoords;
	}
};

template<> struct std::hash<EngineResources::StaticMeshVertex>
{
	size_t operator()(EngineResources::StaticMeshVertex const& vertex) const
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

namespace Util
{
	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData)
	{
		if (fseek(File, 0L, SEEK_END) != 0)
		{
			return false;
		}

		long FileSize = ftell(File);
		if (FileSize < 0)
		{
			return false;
		}

		rewind(File);

		OutFileData.resize(static_cast<size_t>(FileSize));
		size_t ReadResult = fread(OutFileData.data(), 1, static_cast<size_t>(FileSize), File);

		return ReadResult == static_cast<size_t>(FileSize);
	}

	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode)
	{
		FILE* File = fopen(FileName, Mode);
		if (File)
		{
			if (ReadFileFull(File, OutFileData))
			{
				fclose(File);
				return true;
			}

			Log::Error("Failed to read file {}: ", FileName);
			fclose(File);
			return false;
		}

		Log::Error("Cannot open file {}: Result = {}", FileName, 0);
		return false;
	}

	void RenderLog(LogType logType, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		RenderLog(logType, format, args);
		va_end(args);
	}

	void RenderLog(LogType LogType, const char* Format, va_list Args)
	{
		switch (LogType)
		{
			case LogType::Error:
			{
				std::cout << "\033[31;5mError: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				assert(false);
				break;
			}
			case LogType::Warning:
			{
				std::cout << "\033[33;5mWarning: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				break;
			}
			case LogType::Info:
			{
				std::cout << "Info: ";
				vprintf(Format, Args);
				std::cout << '\n';
				break;
			}
		}
	}

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

		std::unordered_map<EngineResources::StaticMeshVertex, u32,
			std::hash<EngineResources::StaticMeshVertex>, VertexEqual> uniqueVertices{ };

		std::hash<std::string> Hasher;


		std::vector<Model3DMaterial> uniqueMaterials;
		std::vector<u32> meshMaterialIndices;
		std::vector<u64> uniqueTextureHashes;
		std::vector<u8> VerticesAndIndices;
		std::vector<EngineResources::StaticMeshVertex> Vertices;
		std::vector<u32> Indices;

		std::unordered_set<u64> textureHashes;

		uniqueMaterials.reserve(Materials.size());
		uniqueTextureHashes.reserve(Materials.size());

		for (u32 i = 0; i < Materials.size(); i++)
		{
			Model3DMaterial NewMaterial;

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

				EngineResources::StaticMeshVertex vertex = { };

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

			VerticesCounts[i] = Vertices.size();
			IndicesCounts[i] = Indices.size();

			u64 VertexBytes = Vertices.size() * sizeof(EngineResources::StaticMeshVertex);
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
		Model3DFileHeader Header;
		Header.MeshCount = Shapes.size();
		Header.VertexDataSize = VerticesAndIndices.size();
		Header.MaterialCount = uniqueMaterials.size();
		Header.UniqueTextureCount = uniqueTextureHashes.size();

		outFile.write(reinterpret_cast<const char*>(&Header), sizeof(Header));
		outFile.write(reinterpret_cast<const char*>(VerticesAndIndices.data()), Header.VertexDataSize);
		outFile.write(reinterpret_cast<const char*>(VerticesCounts), Header.MeshCount * sizeof(VerticesCounts[0]));
		outFile.write(reinterpret_cast<const char*>(IndicesCounts), Header.MeshCount * sizeof(IndicesCounts[0]));
		outFile.write(reinterpret_cast<const char*>(meshMaterialIndices.data()), meshMaterialIndices.size() * sizeof(meshMaterialIndices[0]));
		outFile.write(reinterpret_cast<const char*>(uniqueMaterials.data()), Header.MaterialCount * sizeof(Model3DMaterial));
		outFile.write(reinterpret_cast<const char*>(uniqueTextureHashes.data()), Header.UniqueTextureCount * sizeof(u64));

		free(VerticesCounts);
		free(IndicesCounts);
	}

	Model3DData LoadModel3DData(const char* FilePath)
	{
		namespace fs = std::filesystem;

		fs::path path(FilePath);
		std::error_code ec;
		std::uintmax_t fileSize = fs::file_size(path, ec);
		if (ec)
		{
			assert(false);
		}

		std::ifstream file(FilePath, std::ios::binary);
		if (!file)
		{
			assert(false);
		}

		Model3DData Data = (Model3DData)malloc(fileSize * sizeof(u8));
		file.read(reinterpret_cast<char*>(Data), fileSize);
		if (!file)
		{
			assert(false);
		}

		return Data;
	}

	void ClearModel3DData(Model3DData Data)
	{
		free(Data);
	}

	Model3D ParseModel3D(Model3DData Data)
	{
		Model3D Model;

		memcpy(&Model.Header, Data, sizeof(Model3DFileHeader));
		Data += sizeof(Model3DFileHeader);

		Model.VertexData = Data;
		Data += Model.Header.VertexDataSize;

		Model.VerticesCounts = (u64*)Data;
		Data += Model.Header.MeshCount * sizeof(u64);

		Model.IndicesCounts = (u32*)Data;
		Data += Model.Header.MeshCount * sizeof(u32);

		Model.MaterialIndices = (u32*)Data;
		Data += Model.Header.MeshCount * sizeof(u32);

		Model.Materials = (Model3DMaterial*)Data;
		Data += Model.Header.MaterialCount * sizeof(Model3DMaterial);

		Model.UniqueTextureHashes = (u64*)Data;

		return Model;
	}



	u32 GetAttributeTypeSize(BmRender_AttributeType Attribute)
	{
		switch (Attribute)
		{
			case BmRender_AttributeType::Int:
			case BmRender_AttributeType::Uint:
			case BmRender_AttributeType::Float:
				return 4;
			case BmRender_AttributeType::Vec2:
				return 8;
			case BmRender_AttributeType::Vec3:
				return 12;
			case BmRender_AttributeType::Vec4:
				return 16;
			case BmRender_AttributeType::Mat4:
				return 64;
			default: assert(false);
		}
	}

	VkFormat GliFormatToVkFormat(gli::format Format)
	{
		switch (Format)
		{
			// 8-bit formats
			case gli::FORMAT_R8_UNORM_PACK8: return VK_FORMAT_R8_UNORM;
			case gli::FORMAT_R8_SNORM_PACK8: return VK_FORMAT_R8_SNORM;
			case gli::FORMAT_R8_USCALED_PACK8: return VK_FORMAT_R8_USCALED;
			case gli::FORMAT_R8_SSCALED_PACK8: return VK_FORMAT_R8_SSCALED;
			case gli::FORMAT_R8_UINT_PACK8: return VK_FORMAT_R8_UINT;
			case gli::FORMAT_R8_SINT_PACK8: return VK_FORMAT_R8_SINT;
			case gli::FORMAT_R8_SRGB_PACK8: return VK_FORMAT_R8_SRGB;

				// 16-bit formats
			case gli::FORMAT_RG8_UNORM_PACK8: return VK_FORMAT_R8G8_UNORM;
			case gli::FORMAT_RG8_SNORM_PACK8: return VK_FORMAT_R8G8_SNORM;
			case gli::FORMAT_RG8_USCALED_PACK8: return VK_FORMAT_R8G8_USCALED;
			case gli::FORMAT_RG8_SSCALED_PACK8: return VK_FORMAT_R8G8_SSCALED;
			case gli::FORMAT_RG8_UINT_PACK8: return VK_FORMAT_R8G8_UINT;
			case gli::FORMAT_RG8_SINT_PACK8: return VK_FORMAT_R8G8_SINT;
			case gli::FORMAT_RG8_SRGB_PACK8: return VK_FORMAT_R8G8_SRGB;

				// 24-bit formats
			case gli::FORMAT_RGB8_UNORM_PACK8: return VK_FORMAT_R8G8B8_UNORM;
			case gli::FORMAT_RGB8_SNORM_PACK8: return VK_FORMAT_R8G8B8_SNORM;
			case gli::FORMAT_RGB8_USCALED_PACK8: return VK_FORMAT_R8G8B8_USCALED;
			case gli::FORMAT_RGB8_SSCALED_PACK8: return VK_FORMAT_R8G8B8_SSCALED;
			case gli::FORMAT_RGB8_UINT_PACK8: return VK_FORMAT_R8G8B8_UINT;
			case gli::FORMAT_RGB8_SINT_PACK8: return VK_FORMAT_R8G8B8_SINT;
			case gli::FORMAT_RGB8_SRGB_PACK8: return VK_FORMAT_R8G8B8_SRGB;

			case gli::FORMAT_BGR8_UNORM_PACK8: return VK_FORMAT_B8G8R8_UNORM;
			case gli::FORMAT_BGR8_SNORM_PACK8: return VK_FORMAT_B8G8R8_SNORM;
			case gli::FORMAT_BGR8_USCALED_PACK8: return VK_FORMAT_B8G8R8_USCALED;
			case gli::FORMAT_BGR8_SSCALED_PACK8: return VK_FORMAT_B8G8R8_SSCALED;
			case gli::FORMAT_BGR8_UINT_PACK8: return VK_FORMAT_B8G8R8_UINT;
			case gli::FORMAT_BGR8_SINT_PACK8: return VK_FORMAT_B8G8R8_SINT;
			case gli::FORMAT_BGR8_SRGB_PACK8: return VK_FORMAT_B8G8R8_SRGB;

				// 32-bit formats
			case gli::FORMAT_RGBA8_UNORM_PACK8: return VK_FORMAT_R8G8B8A8_UNORM;
			case gli::FORMAT_RGBA8_SNORM_PACK8: return VK_FORMAT_R8G8B8A8_SNORM;
			case gli::FORMAT_RGBA8_USCALED_PACK8: return VK_FORMAT_R8G8B8A8_USCALED;
			case gli::FORMAT_RGBA8_SSCALED_PACK8: return VK_FORMAT_R8G8B8A8_SSCALED;
			case gli::FORMAT_RGBA8_UINT_PACK8: return VK_FORMAT_R8G8B8A8_UINT;
			case gli::FORMAT_RGBA8_SINT_PACK8: return VK_FORMAT_R8G8B8A8_SINT;
			case gli::FORMAT_RGBA8_SRGB_PACK8: return VK_FORMAT_R8G8B8A8_SRGB;

			case gli::FORMAT_BGRA8_UNORM_PACK8: return VK_FORMAT_B8G8R8A8_UNORM;
			case gli::FORMAT_BGRA8_SNORM_PACK8: return VK_FORMAT_B8G8R8A8_SNORM;
			case gli::FORMAT_BGRA8_USCALED_PACK8: return VK_FORMAT_B8G8R8A8_USCALED;
			case gli::FORMAT_BGRA8_SSCALED_PACK8: return VK_FORMAT_B8G8R8A8_SSCALED;
			case gli::FORMAT_BGRA8_UINT_PACK8: return VK_FORMAT_B8G8R8A8_UINT;
			case gli::FORMAT_BGRA8_SINT_PACK8: return VK_FORMAT_B8G8R8A8_SINT;
			case gli::FORMAT_BGRA8_SRGB_PACK8: return VK_FORMAT_B8G8R8A8_SRGB;

				// 16-bit per component formats
			case gli::FORMAT_R16_UNORM_PACK16: return VK_FORMAT_R16_UNORM;
			case gli::FORMAT_R16_SNORM_PACK16: return VK_FORMAT_R16_SNORM;
			case gli::FORMAT_R16_USCALED_PACK16: return VK_FORMAT_R16_USCALED;
			case gli::FORMAT_R16_SSCALED_PACK16: return VK_FORMAT_R16_SSCALED;
			case gli::FORMAT_R16_UINT_PACK16: return VK_FORMAT_R16_UINT;
			case gli::FORMAT_R16_SINT_PACK16: return VK_FORMAT_R16_SINT;
			case gli::FORMAT_R16_SFLOAT_PACK16: return VK_FORMAT_R16_SFLOAT;

			case gli::FORMAT_RG16_UNORM_PACK16: return VK_FORMAT_R16G16_UNORM;
			case gli::FORMAT_RG16_SNORM_PACK16: return VK_FORMAT_R16G16_SNORM;
			case gli::FORMAT_RG16_USCALED_PACK16: return VK_FORMAT_R16G16_USCALED;
			case gli::FORMAT_RG16_SSCALED_PACK16: return VK_FORMAT_R16G16_SSCALED;
			case gli::FORMAT_RG16_UINT_PACK16: return VK_FORMAT_R16G16_UINT;
			case gli::FORMAT_RG16_SINT_PACK16: return VK_FORMAT_R16G16_SINT;
			case gli::FORMAT_RG16_SFLOAT_PACK16: return VK_FORMAT_R16G16_SFLOAT;

			case gli::FORMAT_RGB16_UNORM_PACK16: return VK_FORMAT_R16G16B16_UNORM;
			case gli::FORMAT_RGB16_SNORM_PACK16: return VK_FORMAT_R16G16B16_SNORM;
			case gli::FORMAT_RGB16_USCALED_PACK16: return VK_FORMAT_R16G16B16_USCALED;
			case gli::FORMAT_RGB16_SSCALED_PACK16: return VK_FORMAT_R16G16B16_SSCALED;
			case gli::FORMAT_RGB16_UINT_PACK16: return VK_FORMAT_R16G16B16_UINT;
			case gli::FORMAT_RGB16_SINT_PACK16: return VK_FORMAT_R16G16B16_SINT;
			case gli::FORMAT_RGB16_SFLOAT_PACK16: return VK_FORMAT_R16G16B16_SFLOAT;

			case gli::FORMAT_RGBA16_UNORM_PACK16: return VK_FORMAT_R16G16B16A16_UNORM;
			case gli::FORMAT_RGBA16_SNORM_PACK16: return VK_FORMAT_R16G16B16A16_SNORM;
			case gli::FORMAT_RGBA16_USCALED_PACK16: return VK_FORMAT_R16G16B16A16_USCALED;
			case gli::FORMAT_RGBA16_SSCALED_PACK16: return VK_FORMAT_R16G16B16A16_SSCALED;
			case gli::FORMAT_RGBA16_UINT_PACK16: return VK_FORMAT_R16G16B16A16_UINT;
			case gli::FORMAT_RGBA16_SINT_PACK16: return VK_FORMAT_R16G16B16A16_SINT;
			case gli::FORMAT_RGBA16_SFLOAT_PACK16: return VK_FORMAT_R16G16B16A16_SFLOAT;

				// 32-bit per component formats
			case gli::FORMAT_R32_UINT_PACK32: return VK_FORMAT_R32_UINT;
			case gli::FORMAT_R32_SINT_PACK32: return VK_FORMAT_R32_SINT;
			case gli::FORMAT_R32_SFLOAT_PACK32: return VK_FORMAT_R32_SFLOAT;

			case gli::FORMAT_RG32_UINT_PACK32: return VK_FORMAT_R32G32_UINT;
			case gli::FORMAT_RG32_SINT_PACK32: return VK_FORMAT_R32G32_SINT;
			case gli::FORMAT_RG32_SFLOAT_PACK32: return VK_FORMAT_R32G32_SFLOAT;

			case gli::FORMAT_RGB32_UINT_PACK32: return VK_FORMAT_R32G32B32_UINT;
			case gli::FORMAT_RGB32_SINT_PACK32: return VK_FORMAT_R32G32B32_SINT;
			case gli::FORMAT_RGB32_SFLOAT_PACK32: return VK_FORMAT_R32G32B32_SFLOAT;

			case gli::FORMAT_RGBA32_UINT_PACK32: return VK_FORMAT_R32G32B32A32_UINT;
			case gli::FORMAT_RGBA32_SINT_PACK32: return VK_FORMAT_R32G32B32A32_SINT;
			case gli::FORMAT_RGBA32_SFLOAT_PACK32: return VK_FORMAT_R32G32B32A32_SFLOAT;

				// Special formats
			case gli::FORMAT_RGB10A2_UNORM_PACK32: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
			case gli::FORMAT_RGB10A2_SNORM_PACK32: return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
			case gli::FORMAT_RGB10A2_USCALED_PACK32: return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
			case gli::FORMAT_RGB10A2_SSCALED_PACK32: return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
			case gli::FORMAT_RGB10A2_UINT_PACK32: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
			case gli::FORMAT_RGB10A2_SINT_PACK32: return VK_FORMAT_A2R10G10B10_SINT_PACK32;

			case gli::FORMAT_BGR10A2_UNORM_PACK32: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			case gli::FORMAT_BGR10A2_SNORM_PACK32: return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
			case gli::FORMAT_BGR10A2_USCALED_PACK32: return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
			case gli::FORMAT_BGR10A2_SSCALED_PACK32: return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
			case gli::FORMAT_BGR10A2_UINT_PACK32: return VK_FORMAT_A2B10G10R10_UINT_PACK32;
			case gli::FORMAT_BGR10A2_SINT_PACK32: return VK_FORMAT_A2B10G10R10_SINT_PACK32;

				// Depth formats
			case gli::FORMAT_D16_UNORM_PACK16: return VK_FORMAT_D16_UNORM;
			case gli::FORMAT_D24_UNORM_PACK32: return VK_FORMAT_D24_UNORM_S8_UINT;
			case gli::FORMAT_D32_SFLOAT_PACK32: return VK_FORMAT_D32_SFLOAT;
			case gli::FORMAT_S8_UINT_PACK8: return VK_FORMAT_S8_UINT;
			case gli::FORMAT_D16_UNORM_S8_UINT_PACK32: return VK_FORMAT_D16_UNORM_S8_UINT;
			case gli::FORMAT_D24_UNORM_S8_UINT_PACK32: return VK_FORMAT_D24_UNORM_S8_UINT;
			case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64: return VK_FORMAT_D32_SFLOAT_S8_UINT;

				// Compressed formats - DXT/BC
			case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return VK_FORMAT_BC2_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return VK_FORMAT_BC2_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return VK_FORMAT_BC3_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return VK_FORMAT_BC3_SRGB_BLOCK;

				// Compressed formats - BC7 (BPTC)
			case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return VK_FORMAT_BC7_UNORM_BLOCK;
			case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return VK_FORMAT_BC7_SRGB_BLOCK;

				// Compressed formats - ETC2
			case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
			case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;

				// Compressed formats - ASTC
			case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;

				// Compressed formats - PVRTC
			case gli::FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32: return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
			case gli::FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32: return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32: return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32: return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8: return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8: return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8: return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
			case gli::FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8: return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;

				// Special packed formats
			case gli::FORMAT_RG11B10_UFLOAT_PACK32: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
			case gli::FORMAT_RGB9E5_UFLOAT_PACK32: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

				// Luminance/Alpha formats
			case gli::FORMAT_L8_UNORM_PACK8: return VK_FORMAT_R8_UNORM; // Luminance maps to R
			case gli::FORMAT_A8_UNORM_PACK8: return VK_FORMAT_R8_UNORM; // Alpha maps to R
			case gli::FORMAT_LA8_UNORM_PACK8: return VK_FORMAT_R8G8_UNORM; // Luminance+Alpha maps to RG
			case gli::FORMAT_L16_UNORM_PACK16: return VK_FORMAT_R16_UNORM; // Luminance maps to R
			case gli::FORMAT_A16_UNORM_PACK16: return VK_FORMAT_R16_UNORM; // Alpha maps to R
			case gli::FORMAT_LA16_UNORM_PACK16: return VK_FORMAT_R16G16_UNORM; // Luminance+Alpha maps to RG

				// Default case for unsupported formats
			default:
				return VK_FORMAT_UNDEFINED;
		}
	}
}

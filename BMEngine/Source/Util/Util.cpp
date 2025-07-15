#include "Util.h"

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "Engine/Systems/Render/Render.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>

#include <SimpleIni/SimpleIni.h>

#include <filesystem>
#include <string>

struct VertexEqual
{
	bool operator()(const Render::StaticMeshVertex& lhs, const Render::StaticMeshVertex& rhs) const
	{
		return lhs.Position == rhs.Position && lhs.TextureCoords == rhs.TextureCoords;
	}
};

template<> struct std::hash<Render::StaticMeshVertex>
{
	size_t operator()(Render::StaticMeshVertex const& vertex) const
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
			case LogType::BMRVkLogType_Error:
			{
				std::cout << "\033[31;5mError: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				assert(false);
				break;
			}
			case LogType::BMRVkLogType_Warning:
			{
				std::cout << "\033[33;5mWarning: "; // Set red color
				vprintf(Format, Args);
				std::cout << "\n\033[m"; // Reset red color
				break;
			}
			case LogType::BMRVkLogType_Info:
			{
				std::cout << "Info: ";
				vprintf(Format, Args);
				std::cout << '\n';
				break;
			}
		}
	}

	void LoadPipelineSettings(VulkanHelper::PipelineSettings& Settings, const char* FilePath)
	{
		const std::string AbsolutePath = std::filesystem::absolute(FilePath).string();

		CSimpleIniA ini;
		ini.SetUnicode();
		SI_Error rc = ini.LoadFile(AbsolutePath.c_str());
		if (rc < 0)
		{
			assert(false);
		}

		const char* pipelineName = ini.GetValue("Pipeline", "PipelineName", "None");

		auto getString = [&](const char* key, const char* def)
		{
			return ini.GetValue("Pipeline", key, def);
		};

		auto getInt = [&](const char* key, int def)
		{
			return std::stoi(ini.GetValue("Pipeline", key, std::to_string(def).c_str()));
		};

		auto parseString = [&](const char* key, const char* defaultValue, auto parseFunc)
		{
			const char* value = getString(key, defaultValue);
			return parseFunc(value, strlen(value));
		};

		Settings.DepthClampEnable = parseString("DepthClampEnable", "false", VulkanHelper::ParseBool);
		Settings.RasterizerDiscardEnable = parseString("RasterizerDiscardEnable", "false", VulkanHelper::ParseBool);
		Settings.PolygonMode = parseString("PolygonMode", "fill", VulkanHelper::ParsePolygonMode);
		Settings.LineWidth = static_cast<float>(getInt("LineWidth", 1));
		Settings.CullMode = parseString("CullMode", "back", VulkanHelper::ParseCullMode);
		Settings.FrontFace = parseString("FrontFace", "counter_clockwise", VulkanHelper::ParseFrontFace);
		Settings.DepthBiasEnable = parseString("DepthBiasEnable", "false", VulkanHelper::ParseBool);
		Settings.BlendEnable = parseString("BlendEnable", "true", VulkanHelper::ParseBool);
		Settings.LogicOpEnable = parseString("LogicOpEnable", "false", VulkanHelper::ParseBool);
		Settings.AttachmentCount = getInt("AttachmentCount", 1);
		Settings.ColorWriteMask = parseString("ColorWriteMask", "RGBA", VulkanHelper::ParseColorWriteMask);
		Settings.SrcColorBlendFactor = parseString("SrcColorBlendFactor", "src_alpha", VulkanHelper::ParseBlendFactor);
		Settings.DstColorBlendFactor = parseString("DstColorBlendFactor", "one_minus_src_alpha", VulkanHelper::ParseBlendFactor);
		Settings.ColorBlendOp = parseString("ColorBlendOp", "add", VulkanHelper::ParseBlendOp);
		Settings.SrcAlphaBlendFactor = parseString("SrcAlphaBlendFactor", "one", VulkanHelper::ParseBlendFactor);
		Settings.DstAlphaBlendFactor = parseString("DstAlphaBlendFactor", "zero", VulkanHelper::ParseBlendFactor);
		Settings.AlphaBlendOp = parseString("AlphaBlendOp", "add", VulkanHelper::ParseBlendOp);
		Settings.DepthTestEnable = parseString("DepthTestEnable", "true", VulkanHelper::ParseBool);
		Settings.DepthWriteEnable = parseString("DepthWriteEnable", "true", VulkanHelper::ParseBool);
		Settings.DepthCompareOp = parseString("DepthCompareOp", "less", VulkanHelper::ParseCompareOp);
		Settings.DepthBoundsTestEnable = parseString("DepthBoundsTestEnable", "false", VulkanHelper::ParseBool);
		Settings.StencilTestEnable = parseString("StencilTestEnable", "false", VulkanHelper::ParseBool);
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

		u64* VerticesCounts = Memory::MemoryManagementSystem::Allocate<u64>(Shapes.size());
		u32* IndicesCounts = Memory::MemoryManagementSystem::Allocate<u32>(Shapes.size());

		std::unordered_map<Render::StaticMeshVertex, u32,
			std::hash<Render::StaticMeshVertex>, VertexEqual> uniqueVertices{ };

		std::hash<std::string> Hasher;

		std::vector<u64> TextureHashes(Shapes.size());
		std::vector<u8> VerticesAndIndices;

		std::vector<Render::StaticMeshVertex> Vertices;
		std::vector<u32> Indices;

		for (u32 i = 0; i < Shapes.size(); i++)
		{
			Vertices.clear();
			Indices.clear();

			const tinyobj::shape_t* Shape = Shapes.data() + i;

			for (u32 j = 0; j < Shape->mesh.indices.size(); j++)
			{
				tinyobj::index_t Index = Shape->mesh.indices[j];

				Render::StaticMeshVertex vertex = { };

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

			std::string diffuseTexName;
			if (!Materials.empty())
			{
				diffuseTexName = Materials[Shape->mesh.material_ids[0]].diffuse_texname;
			}

			fs::path texturePath = diffuseTexName;
			std::string FileNameNoExt = texturePath.stem().string();

			TextureHashes[i] = Hasher(FileNameNoExt);
			VerticesCounts[i] = Vertices.size();
			IndicesCounts[i] = Indices.size();

			u64 VertexBytes = Vertices.size() * sizeof(Render::StaticMeshVertex);
			u64 IndexBytes = Indices.size() * sizeof(u32);

			u64 CurrentOffset = VerticesAndIndices.size();
			VerticesAndIndices.resize(CurrentOffset + VertexBytes + IndexBytes);

			std::memcpy(VerticesAndIndices.data() + CurrentOffset, Vertices.data(), VertexBytes);
			std::memcpy(VerticesAndIndices.data() + CurrentOffset + VertexBytes, Indices.data(), IndexBytes);
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

		outFile.write(reinterpret_cast<const char*>(&Header), sizeof(Header));
		outFile.write(reinterpret_cast<const char*>(VerticesAndIndices.data()), Header.VertexDataSize);
		outFile.write(reinterpret_cast<const char*>(VerticesCounts), Header.MeshCount * sizeof(VerticesCounts[0]));
		outFile.write(reinterpret_cast<const char*>(IndicesCounts), Header.MeshCount * sizeof(IndicesCounts[0]));
		outFile.write(reinterpret_cast<const char*>(TextureHashes.data()), Header.MeshCount * sizeof(TextureHashes[0]));

		Memory::MemoryManagementSystem::Free(VerticesCounts);
		Memory::MemoryManagementSystem::Free(IndicesCounts);
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

		Model3DData Data = Memory::MemoryManagementSystem::Allocate<u8>(fileSize);
		file.read(reinterpret_cast<char*>(Data), fileSize);
		if (!file)
		{
			assert(false);
		}

		return Data;
	}

	void ClearModel3DData(Model3DData Data)
	{
		Memory::MemoryManagementSystem::Free(Data);
	}

	Model3D ParseModel3D(Model3DData Data)
	{
		Model3DFileHeader* Header = (Model3DFileHeader*)Data;
		Data += sizeof(Model3DFileHeader);

		Model3D Model;
		Model.MeshCount = Header->MeshCount;

		Model.VertexData = Data;
		Data += Header->VertexDataSize;

		Model.VerticesCounts = (u64*)Data;
		Data += Header->MeshCount * sizeof(u64);

		Model.IndicesCounts = (u32*)Data;
		Data += Header->MeshCount * sizeof(u32);

		Model.DiffuseTexturesHashes = (u64*)Data;

		return Model;
	}
}

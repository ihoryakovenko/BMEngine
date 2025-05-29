#include "Util.h"

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "Engine/Systems/Render/Render.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>

#include <Windows.h>
#include <shlwapi.h>

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
		_fseeki64(File, 0LL, static_cast<int>(SEEK_END));
		const long long FileSize = _ftelli64(File);
		if (FileSize != -1LL)
		{
			rewind(File); // Put file pointer to 0 index

			OutFileData.resize(static_cast<u64>(FileSize));
			// Need to check fread result if we know the size?
			const u64 ReadResult = fread(OutFileData.data(), 1, static_cast<u64>(FileSize), File);
			if (ReadResult == static_cast<u64>(FileSize))
			{
				return true;
			}

			return false;
		}

		return false;
	}

	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode)
	{
		FILE* File = nullptr;
		const int Result = fopen_s(&File, FileName, Mode);
		if (Result == 0)
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

		Log::Error("Cannot open file {}: Result = {}", FileName, Result);
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

	void LoadPipelineSettings(VulkanInterface::PipelineSettings& Settings, const char* FilePath)
	{
		// TODO: static
		static char buffer[256];

		GetPrivateProfileStringA("Pipeline", "PipelineName", "None", buffer, sizeof(buffer), FilePath);
		Settings.PipelineName = buffer;

		char PipelineFilePath[MAX_PATH];
		if (GetPrivateProfileStringA("Pipeline", "PipelineSettingsFile", FilePath,
			PipelineFilePath, sizeof(PipelineFilePath), FilePath) != 0)
		{
			FilePath = PipelineFilePath;
		}

		Settings.DepthClampEnable = GetPrivateProfileIntA("Pipeline", "DepthClampEnable", 0, FilePath);
		Settings.RasterizerDiscardEnable = GetPrivateProfileIntA("Pipeline", "RasterizerDiscardEnable", 0, FilePath);
		Settings.PolygonMode = (VkPolygonMode)GetPrivateProfileIntA("Pipeline", "PolygonMode", VK_POLYGON_MODE_FILL, FilePath);
		Settings.LineWidth = (float)GetPrivateProfileIntA("Pipeline", "LineWidth", 1, FilePath);
		Settings.CullMode = (VkCullModeFlags)GetPrivateProfileIntA("Pipeline", "CullMode", VK_CULL_MODE_BACK_BIT, FilePath);
		Settings.FrontFace = (VkFrontFace)GetPrivateProfileIntA("Pipeline", "FrontFace", VK_FRONT_FACE_COUNTER_CLOCKWISE, FilePath);
		Settings.DepthBiasEnable = GetPrivateProfileIntA("Pipeline", "DepthBiasEnable", 0, FilePath);
		Settings.BlendEnable = GetPrivateProfileIntA("Pipeline", "BlendEnable", 1, FilePath);
		Settings.LogicOpEnable = GetPrivateProfileIntA("Pipeline", "LogicOpEnable", 0, FilePath);
		Settings.AttachmentCount = GetPrivateProfileIntA("Pipeline", "AttachmentCount", 1, FilePath);
		Settings.ColorWriteMask = GetPrivateProfileIntA("Pipeline", "ColorWriteMask", VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, FilePath);
		Settings.SrcColorBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "SrcColorBlendFactor", VK_BLEND_FACTOR_SRC_ALPHA, FilePath);
		Settings.DstColorBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "DstColorBlendFactor", VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, FilePath);
		Settings.ColorBlendOp = (VkBlendOp)GetPrivateProfileIntA("Pipeline", "ColorBlendOp", VK_BLEND_OP_ADD, FilePath);
		Settings.SrcAlphaBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "SrcAlphaBlendFactor", VK_BLEND_FACTOR_ONE, FilePath);
		Settings.DstAlphaBlendFactor = (VkBlendFactor)GetPrivateProfileIntA("Pipeline", "DstAlphaBlendFactor", VK_BLEND_FACTOR_ZERO, FilePath);
		Settings.AlphaBlendOp = (VkBlendOp)GetPrivateProfileIntA("Pipeline", "AlphaBlendOp", VK_BLEND_OP_ADD, FilePath);
		Settings.DepthTestEnable = GetPrivateProfileIntA("Pipeline", "DepthTestEnable", 1, FilePath);
		Settings.DepthWriteEnable = GetPrivateProfileIntA("Pipeline", "DepthWriteEnable", 1, FilePath);
		Settings.DepthCompareOp = (VkCompareOp)GetPrivateProfileIntA("Pipeline", "DepthCompareOp", VK_COMPARE_OP_LESS, FilePath);
		Settings.DepthBoundsTestEnable = GetPrivateProfileIntA("Pipeline", "DepthBoundsTestEnable", 0, FilePath);
		Settings.StencilTestEnable = GetPrivateProfileIntA("Pipeline", "StencilTestEnable", 0, FilePath);
	}

	void ObjToModel3D(const char* FilePath, const char* OutputPath)
	{
		char BaseDir[MAX_PATH];
		strcpy(BaseDir, FilePath);
		PathRemoveFileSpecA(BaseDir);

		char NewAssetPath[MAX_PATH];
		strcpy(NewAssetPath, OutputPath);
		strcat(NewAssetPath, PathFindFileNameA(FilePath));
		PathRemoveExtensionA(NewAssetPath);
		strcat(NewAssetPath, ".model");

		tinyobj::attrib_t Attrib;
		std::vector<tinyobj::shape_t> Shapes;
		std::vector<tinyobj::material_t> Materials;
		std::string Warn, Err;

		if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, FilePath, BaseDir))
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

			char FileNameNoExt[MAX_PATH];
			strncpy(FileNameNoExt, Materials[Shape->mesh.material_ids[0]].diffuse_texname.c_str(), sizeof(FileNameNoExt));
			FileNameNoExt[sizeof(FileNameNoExt) - 1] = '\0';
			PathRemoveExtensionA(FileNameNoExt);

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

		HANDLE hFile = CreateFileA(NewAssetPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			assert(false);
			return;
		}

		DWORD BytesWritten;
		BOOL Result = 1;

		Model3DFileHeader Header;
		Header.MeshCount = Shapes.size();
		Header.VertexDataSize = VerticesAndIndices.size();

		Result &= WriteFile(hFile, &Header, sizeof(Header), &BytesWritten, NULL);
		Result &= WriteFile(hFile, VerticesAndIndices.data(), Header.VertexDataSize, &BytesWritten, NULL);
		Result &= WriteFile(hFile, VerticesCounts, Header.MeshCount * sizeof(VerticesCounts[0]), &BytesWritten, NULL);
		Result &= WriteFile(hFile, IndicesCounts, Header.MeshCount * sizeof(IndicesCounts[0]), &BytesWritten, NULL);
		Result &= WriteFile(hFile, TextureHashes.data(), Header.MeshCount * sizeof(TextureHashes[0]), &BytesWritten, NULL);

		CloseHandle(hFile);

		Memory::MemoryManagementSystem::Free(VerticesCounts);
		Memory::MemoryManagementSystem::Free(IndicesCounts);

		assert(Result);
	}

	Model3DData LoadModel3DData(const char* FilePath)
	{
		HANDLE hFile = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			assert(false);
		}

		LARGE_INTEGER FileSize;
		if (!GetFileSizeEx(hFile, &FileSize))
		{
			assert(false);
		}

		Model3DData Data = Memory::MemoryManagementSystem::Allocate<u8>(FileSize.QuadPart);

		DWORD BytesRead;
		BOOL Result = ReadFile(hFile, Data, FileSize.QuadPart, &BytesRead, NULL);
		assert(Result);

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

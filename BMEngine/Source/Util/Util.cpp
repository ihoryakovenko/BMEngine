#include "Util.h"

#include <mini-yaml/yaml/Yaml.hpp>

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "Engine/Systems/Render/Render.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>

#include <filesystem>
#include <string>
#include <cstdarg>

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
	static VkShaderStageFlagBits ParseShaderStage(const char* Value, u32 Length)
	{
		if ((strncmp(Value, "vertex", Length) == 0) || (strncmp(Value, "VERTEX", Length) == 0))
			return VK_SHADER_STAGE_VERTEX_BIT;
		if ((strncmp(Value, "fragment", Length) == 0) || (strncmp(Value, "FRAGMENT", Length) == 0))
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		if ((strncmp(Value, "geometry", Length) == 0) || (strncmp(Value, "GEOMETRY", Length) == 0))
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		if ((strncmp(Value, "compute", Length) == 0) || (strncmp(Value, "COMPUTE", Length) == 0))
			return VK_SHADER_STAGE_COMPUTE_BIT;
		if ((strncmp(Value, "tess_control", Length) == 0) || (strncmp(Value, "TESS_CONTROL", Length) == 0))
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if ((strncmp(Value, "tess_eval", Length) == 0) || (strncmp(Value, "TESS_EVAL", Length) == 0))
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if ((strncmp(Value, "task", Length) == 0) || (strncmp(Value, "TASK", Length) == 0))
			return VK_SHADER_STAGE_TASK_BIT_EXT;
		if ((strncmp(Value, "mesh", Length) == 0) || (strncmp(Value, "MESH", Length) == 0))
			return VK_SHADER_STAGE_MESH_BIT_EXT;
		if ((strncmp(Value, "raygen", Length) == 0) || (strncmp(Value, "RAYGEN", Length) == 0))
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if ((strncmp(Value, "closest_hit", Length) == 0) || (strncmp(Value, "CLOSEST_HIT", Length) == 0))
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if ((strncmp(Value, "any_hit", Length) == 0) || (strncmp(Value, "ANY_HIT", Length) == 0))
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if ((strncmp(Value, "miss", Length) == 0) || (strncmp(Value, "MISS", Length) == 0))
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		if ((strncmp(Value, "intersection", Length) == 0) || (strncmp(Value, "INTERSECTION", Length) == 0))
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if ((strncmp(Value, "callable", Length) == 0) || (strncmp(Value, "CALLABLE", Length) == 0))
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;

		assert(false);
		return VK_SHADER_STAGE_VERTEX_BIT;
	}

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

	void LoadPipelineSettingsYAML(VulkanHelper::PipelineSettings& Settings, const char* FilePath)
	{
		const std::string AbsolutePath = std::filesystem::absolute(FilePath).string();

		Yaml::Node Root;
		Yaml::Parse(Root, AbsolutePath.c_str());

		Settings.Shaders = Memory::AllocateArray<VulkanHelper::Shader>(1);

		if (!Root["shaders"].IsNone())
		{
			Yaml::Node& Shaders = Root["shaders"];
			for (auto it = Shaders.Begin(); it != Shaders.End(); it++)
			{
				std::cout << (*it).first << ": " << (*it).second.As<std::string>() << std::endl;
				// TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				std::vector<char> VertexShaderCode;
				Util::OpenAndReadFileFull((*it).second.As<std::string>().c_str(), VertexShaderCode, "rb");

				VulkanHelper::Shader* NewShader = Memory::ArrayGetNew(&Settings.Shaders);

				// TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				NewShader->ShaderCode = Memory::AllocateArray<char>(VertexShaderCode.size());
				for (u32 i = 0; i < VertexShaderCode.size(); ++i)
				{
					Memory::PushBackToArray(&NewShader->ShaderCode, VertexShaderCode.data() + i);
				}

				NewShader->Stage = ParseShaderStage((*it).first.c_str(), (*it).first.size()); // TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				NewShader->Code = NewShader->ShaderCode.Data; // TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				NewShader->CodeSize = NewShader->ShaderCode.Count; // TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			}
		}

		Settings.RasterizationState = {};
		Settings.RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		
		if (!Root["rasterization"].IsNone())
		{
			Yaml::Node& rasterization = Root["rasterization"];
			Settings.RasterizationState.depthClampEnable = rasterization["depthClampEnable"].As<bool>();
			Settings.RasterizationState.rasterizerDiscardEnable = rasterization["rasterizerDiscardEnable"].As<bool>();
			std::string polygonModeStr = rasterization["polygonMode"].As<std::string>();
			Settings.RasterizationState.polygonMode = VulkanHelper::ParsePolygonMode(polygonModeStr.c_str(), polygonModeStr.length());
			Settings.RasterizationState.lineWidth = static_cast<float>(rasterization["lineWidth"].As<int>());
			std::string cullModeStr = rasterization["cullMode"].As<std::string>();
			Settings.RasterizationState.cullMode = VulkanHelper::ParseCullMode(cullModeStr.c_str(), cullModeStr.length());
			std::string frontFaceStr = rasterization["frontFace"].As<std::string>();
			Settings.RasterizationState.frontFace = VulkanHelper::ParseFrontFace(frontFaceStr.c_str(), frontFaceStr.length());
			Settings.RasterizationState.depthBiasEnable = rasterization["depthBiasEnable"].As<bool>();
		}
		
		Settings.ColorBlendState = {};
		Settings.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		Settings.ColorBlendState.pAttachments = &Settings.ColorBlendAttachment;
		
		Settings.ColorBlendAttachment = {};
		
		if (!Root["colorBlend"].IsNone())
		{
			Yaml::Node& colorBlend = Root["colorBlend"];
			Settings.ColorBlendState.logicOpEnable = colorBlend["logicOpEnable"].As<bool>();
			Settings.ColorBlendState.attachmentCount = colorBlend["attachmentCount"].As<int>();
			std::string colorWriteMaskStr = colorBlend["colorWriteMask"].As<std::string>();
			Settings.ColorBlendAttachment.colorWriteMask = VulkanHelper::ParseColorWriteMask(colorWriteMaskStr.c_str(), colorWriteMaskStr.length());
			Settings.ColorBlendAttachment.blendEnable = colorBlend["blendEnable"].As<bool>();
			std::string srcColorBlendFactorStr = colorBlend["srcColorBlendFactor"].As<std::string>();
			Settings.ColorBlendAttachment.srcColorBlendFactor = VulkanHelper::ParseBlendFactor(srcColorBlendFactorStr.c_str(), srcColorBlendFactorStr.length());
			std::string dstColorBlendFactorStr = colorBlend["dstColorBlendFactor"].As<std::string>();
			Settings.ColorBlendAttachment.dstColorBlendFactor = VulkanHelper::ParseBlendFactor(dstColorBlendFactorStr.c_str(), dstColorBlendFactorStr.length());
			std::string colorBlendOpStr = colorBlend["colorBlendOp"].As<std::string>();
			Settings.ColorBlendAttachment.colorBlendOp = VulkanHelper::ParseBlendOp(colorBlendOpStr.c_str(), colorBlendOpStr.length());
			std::string srcAlphaBlendFactorStr = colorBlend["srcAlphaBlendFactor"].As<std::string>();
			Settings.ColorBlendAttachment.srcAlphaBlendFactor = VulkanHelper::ParseBlendFactor(srcAlphaBlendFactorStr.c_str(), srcAlphaBlendFactorStr.length());
			std::string dstAlphaBlendFactorStr = colorBlend["dstAlphaBlendFactor"].As<std::string>();
			Settings.ColorBlendAttachment.dstAlphaBlendFactor = VulkanHelper::ParseBlendFactor(dstAlphaBlendFactorStr.c_str(), dstAlphaBlendFactorStr.length());
			std::string alphaBlendOpStr = colorBlend["alphaBlendOp"].As<std::string>();
			Settings.ColorBlendAttachment.alphaBlendOp = VulkanHelper::ParseBlendOp(alphaBlendOpStr.c_str(), alphaBlendOpStr.length());
		}
		
		Settings.DepthStencilState = {};
		Settings.DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		
		if (!Root["depthStencil"].IsNone())
		{
			Yaml::Node& depthStencil = Root["depthStencil"];
			Settings.DepthStencilState.depthTestEnable = depthStencil["depthTestEnable"].As<bool>();
			Settings.DepthStencilState.depthWriteEnable = depthStencil["depthWriteEnable"].As<bool>();
			std::string depthCompareOpStr = depthStencil["depthCompareOp"].As<std::string>();
			Settings.DepthStencilState.depthCompareOp = VulkanHelper::ParseCompareOp(depthCompareOpStr.c_str(), depthCompareOpStr.length());
			Settings.DepthStencilState.depthBoundsTestEnable = depthStencil["depthBoundsTestEnable"].As<bool>();
			Settings.DepthStencilState.stencilTestEnable = depthStencil["stencilTestEnable"].As<bool>();
		}
		
		Settings.MultisampleState = {};
		Settings.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		
		if (!Root["multisample"].IsNone())
		{
			Yaml::Node& multisample = Root["multisample"];
			Settings.MultisampleState.sampleShadingEnable = multisample["sampleShadingEnable"].As<bool>();
			std::string rasterizationSamplesStr = multisample["rasterizationSamples"].As<std::string>();
			Settings.MultisampleState.rasterizationSamples = VulkanHelper::ParseSampleCount(rasterizationSamplesStr.c_str(), rasterizationSamplesStr.length());
		}
		
		Settings.InputAssemblyState = {};
		Settings.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		
		if (!Root["inputAssembly"].IsNone())
		{
			Yaml::Node& inputAssembly = Root["inputAssembly"];
			std::string topologyStr = inputAssembly["topology"].As<std::string>();
			Settings.InputAssemblyState.topology = VulkanHelper::ParseTopology(topologyStr.c_str(), topologyStr.length());
			Settings.InputAssemblyState.primitiveRestartEnable = inputAssembly["primitiveRestartEnable"].As<bool>();
		}
		
		Settings.ViewportState = {};
		Settings.ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		Settings.ViewportState.pViewports = &Settings.Viewport;
		Settings.ViewportState.pScissors = &Settings.Scissor;

		Settings.Viewport = {};
		Settings.Scissor = {};
		Settings.Scissor.offset = {};
		
		if (!Root["viewport"].IsNone())
		{
			Yaml::Node& viewport = Root["viewport"];
			Settings.ViewportState.viewportCount = viewport["viewportCount"].As<int>();
			Settings.ViewportState.scissorCount = viewport["scissorCount"].As<int>();
			Settings.Viewport.minDepth = viewport["minDepth"].As<float>();
			Settings.Viewport.maxDepth = viewport["maxDepth"].As<float>();
			Settings.Viewport.x = viewport["x"].As<float>();
			Settings.Viewport.y = viewport["y"].As<float>();
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

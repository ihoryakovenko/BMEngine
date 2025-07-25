#include "Util.h"

#include <mini-yaml/yaml/Yaml.hpp>

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/RenderResources.h"

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

		Yaml::Node Root;
		Yaml::Parse(Root, AbsolutePath.c_str());

		Yaml::Node& PipelineNode = Root["pipeline"];

		Settings.Shaders = Memory::AllocateArray<VkPipelineShaderStageCreateInfo>(1);

		if (!PipelineNode["shaders"].IsNone())
		{
			Yaml::Node& Shaders = PipelineNode["shaders"];
			for (auto it = Shaders.Begin(); it != Shaders.End(); it++)
			{
				std::cout << (*it).first << ": " << (*it).second.As<std::string>() << std::endl;
				VkPipelineShaderStageCreateInfo* NewShaderStage = Memory::ArrayGetNew(&Settings.Shaders);
				*NewShaderStage = { };
				NewShaderStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				NewShaderStage->stage = VulkanHelper::ParseShaderStage((*it).first.c_str(), (*it).first.length());
				NewShaderStage->pName = "main";
				NewShaderStage->module = RenderResources::GetShader((*it).second.As<std::string>());
			}
		}

		Settings.RasterizationState = {};
		Settings.RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		
		if (!PipelineNode["rasterization"].IsNone())
		{
			Yaml::Node& rasterization = PipelineNode["rasterization"];
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
		
		if (!PipelineNode["colorBlend"].IsNone())
		{
			Yaml::Node& colorBlend = PipelineNode["colorBlend"];
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
		
		if (!PipelineNode["depthStencil"].IsNone())
		{
			Yaml::Node& depthStencil = PipelineNode["depthStencil"];
			Settings.DepthStencilState.depthTestEnable = depthStencil["depthTestEnable"].As<bool>();
			Settings.DepthStencilState.depthWriteEnable = depthStencil["depthWriteEnable"].As<bool>();
			std::string depthCompareOpStr = depthStencil["depthCompareOp"].As<std::string>();
			Settings.DepthStencilState.depthCompareOp = VulkanHelper::ParseCompareOp(depthCompareOpStr.c_str(), depthCompareOpStr.length());
			Settings.DepthStencilState.depthBoundsTestEnable = depthStencil["depthBoundsTestEnable"].As<bool>();
			Settings.DepthStencilState.stencilTestEnable = depthStencil["stencilTestEnable"].As<bool>();
		}
		
		Settings.MultisampleState = {};
		Settings.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		
		if (!PipelineNode["multisample"].IsNone())
		{
			Yaml::Node& multisample = PipelineNode["multisample"];
			Settings.MultisampleState.sampleShadingEnable = multisample["sampleShadingEnable"].As<bool>();
			std::string rasterizationSamplesStr = multisample["rasterizationSamples"].As<std::string>();
			Settings.MultisampleState.rasterizationSamples = VulkanHelper::ParseSampleCount(rasterizationSamplesStr.c_str(), rasterizationSamplesStr.length());
		}
		
		Settings.InputAssemblyState = {};
		Settings.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		
		if (!PipelineNode["inputAssembly"].IsNone())
		{
			Yaml::Node& inputAssembly = PipelineNode["inputAssembly"];
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
		
		if (!PipelineNode["viewport"].IsNone())
		{
			Yaml::Node& viewport = PipelineNode["viewport"];
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

	Yaml::Node& GetSamplers(Yaml::Node& Root)
	{
		if (!Root["samplers"].IsNone())
		{
			return Root["samplers"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseSamplerNode(Yaml::Node& Sampler, VkSamplerCreateInfo* OutSamplerCreateInfo)
	{
		*OutSamplerCreateInfo = { };
		OutSamplerCreateInfo->sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		OutSamplerCreateInfo->pNext = nullptr;
		OutSamplerCreateInfo->flags = 0;

		std::string Value;

		if (!Sampler["magFilter"].IsNone())
		{
			Value = Sampler["magFilter"].As<std::string>();
			OutSamplerCreateInfo->magFilter = VulkanHelper::ParseFilter(Value.c_str(), Value.length());
		}
		if (!Sampler["minFilter"].IsNone())
		{
			Value = Sampler["minFilter"].As<std::string>();
			OutSamplerCreateInfo->minFilter = VulkanHelper::ParseFilter(Value.c_str(), Value.length());
		}

		if (!Sampler["addressModeU"].IsNone())
		{
			Value = Sampler["addressModeU"].As<std::string>();
			OutSamplerCreateInfo->addressModeU = VulkanHelper::ParseAddressMode(Value.c_str(), Value.length());
		}
		if (!Sampler["addressModeV"].IsNone())
		{
			Value = Sampler["addressModeV"].As<std::string>();
			OutSamplerCreateInfo->addressModeV = VulkanHelper::ParseAddressMode(Value.c_str(), Value.length());
		}
		if (!Sampler["addressModeW"].IsNone())
		{
			Value = Sampler["addressModeW"].As<std::string>();
			OutSamplerCreateInfo->addressModeW = VulkanHelper::ParseAddressMode(Value.c_str(), Value.length());
		}

		if (!Sampler["borderColor"].IsNone())
		{
			Value = Sampler["borderColor"].As<std::string>();
			OutSamplerCreateInfo->borderColor = VulkanHelper::ParseBorderColor(Value.c_str(), Value.length());
		}

		if (!Sampler["unnormalizedCoordinates"].IsNone())
		{
			OutSamplerCreateInfo->unnormalizedCoordinates = Sampler["unnormalizedCoordinates"].As<bool>() ? VK_TRUE : VK_FALSE;
		}
		if (!Sampler["anisotropyEnable"].IsNone())
		{
			OutSamplerCreateInfo->anisotropyEnable = Sampler["anisotropyEnable"].As<bool>() ? VK_TRUE : VK_FALSE;
		}
		if (!Sampler["compareEnable"].IsNone())
		{
			OutSamplerCreateInfo->compareEnable = Sampler["compareEnable"].As<bool>() ? VK_TRUE : VK_FALSE;
		}

		if (!Sampler["mipmapMode"].IsNone())
		{
			Value = Sampler["mipmapMode"].As<std::string>();
			OutSamplerCreateInfo->mipmapMode = VulkanHelper::ParseMipmapMode(Value.c_str(), Value.length());
		}

		if (!Sampler["mipLodBias"].IsNone())
		{
			OutSamplerCreateInfo->mipLodBias = Sampler["mipLodBias"].As<float>();
		}
		if (!Sampler["minLod"].IsNone())
		{
			OutSamplerCreateInfo->minLod = Sampler["minLod"].As<float>();
		}
		if (!Sampler["maxLod"].IsNone())
		{
			OutSamplerCreateInfo->maxLod = Sampler["maxLod"].As<float>();
		}
		if (!Sampler["maxAnisotropy"].IsNone())
		{
			OutSamplerCreateInfo->maxAnisotropy = Sampler["maxAnisotropy"].As<float>();
		}
	}

	Yaml::Node& GetShaders(Yaml::Node& Root)
	{
		if (!Root["shaders"].IsNone())
		{
			return Root["shaders"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseShaderNode(Yaml::Node& ShaderNode, std::string* OutShaderPath)
	{
		*OutShaderPath = ShaderNode.As<std::string>();
	}

	Yaml::Node& GetDescriptorSetLayouts(Yaml::Node& Root)
	{
		if (!Root["DescriptorSetLayouts"].IsNone())
		{
			return Root["DescriptorSetLayouts"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& ParseDescriptorSetLayoutNode(Yaml::Node& Root)
	{
		if (!Root["bindings"].IsNone())
		{
			return Root["bindings"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode, VkDescriptorSetLayoutBinding* OutBinding)
	{
		*OutBinding = { };
		OutBinding->pImmutableSamplers = nullptr;

		if (!BindingNode["binding"].IsNone())
		{
			OutBinding->binding = BindingNode["binding"].As<u32>();
		}
		
		if (!BindingNode["descriptorType"].IsNone())
		{
			std::string value = BindingNode["descriptorType"].As<std::string>();
			OutBinding->descriptorType = VulkanHelper::ParseDescriptorType(value.c_str(), value.length());
		}
		
		if (!BindingNode["descriptorCount"].IsNone())
		{
			OutBinding->descriptorCount = BindingNode["descriptorCount"].As<u32>();
		}
		
		if (!BindingNode["stageFlags"].IsNone())
		{
			std::string value = BindingNode["stageFlags"].As<std::string>();
			OutBinding->stageFlags = VulkanHelper::ParseShaderStageFlags(value.c_str(), value.length());
		}
	}

	Yaml::Node& GetVertices(Yaml::Node& Root)
	{
		if (!Root["vertices"].IsNone())
		{
			return Root["vertices"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetVertexBindingNode(Yaml::Node& VertexNode)
	{
		if (!VertexNode["binding"].IsNone())
		{
			return VertexNode["binding"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetVertexAttributesNode(Yaml::Node& VertexNode)
	{
		if (!VertexNode["attributes"].IsNone())
		{
			return VertexNode["attributes"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetVertexAttributeFormatNode(Yaml::Node& AttributeNode)
	{
		if (!AttributeNode["format"].IsNone())
		{
			return AttributeNode["format"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseVertexAttributeNode(Yaml::Node& AttributeNode, RenderResources::VertexAttribute* OutAttribute)
	{
		*OutAttribute = { };

		if (!AttributeNode["format"].IsNone())
		{
			std::string formatStr = AttributeNode["format"].As<std::string>();
			OutAttribute->Format = VulkanHelper::ParseFormat(formatStr.c_str(), formatStr.length());
		}
	}

	void ParseVertexBindingNode(Yaml::Node& BindingNode, RenderResources::VertexBinding* OutBinding)
	{
		*OutBinding = { };

		if (!BindingNode["inputRate"].IsNone())
		{
			std::string inputRateStr = BindingNode["inputRate"].As<std::string>();
			OutBinding->InputRate = VulkanHelper::ParseVertexInputRate(inputRateStr.c_str(), inputRateStr.length());
		}
	}
}

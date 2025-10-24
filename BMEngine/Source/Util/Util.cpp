#include "Util.h"

#include <mini-yaml/yaml/Yaml.hpp>
#include <vector>
#include <algorithm>

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
FORGE_MEMORY_DEBUG
#include "Engine/Systems/Memory/forge_memory_debugger.h"

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Engine/Systems/EngineResources.h"
#include "gli/gli.hpp"

// Extern declarations for global resource maps
extern std::unordered_map<std::string, VertexBinding_depr> VBindings;
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
	template<size_t N>
	bool StringMatches(const char* Value, u32 Length, const char* const (&Strings)[N])
	{
		for (size_t i = 0; i < N; ++i)
		{
			if (strncmp(Value, Strings[i], Length) == 0)
			{
				return true;
			}
		}
		return false;
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

	std::string ParseNameNode(Yaml::Node& Node)
	{
		if (!Node["name"].IsNone())
		{
			return Node["name"].As<std::string>();
		}
		return {};
	}

	BmRHI_SamplerDescription ParseSamplerNode(Yaml::Node& Sampler)
	{
		BmRHI_SamplerDescription Data = { };

		std::string Value;

		if (!Sampler["magFilter"].IsNone())
		{
			Value = Sampler["magFilter"].As<std::string>();
			Data.MagFilter = ParseFilter(Value.c_str(), Value.length());
		}
		if (!Sampler["minFilter"].IsNone())
		{
			Value = Sampler["minFilter"].As<std::string>();
			Data.MinFilter = ParseFilter(Value.c_str(), Value.length());
		}

		if (!Sampler["addressModeU"].IsNone())
		{
			Value = Sampler["addressModeU"].As<std::string>();
			Data.AddressModeU = ParseAddressMode(Value.c_str(), Value.length());
		}
		if (!Sampler["addressModeV"].IsNone())
		{
			Value = Sampler["addressModeV"].As<std::string>();
			Data.AddressModeV = ParseAddressMode(Value.c_str(), Value.length());
		}
		if (!Sampler["addressModeW"].IsNone())
		{
			Value = Sampler["addressModeW"].As<std::string>();
			Data.AddressModeW = ParseAddressMode(Value.c_str(), Value.length());
		}

		if (!Sampler["borderColor"].IsNone())
		{
			Value = Sampler["borderColor"].As<std::string>();
			Data.BorderColor = ParseBorderColor(Value.c_str(), Value.length());
		}

		if (!Sampler["unnormalizedCoordinates"].IsNone())
		{
			Data.UnnormalizedCoordinates = Sampler["unnormalizedCoordinates"].As<bool>() ? VK_TRUE : VK_FALSE;
		}
		if (!Sampler["anisotropyEnable"].IsNone())
		{
			Data.AnisotropyEnable = Sampler["anisotropyEnable"].As<bool>() ? VK_TRUE : VK_FALSE;
		}
		if (!Sampler["compareEnable"].IsNone())
		{
			Data.CompareEnable = Sampler["compareEnable"].As<bool>() ? VK_TRUE : VK_FALSE;
		}

		if (!Sampler["mipmapMode"].IsNone())
		{
			Value = Sampler["mipmapMode"].As<std::string>();
			Data.MipmapMode = ParseMipmapMode(Value.c_str(), Value.length());
		}

		if (!Sampler["mipLodBias"].IsNone())
		{
			Data.MipLodBias = Sampler["mipLodBias"].As<f32>();
		}
		if (!Sampler["minLod"].IsNone())
		{
			Data.MinLod = Sampler["minLod"].As<f32>();
		}
		if (!Sampler["maxLod"].IsNone())
		{
			Data.MaxLod = Sampler["maxLod"].As<f32>();
		}
		if (!Sampler["maxAnisotropy"].IsNone())
		{
			Data.MaxAnisotropy = Sampler["maxAnisotropy"].As<f32>();
		}
		
		return Data;
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

	std::string ParseShaderNode(Yaml::Node& ShaderNode)
	{
		return ShaderNode.As<std::string>();
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

	BmRender_LayoutBinding ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode)
	{
		BmRender_LayoutBinding OutBinding = { };		
		if (!BindingNode["descriptorType"].IsNone())
		{
			std::string value = BindingNode["descriptorType"].As<std::string>();
			OutBinding.DescriptorType = ParseDescriptorType(value.c_str(), value.length());
		}
		
		if (!BindingNode["descriptorCount"].IsNone())
		{
			OutBinding.DescriptorCount = BindingNode["descriptorCount"].As<u32>();
		}
		
		if (!BindingNode["stageFlags"].IsNone())
		{
			std::string value = BindingNode["stageFlags"].As<std::string>();
			OutBinding.StageFlags = ParseShaderStageFlags(value.c_str(), value.length());
		}
		
		return OutBinding;
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

	Yaml::Node& GetVertexAttributeTypeNode(Yaml::Node& AttributeNode)
	{
		if (!AttributeNode["type"].IsNone())
		{
			return AttributeNode["type"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseVertexAttributeNode(Yaml::Node& AttributeNode, VertexAttribute* OutAttribute, std::string* OutAttributeName)
	{
		*OutAttribute = { };

		if (!AttributeNode["type"].IsNone())
		{
			std::string typeStr = AttributeNode["type"].As<std::string>();
			OutAttribute->Type = ParseShaderTypeToAttributeType(typeStr.c_str(), typeStr.length());
		}

		*OutAttributeName = ParseNameNode(AttributeNode);
	}

	VertexBinding_depr ParseVertexBindingNode(Yaml::Node& BindingNode)
	{
		VertexBinding_depr OutBinding = { };

		if (!BindingNode["inputRate"].IsNone())
		{
			std::string inputRateStr = BindingNode["inputRate"].As<std::string>();
			OutBinding.InputRate = ParseVertexInputRate(inputRateStr.c_str(), inputRateStr.length());
		}
		
		return OutBinding;
	}

	Yaml::Node& GetPipelineNode(Yaml::Node& Root)
	{
		if (!Root["pipeline"].IsNone())
		{
			return Root["pipeline"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineShadersNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["shaders"].IsNone())
		{
			return PipelineNode["shaders"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineRasterizationNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["rasterization"].IsNone())
		{
			return PipelineNode["rasterization"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineColorBlendStateNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["colorBlendState"].IsNone())
		{
			return PipelineNode["colorBlendState"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineColorBlendAttachmentNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["colorBlendAttachment"].IsNone())
		{
			return PipelineNode["colorBlendAttachment"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineDepthStencilNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["depthStencil"].IsNone())
		{
			return PipelineNode["depthStencil"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineMultisampleNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["multisample"].IsNone())
		{
			return PipelineNode["multisample"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineInputAssemblyNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["inputAssembly"].IsNone())
		{
			return PipelineNode["inputAssembly"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}


	Yaml::Node& GetVertexAttributeLayoutNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["VertexAttributeLayout"].IsNone())
		{
			return PipelineNode["VertexAttributeLayout"];
		}

		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPipelineLayoutNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["PipelineLayout"].IsNone())
		{
			return PipelineNode["PipelineLayout"];
		}
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	VkPipelineRasterizationStateCreateInfo ParsePipelineRasterizationNode(Yaml::Node& RasterizationNode)
	{
		VkPipelineRasterizationStateCreateInfo OutRasterizationState = {};
		OutRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		
		if (!RasterizationNode["depthClampEnable"].IsNone())
			OutRasterizationState.depthClampEnable = RasterizationNode["depthClampEnable"].As<bool>();
		if (!RasterizationNode["rasterizerDiscardEnable"].IsNone())
			OutRasterizationState.rasterizerDiscardEnable = RasterizationNode["rasterizerDiscardEnable"].As<bool>();
		if (!RasterizationNode["polygonMode"].IsNone())
		{
			std::string polygonModeStr = RasterizationNode["polygonMode"].As<std::string>();
			OutRasterizationState.polygonMode = ParsePolygonMode(polygonModeStr.c_str(), polygonModeStr.length());
		}
		if (!RasterizationNode["lineWidth"].IsNone())
			OutRasterizationState.lineWidth = RasterizationNode["lineWidth"].As<u32>();
		if (!RasterizationNode["cullMode"].IsNone())
		{
			std::string cullModeStr = RasterizationNode["cullMode"].As<std::string>();
			OutRasterizationState.cullMode = ParseCullMode(cullModeStr.c_str(), cullModeStr.length());
		}
		if (!RasterizationNode["frontFace"].IsNone())
		{
			std::string frontFaceStr = RasterizationNode["frontFace"].As<std::string>();
			OutRasterizationState.frontFace = ParseFrontFace(frontFaceStr.c_str(), frontFaceStr.length());
		}
		if (!RasterizationNode["depthBiasEnable"].IsNone())
			OutRasterizationState.depthBiasEnable = RasterizationNode["depthBiasEnable"].As<bool>();
	
		return OutRasterizationState;
	}

	VkPipelineColorBlendStateCreateInfo ParsePipelineColorBlendStateNode(Yaml::Node& ColorBlendStateNode)
	{
		VkPipelineColorBlendStateCreateInfo OutColorBlendState = {};
		OutColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	
		if (!ColorBlendStateNode["logicOpEnable"].IsNone())
			OutColorBlendState.logicOpEnable = ColorBlendStateNode["logicOpEnable"].As<bool>();
		if (!ColorBlendStateNode["attachmentCount"].IsNone())
			OutColorBlendState.attachmentCount = ColorBlendStateNode["attachmentCount"].As<u32>();
	
		return OutColorBlendState;
	}

	VkPipelineColorBlendAttachmentState ParsePipelineColorBlendAttachmentNode(Yaml::Node& ColorBlendAttachmentNode)
	{
		VkPipelineColorBlendAttachmentState OutColorBlendAttachment = {};
	
		if (!ColorBlendAttachmentNode["colorWriteMask"].IsNone())
		{
			std::string colorWriteMaskStr = ColorBlendAttachmentNode["colorWriteMask"].As<std::string>();
			OutColorBlendAttachment.colorWriteMask = ParseColorWriteMask(colorWriteMaskStr.c_str(), colorWriteMaskStr.length());
		}
		if (!ColorBlendAttachmentNode["blendEnable"].IsNone())
			OutColorBlendAttachment.blendEnable = ColorBlendAttachmentNode["blendEnable"].As<bool>();
		if (!ColorBlendAttachmentNode["srcColorBlendFactor"].IsNone())
		{
			std::string srcColorBlendFactorStr = ColorBlendAttachmentNode["srcColorBlendFactor"].As<std::string>();
			OutColorBlendAttachment.srcColorBlendFactor = ParseBlendFactor(srcColorBlendFactorStr.c_str(), srcColorBlendFactorStr.length());
		}
		if (!ColorBlendAttachmentNode["dstColorBlendFactor"].IsNone())
		{
			std::string dstColorBlendFactorStr = ColorBlendAttachmentNode["dstColorBlendFactor"].As<std::string>();
			OutColorBlendAttachment.dstColorBlendFactor = ParseBlendFactor(dstColorBlendFactorStr.c_str(), dstColorBlendFactorStr.length());
		}
		if (!ColorBlendAttachmentNode["colorBlendOp"].IsNone())
		{
			std::string colorBlendOpStr = ColorBlendAttachmentNode["colorBlendOp"].As<std::string>();
			OutColorBlendAttachment.colorBlendOp = ParseBlendOp(colorBlendOpStr.c_str(), colorBlendOpStr.length());
		}
		if (!ColorBlendAttachmentNode["srcAlphaBlendFactor"].IsNone())
		{
			std::string srcAlphaBlendFactorStr = ColorBlendAttachmentNode["srcAlphaBlendFactor"].As<std::string>();
			OutColorBlendAttachment.srcAlphaBlendFactor = ParseBlendFactor(srcAlphaBlendFactorStr.c_str(), srcAlphaBlendFactorStr.length());
		}
		if (!ColorBlendAttachmentNode["dstAlphaBlendFactor"].IsNone())
		{
			std::string dstAlphaBlendFactorStr = ColorBlendAttachmentNode["dstAlphaBlendFactor"].As<std::string>();
			OutColorBlendAttachment.dstAlphaBlendFactor = ParseBlendFactor(dstAlphaBlendFactorStr.c_str(), dstAlphaBlendFactorStr.length());
		}
		if (!ColorBlendAttachmentNode["alphaBlendOp"].IsNone())
		{
			std::string alphaBlendOpStr = ColorBlendAttachmentNode["alphaBlendOp"].As<std::string>();
			OutColorBlendAttachment.alphaBlendOp = ParseBlendOp(alphaBlendOpStr.c_str(), alphaBlendOpStr.length());
		}
		
		return OutColorBlendAttachment;
	}

	VkPipelineDepthStencilStateCreateInfo ParsePipelineDepthStencilNode(Yaml::Node& DepthStencilNode)
	{
		VkPipelineDepthStencilStateCreateInfo OutDepthStencilState = {};
		OutDepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		
		if (!DepthStencilNode["depthTestEnable"].IsNone())
			OutDepthStencilState.depthTestEnable = DepthStencilNode["depthTestEnable"].As<bool>();
		if (!DepthStencilNode["depthWriteEnable"].IsNone())
			OutDepthStencilState.depthWriteEnable = DepthStencilNode["depthWriteEnable"].As<bool>();
		if (!DepthStencilNode["depthCompareOp"].IsNone())
		{
			std::string depthCompareOpStr = DepthStencilNode["depthCompareOp"].As<std::string>();
			OutDepthStencilState.depthCompareOp = ParseCompareOp(depthCompareOpStr.c_str(), depthCompareOpStr.length());
		}
		if (!DepthStencilNode["depthBoundsTestEnable"].IsNone())
			OutDepthStencilState.depthBoundsTestEnable = DepthStencilNode["depthBoundsTestEnable"].As<bool>();
		if (!DepthStencilNode["stencilTestEnable"].IsNone())
			OutDepthStencilState.stencilTestEnable = DepthStencilNode["stencilTestEnable"].As<bool>();
		
		return OutDepthStencilState;
	}

	VkPipelineMultisampleStateCreateInfo ParsePipelineMultisampleNode(Yaml::Node& MultisampleNode)
	{
		VkPipelineMultisampleStateCreateInfo OutMultisampleState = {};
		OutMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		
		if (!MultisampleNode["sampleShadingEnable"].IsNone())
			OutMultisampleState.sampleShadingEnable = MultisampleNode["sampleShadingEnable"].As<bool>();
		if (!MultisampleNode["rasterizationSamples"].IsNone())
		{
			std::string rasterizationSamplesStr = MultisampleNode["rasterizationSamples"].As<std::string>();
			OutMultisampleState.rasterizationSamples = ParseSampleCount(rasterizationSamplesStr.c_str(), rasterizationSamplesStr.length());
		}
		
		return OutMultisampleState;
	}

	VkPipelineInputAssemblyStateCreateInfo ParsePipelineInputAssemblyNode(Yaml::Node& InputAssemblyNode)
	{
		VkPipelineInputAssemblyStateCreateInfo OutInputAssemblyState = {};
		OutInputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		
		if (!InputAssemblyNode["topology"].IsNone())
		{
			std::string topologyStr = InputAssemblyNode["topology"].As<std::string>();
			OutInputAssemblyState.topology = ParseTopology(topologyStr.c_str(), topologyStr.length());
		}
		if (!InputAssemblyNode["primitiveRestartEnable"].IsNone())
			OutInputAssemblyState.primitiveRestartEnable = InputAssemblyNode["primitiveRestartEnable"].As<bool>();
		
		return OutInputAssemblyState;
	}

	VkPipelineViewportStateCreateInfo ParsePipelineViewportStateNode(Yaml::Node& ViewportStateNode)
	{
		VkPipelineViewportStateCreateInfo OutViewportState = {};
		OutViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		if (!ViewportStateNode["viewportCount"].IsNone())
			OutViewportState.viewportCount = ViewportStateNode["viewportCount"].As<u32>();
		if (!ViewportStateNode["scissorCount"].IsNone())
			OutViewportState.scissorCount = ViewportStateNode["scissorCount"].As<u32>();
		return OutViewportState;
	}

	VkViewport ParseViewportNode(Yaml::Node& ViewportNode)
	{
		VkViewport OutViewport = {};
		if (!ViewportNode["minDepth"].IsNone())
			OutViewport.minDepth = ViewportNode["minDepth"].As<f32>();
		if (!ViewportNode["maxDepth"].IsNone())
			OutViewport.maxDepth = ViewportNode["maxDepth"].As<f32>();
		if (!ViewportNode["x"].IsNone())
			OutViewport.x = ViewportNode["x"].As<f32>();
		if (!ViewportNode["y"].IsNone())
			OutViewport.y = ViewportNode["y"].As<f32>();
		return OutViewport;
	}

	VkRect2D ParseScissorNode(Yaml::Node& ScissorNode)
	{
		VkRect2D OutScissor = {};
		if (!ScissorNode["offsetX"].IsNone())
			OutScissor.offset.x = ScissorNode["offsetX"].As<s32>();
		if (!ScissorNode["offsetY"].IsNone())
			OutScissor.offset.y = ScissorNode["offsetY"].As<s32>();
		return OutScissor;
	}

	Yaml::Node& GetPipelineViewportStateNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["viewportState"].IsNone())
			return PipelineNode["viewportState"];
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}
	Yaml::Node& GetViewportNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["viewport"].IsNone())
			return PipelineNode["viewport"];
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}
	Yaml::Node& GetScissorNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["scissor"].IsNone())
			return PipelineNode["scissor"];
		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	VkBool32 ParseBool(const char* Value, u32 Length)
	{
		if (Length == 0) return VK_FALSE;

		if (StringMatches(Value, Length, ParseStrings::TRUE_STRINGS)) return VK_TRUE;
		if (StringMatches(Value, Length, ParseStrings::FALSE_STRINGS)) return VK_FALSE;

		return VK_FALSE;
	}

	VkPolygonMode ParsePolygonMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FILL_STRINGS)) return VK_POLYGON_MODE_FILL;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRINGS)) return VK_POLYGON_MODE_LINE;
		if (StringMatches(Value, Length, ParseStrings::POINT_STRINGS)) return VK_POLYGON_MODE_POINT;

		return VK_POLYGON_MODE_FILL;
	}

	VkCullModeFlags ParseCullMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NONE_STRINGS)) return VK_CULL_MODE_NONE;
		if (StringMatches(Value, Length, ParseStrings::BACK_STRINGS)) return VK_CULL_MODE_BACK_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRONT_STRINGS)) return VK_CULL_MODE_FRONT_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRONT_BACK_STRINGS)) return VK_CULL_MODE_FRONT_AND_BACK;

		return VK_CULL_MODE_BACK_BIT;
	}

	VkFrontFace ParseFrontFace(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::COUNTER_CLOCKWISE_STRINGS)) return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		if (StringMatches(Value, Length, ParseStrings::CLOCKWISE_STRINGS)) return VK_FRONT_FACE_CLOCKWISE;

		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	VkColorComponentFlags ParseColorWriteMask(const char* Value, u32 Length)
	{
		VkColorComponentFlags Flags = 0;

		for (u32 i = 0; i < Length; ++i)
		{
			if (Value[i] == 'R' || Value[i] == 'r') Flags |= VK_COLOR_COMPONENT_R_BIT;
			if (Value[i] == 'G' || Value[i] == 'g') Flags |= VK_COLOR_COMPONENT_G_BIT;
			if (Value[i] == 'B' || Value[i] == 'b') Flags |= VK_COLOR_COMPONENT_B_BIT;
			if (Value[i] == 'A' || Value[i] == 'a') Flags |= VK_COLOR_COMPONENT_A_BIT;
		}

		if (Flags == 0) Flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		return Flags;
	}

	VkBlendFactor ParseBlendFactor(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_ALPHA_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_ALPHA_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_COLOR_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_COLOR_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		if (StringMatches(Value, Length, ParseStrings::SRC_COLOR_STRINGS)) return VK_BLEND_FACTOR_SRC_COLOR;
		if (StringMatches(Value, Length, ParseStrings::DST_COLOR_STRINGS)) return VK_BLEND_FACTOR_DST_COLOR;
		if (StringMatches(Value, Length, ParseStrings::SRC_ALPHA_STRINGS)) return VK_BLEND_FACTOR_SRC_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::DST_ALPHA_STRINGS)) return VK_BLEND_FACTOR_DST_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ZERO_STRINGS)) return VK_BLEND_FACTOR_ZERO;
		if (StringMatches(Value, Length, ParseStrings::ONE_STRINGS)) return VK_BLEND_FACTOR_ONE;

		return VK_BLEND_FACTOR_SRC_ALPHA;
	}

	VkBlendOp ParseBlendOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REVERSE_SUBTRACT_STRINGS)) return VK_BLEND_OP_REVERSE_SUBTRACT;
		if (StringMatches(Value, Length, ParseStrings::SUBTRACT_STRINGS)) return VK_BLEND_OP_SUBTRACT;
		if (StringMatches(Value, Length, ParseStrings::ADD_STRINGS)) return VK_BLEND_OP_ADD;
		if (StringMatches(Value, Length, ParseStrings::MIN_STRINGS)) return VK_BLEND_OP_MIN;
		if (StringMatches(Value, Length, ParseStrings::MAX_STRINGS)) return VK_BLEND_OP_MAX;

		return VK_BLEND_OP_ADD;
	}

	VkCompareOp ParseCompareOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEVER_STRINGS)) return VK_COMPARE_OP_NEVER;
		if (StringMatches(Value, Length, ParseStrings::LESS_STRINGS)) return VK_COMPARE_OP_LESS;
		if (StringMatches(Value, Length, ParseStrings::EQUAL_STRINGS)) return VK_COMPARE_OP_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::LESS_OR_EQUAL_STRINGS)) return VK_COMPARE_OP_LESS_OR_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::GREATER_STRINGS)) return VK_COMPARE_OP_GREATER;
		if (StringMatches(Value, Length, ParseStrings::NOT_EQUAL_STRINGS)) return VK_COMPARE_OP_NOT_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::GREATER_OR_EQUAL_STRINGS)) return VK_COMPARE_OP_GREATER_OR_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::ALWAYS_STRINGS)) return VK_COMPARE_OP_ALWAYS;

		return VK_COMPARE_OP_LESS;
	}

	VkSampleCountFlagBits ParseSampleCount(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_1_STRINGS)) return VK_SAMPLE_COUNT_1_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_2_STRINGS)) return VK_SAMPLE_COUNT_2_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_4_STRINGS)) return VK_SAMPLE_COUNT_4_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_8_STRINGS)) return VK_SAMPLE_COUNT_8_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_16_STRINGS)) return VK_SAMPLE_COUNT_16_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_32_STRINGS)) return VK_SAMPLE_COUNT_32_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_64_STRINGS)) return VK_SAMPLE_COUNT_64_BIT;

		return VK_SAMPLE_COUNT_1_BIT;
	}

	VkPrimitiveTopology ParseTopology(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::POINT_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_FAN_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::PATCH_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	VkShaderStageFlagBits ParseShaderStage(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_SHADER_STRINGS))
			return VK_SHADER_STAGE_VERTEX_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRAGMENT_STRINGS))
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		if (StringMatches(Value, Length, ParseStrings::GEOMETRY_STRINGS))
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		if (StringMatches(Value, Length, ParseStrings::COMPUTE_STRINGS))
			return VK_SHADER_STAGE_COMPUTE_BIT;
		if (StringMatches(Value, Length, ParseStrings::TESS_CONTROL_STRINGS))
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (StringMatches(Value, Length, ParseStrings::TESS_EVAL_STRINGS))
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (StringMatches(Value, Length, ParseStrings::TASK_STRINGS))
			return VK_SHADER_STAGE_TASK_BIT_EXT;
		if (StringMatches(Value, Length, ParseStrings::MESH_STRINGS))
			return VK_SHADER_STAGE_MESH_BIT_EXT;
		if (StringMatches(Value, Length, ParseStrings::RAYGEN_STRINGS))
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::CLOSEST_HIT_STRINGS))
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::ANY_HIT_STRINGS))
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::MISS_STRINGS))
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::INTERSECTION_STRINGS))
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::CALLABLE_STRINGS))
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;

		assert(false);
		return VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkFilter ParseFilter(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEAREST_STRINGS)) return VK_FILTER_NEAREST;
		if (StringMatches(Value, Length, ParseStrings::LINEAR_STRINGS)) return VK_FILTER_LINEAR;
		return VK_FILTER_LINEAR;
	}

	VkSamplerAddressMode ParseAddressMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REPEAT_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		if (StringMatches(Value, Length, ParseStrings::MIRRORED_REPEAT_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_EDGE_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_BORDER_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		if (StringMatches(Value, Length, ParseStrings::MIRROR_CLAMP_TO_EDGE_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}

	VkBorderColor ParseBorderColor(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FLOAT_TRANSPARENT_BLACK_STRINGS)) return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		if (StringMatches(Value, Length, ParseStrings::INT_TRANSPARENT_BLACK_STRINGS)) return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_BLACK_STRINGS)) return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_BLACK_STRINGS)) return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_WHITE_STRINGS)) return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_WHITE_STRINGS)) return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	}

	VkSamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_NEAREST_STRINGS)) return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_LINEAR_STRINGS)) return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}

	VkDescriptorType ParseDescriptorType(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLER_STRINGS)) return VK_DESCRIPTOR_TYPE_SAMPLER;
		if (StringMatches(Value, Length, ParseStrings::COMBINED_IMAGE_SAMPLER_STRINGS)) return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		if (StringMatches(Value, Length, ParseStrings::SAMPLED_IMAGE_STRINGS)) return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_IMAGE_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_TEXEL_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_TEXEL_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_DYNAMIC_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_DYNAMIC_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		if (StringMatches(Value, Length, ParseStrings::INPUT_ATTACHMENT_STRINGS)) return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	VkShaderStageFlags ParseShaderStageFlags(const char* Value, u32 Length)
	{
		VkShaderStageFlags flags = 0;

		const char* token = Value;
		const char* end = Value + Length;

		while (token < end)
		{
			while (token < end && (*token == ' ' || *token == '|' || *token == '\t')) token++;
			if (token >= end) break;

			const char* tokenStart = token;
			while (token < end && *token != ' ' && *token != '|' && *token != '\t') token++;

			u32 tokenLength = static_cast<u32>(token - tokenStart);

			if (StringMatches(tokenStart, tokenLength, ParseStrings::VERTEX_BIT_STRINGS)) flags |= VK_SHADER_STAGE_VERTEX_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::FRAGMENT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::GEOMETRY_BIT_STRINGS)) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::COMPUTE_BIT_STRINGS)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_CONTROL_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_EVALUATION_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TASK_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MESH_BIT_STRINGS)) flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::RAYGEN_BIT_STRINGS)) flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CLOSEST_HIT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::ANY_HIT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MISS_BIT_STRINGS)) flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::INTERSECTION_BIT_STRINGS)) flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CALLABLE_BIT_STRINGS)) flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		}

		return flags;
	}

	//VkFormat ParseShaderTypeToVkFormat(const char* Value, u32 Length)
	//{
	//	if (strncmp(Value, "vec2", Length) == 0) return VK_FORMAT_R32G32_SFLOAT;
	//	if (strncmp(Value, "vec3", Length) == 0) return VK_FORMAT_R32G32B32_SFLOAT;
	//	if (strncmp(Value, "vec4", Length) == 0) return VK_FORMAT_R32G32B32A32_SFLOAT;
	//	if (strncmp(Value, "mat4", Length) == 0) return VK_FORMAT_R32G32B32A32_SFLOAT;
	//	if (strncmp(Value, "uint", Length) == 0) return VK_FORMAT_R32_UINT;
	//	if (strncmp(Value, "int", Length) == 0) return VK_FORMAT_R32_SINT;
	//	if (strncmp(Value, "float", Length) == 0) return VK_FORMAT_R32_SFLOAT;
	//	if (strncmp(Value, "double", Length) == 0) return VK_FORMAT_R64_SFLOAT;

	//	assert(false);
	//	return VK_FORMAT_R32_SFLOAT;
	//}

	BmRender_AttributeType ParseShaderTypeToAttributeType(const char* Value, u32 Length)
	{
		if (strncmp(Value, "int", Length) == 0) return BmRender_AttributeType::Int;
		if (strncmp(Value, "uint", Length) == 0) return BmRender_AttributeType::Uint;
		if (strncmp(Value, "float", Length) == 0) return BmRender_AttributeType::Float;
		if (strncmp(Value, "vec2", Length) == 0) return BmRender_AttributeType::Vec2;
		if (strncmp(Value, "vec3", Length) == 0) return BmRender_AttributeType::Vec3;
		if (strncmp(Value, "vec4", Length) == 0) return BmRender_AttributeType::Vec4;
		if (strncmp(Value, "mat4", Length) == 0) return BmRender_AttributeType::Mat4;

		assert(false);
		return BmRender_AttributeType::Float;
	}

	VkVertexInputRate ParseVertexInputRate(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_STRINGS)) return VK_VERTEX_INPUT_RATE_VERTEX;
		if (StringMatches(Value, Length, ParseStrings::INSTANCE_STRINGS)) return VK_VERTEX_INPUT_RATE_INSTANCE;

		assert(false);
		return VK_VERTEX_INPUT_RATE_VERTEX;
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

	Yaml::Node& GetSceneResources(Yaml::Node& Root)
	{
		if (!Root["SceneResources"].IsNone())
		{
			return Root["SceneResources"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetTextures(Yaml::Node& Root)
	{
		if (!Root["Textures"].IsNone())
		{
			return Root["Textures"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetModels(Yaml::Node& Root)
	{
		if (!Root["Models"].IsNone())
		{
			return Root["Models"];
		}

		assert(false);
		static Yaml::Node Empty;
		return Empty;
	}


	std::string GetModelPath(Yaml::Node& ModelNode)
	{
		return ModelNode["path"].As<std::string>();
	}

	glm::vec3 GetModelPosition(Yaml::Node& ModelNode)
	{
		glm::vec3 Position(0.0f);
		
		if (!ModelNode["position"].IsNone())
		{
			Yaml::Node& PositionNode = ModelNode["position"];
			
			if (!PositionNode["x"].IsNone())
			{
				Position.x = PositionNode["x"].As<f32>();
			}
			if (!PositionNode["y"].IsNone())
			{
				Position.y = PositionNode["y"].As<f32>();
			}
			if (!PositionNode["z"].IsNone())
			{
				Position.z = PositionNode["z"].As<f32>();
			}
		}
		
		return Position;
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

	BufferUsageFlag ParseBufferUsageFlag(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::COMBINED_VERTEX_INDEX_FLAG_STRINGS)) return BufferUsageFlag::CombinedVertexIndexFlag;
		if (StringMatches(Value, Length, ParseStrings::INSTANCE_FLAG_STRINGS)) return BufferUsageFlag::InstanceFlag;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_FLAG_STRINGS)) return BufferUsageFlag::StorageFlag;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_FLAG_STRINGS)) return BufferUsageFlag::UniformFlag;
		if (StringMatches(Value, Length, ParseStrings::VERTEX_FLAG_STRINGS)) return BufferUsageFlag::VertexFlag;
		if (StringMatches(Value, Length, ParseStrings::INDEX_FLAG_STRINGS)) return BufferUsageFlag::IndexFlag;

		assert(false);
		return BufferUsageFlag::VertexFlag;
	}

	MemoryPropertyFlag ParseMemoryPropertyFlag(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::GPU_LOCAL_STRINGS)) return MemoryPropertyFlag::GPULocal;
		if (StringMatches(Value, Length, ParseStrings::CPU_HOST_COMPATIBLE_STRINGS)) return MemoryPropertyFlag::HostCompatible;

		assert(false);
		return MemoryPropertyFlag::GPULocal;
	}

	PipelineStage ParseStageBarrier(const char* Value, u32 Length)
	{
		if (strncmp(Value, "Vertex", Length) == 0)
		{
			return PipelineStage::Vertex;
		}
		else if (strncmp(Value, "Fragment", Length) == 0)
		{
			return PipelineStage::Fragment;
		}

		assert(false);
		return PipelineStage::Fragment;
	}

	BufferUpdateFrequency ParseUpdateFrequency(const char* Value, u32 Length)
	{
		if (strncmp(Value, "Static", Length) == 0)
		{
			return BufferUpdateFrequency::Static;
		}
		else if (strncmp(Value, "PerFrame", Length) == 0)
		{
			return BufferUpdateFrequency::PerFrame;
		}

		return BufferUpdateFrequency::Static;
	}

	ShaderType ParseShaderType(const char* Value, u32 Length)
	{
		if (strncmp(Value, "uniform", Length) == 0)
		{
			return ShaderType::Uniform;
		}
		else if (strncmp(Value, "buffer", Length) == 0)
		{
			return ShaderType::Buffer;
		}
		else if (strncmp(Value, "sampler2d", Length) == 0 || strncmp(Value, "sampler2D", Length) == 0)
		{
			return ShaderType::Sampler2D;
		}
		else if (strncmp(Value, "sampler2dArray", Length) == 0 || strncmp(Value, "sampler2DArray", Length) == 0)
		{
			return ShaderType::Sampler2DArray;
		}

		return ShaderType::Sampler2D; // Default fallback
	}

	std::vector<DescriptorSetLayout> ParseDescriptorSetLayouts(Yaml::Node& DescriptorSetLayoutsNode)
	{
		std::vector<DescriptorSetLayout> Layouts;
		
		for (auto LayoutIt = DescriptorSetLayoutsNode.Begin(); LayoutIt != DescriptorSetLayoutsNode.End(); LayoutIt++)
		{
			DescriptorSetLayout Layout;
			Layout.Name = (*LayoutIt).first;
			
			Yaml::Node& LayoutNode = (*LayoutIt).second;
			Yaml::Node& BindingsNode = ParseDescriptorSetLayoutNode(LayoutNode);
			
			for (auto BindingIt = BindingsNode.Begin(); BindingIt != BindingsNode.End(); BindingIt++)
			{
				DescriptorBinding Binding = {};
				
				// Parse shader type
				if (!(*BindingIt).second["type"].IsNone())
				{
					std::string typeStr = (*BindingIt).second["type"].As<std::string>();
					Binding.Type = ParseShaderType(typeStr.c_str(), typeStr.length());
				}
				
				// Parse update frequency
				if (!(*BindingIt).second["updateFrequency"].IsNone())
				{
					std::string freqStr = (*BindingIt).second["updateFrequency"].As<std::string>();
					Binding.UpdateFrequency = ParseUpdateFrequency(freqStr.c_str(), freqStr.length());
				}
				else
				{
					Binding.UpdateFrequency = BufferUpdateFrequency::Static;
				}
				
				// Parse stage flags
				if (!(*BindingIt).second["stages"].IsNone())
				{
					std::string value = (*BindingIt).second["stages"].As<std::string>();
					Binding.StageFlags = ParseShaderStageFlags(value.c_str(), value.length());
				}
				else if (!(*BindingIt).second["stageFlags"].IsNone())
				{
					std::string value = (*BindingIt).second["stageFlags"].As<std::string>();
					Binding.StageFlags = ParseShaderStageFlags(value.c_str(), value.length());
				}
				
				Layout.Bindings.push_back(Binding);
			}
			
			Layouts.push_back(Layout);
		}
		
		return Layouts;
	}

	void ParseShaderBufferFromYaml(Yaml::Node& BufferNode, u64& Capacity, BufferUpdateFrequency& UpdateFrequency, PipelineStage& StageBarrier, std::string& OutName)
	{
		OutName = GetBufferName(BufferNode);
		if (!BufferNode["Capacity"].IsNone())
			Capacity = BufferNode["Capacity"].As<u64>();
		if (!BufferNode["UpdateFrequency"].IsNone())
		{
			std::string v = BufferNode["UpdateFrequency"].As<std::string>();
			UpdateFrequency = ParseUpdateFrequency(v.c_str(), (u32)v.length());
		}
		if (!BufferNode["StageBarier"].IsNone())
		{
			std::string v = BufferNode["StageBarier"].As<std::string>();
			StageBarrier = ParseStageBarrier(v.c_str(), (u32)v.length());
		}
	}

	void ParseGeometryBufferFromYaml(Yaml::Node& BufferNode, u64& Capacity, BufferUpdateFrequency& UpdateFrequency, std::string& OutName)
	{
		OutName = GetBufferName(BufferNode);
		if (!BufferNode["Capacity"].IsNone())
			Capacity = BufferNode["Capacity"].As<u64>();
		if (!BufferNode["UpdateFrequency"].IsNone())
		{
			std::string v = BufferNode["UpdateFrequency"].As<std::string>();
			UpdateFrequency = ParseUpdateFrequency(v.c_str(), (u32)v.length());
		}
	}

// Deprecated ParseStorageBufferNode removed

	std::string GetBufferName(Yaml::Node& BufferNode)
	{
		if (!BufferNode["name"].IsNone())
		{
			return BufferNode["name"].As<std::string>();
		}
		return {};
	}

	void ParseDescriptorSetLayoutFromYaml(Yaml::Node& DescriptorSetLayoutNode, BmRender_DescriptorSetLayoutDescription& Description, std::vector<BmRender_LayoutBinding>& Bindings)
	{
		Yaml::Node& BindingsNode = ParseDescriptorSetLayoutNode(DescriptorSetLayoutNode);
		
		Bindings.clear();
		for (auto BindingIt = BindingsNode.Begin(); BindingIt != BindingsNode.End(); BindingIt++)
		{
			BmRender_LayoutBinding Binding = ParseDescriptorSetLayoutBindingNode((*BindingIt).second);
			Bindings.push_back(Binding);
		}
		
		Description.Bindings = Bindings.data();
		Description.BindingsCount = static_cast<u32>(Bindings.size());
	}

	Yaml::Node& GetPushConstantNode(Yaml::Node& PipelineNode)
	{
		if (!PipelineNode["PushConstant"].IsNone())
		{
			return PipelineNode["PushConstant"];
		}
		static Yaml::Node Empty;
		return Empty;
	}

	Yaml::Node& GetPushConstantsFromResources(Yaml::Node& Root)
	{
		if (!Root["PushConstants"].IsNone())
		{
			return Root["PushConstants"];
		}
		static Yaml::Node Empty;
		return Empty;
	}

	void ParseAndCreatePushConstants(Yaml::Node& PushConstantsNode)
	{
		for (auto it = PushConstantsNode.Begin(); it != PushConstantsNode.End(); it++)
		{
			std::string ConstantName = (*it).first;
			Yaml::Node& ConstantNode = (*it).second;
			
			u32 totalSize = 0;
			VkShaderStageFlags combinedStageFlags = 0;
			
			// Calculate total size and combine stage flags
			for (auto TypeIt = ConstantNode.Begin(); TypeIt != ConstantNode.End(); TypeIt++)
			{
				Yaml::Node& TypeNode = (*TypeIt).second;
				std::string TypeName = TypeNode["type"].As<std::string>();
				
				u32 Size = 0;
				VkShaderStageFlags StageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				
				// Calculate size based on type
				if (TypeName == "uint" || TypeName == "int")
				{
					Size = sizeof(u32);
				}
				else if (TypeName == "float")
				{
					Size = sizeof(float);
				}
				else if (TypeName == "vec2")
				{
					Size = sizeof(float) * 2;
				}
				else if (TypeName == "vec3")
				{
					Size = sizeof(float) * 3;
				}
				else if (TypeName == "vec4")
				{
					Size = sizeof(float) * 4;
				}
				else if (TypeName == "mat4")
				{
					Size = sizeof(float) * 16;
				}
				else
				{
					assert(false && "Unsupported push constant type");
				}
				
				// Parse stages if specified
				if (!TypeNode["stages"].IsNone())
				{
					std::string StagesStr = TypeNode["stages"].As<std::string>();
					StageFlags = ParseShaderStageFlags(StagesStr.c_str(), StagesStr.length());
				}
				
				totalSize += Size;
				combinedStageFlags |= StageFlags;
			}
			
			// Create a single push constant handle for the entire definition
			BmRender_PushConstant PushConstantHandle = BmRender_CreatePushConstant(
				static_cast<PipelineStage>(combinedStageFlags), 
				0, // offset starts at 0 for the entire push constant
				totalSize
			);
			
			// Store the single handle
			PushConstants[ConstantName] = PushConstantHandle;
		}
	}

	void ParsePushConstantsFromYaml(Yaml::Node& PushConstantsNode, std::vector<VkPushConstantRange>& PushConstantRanges)
	{
		u32 currentOffset = 0;
		
		for (auto it = PushConstantsNode.Begin(); it != PushConstantsNode.End(); it++)
		{
			std::string ConstantName = (*it).first;
			Yaml::Node& ConstantNode = (*it).second;
			
			// Parse each constant type in the array
			for (auto TypeIt = ConstantNode.Begin(); TypeIt != ConstantNode.End(); TypeIt++)
			{
				Yaml::Node& TypeNode = (*TypeIt).second;
				std::string TypeName = TypeNode["type"].As<std::string>();
				
				VkPushConstantRange Range = {};
				Range.offset = currentOffset;
				
				// Calculate size based on type
				if (TypeName == "uint" || TypeName == "int")
				{
					Range.size = sizeof(u32);
				}
				else if (TypeName == "float")
				{
					Range.size = sizeof(float);
				}
				else if (TypeName == "vec2")
				{
					Range.size = sizeof(float) * 2;
				}
				else if (TypeName == "vec3")
				{
					Range.size = sizeof(float) * 3;
				}
				else if (TypeName == "vec4")
				{
					Range.size = sizeof(float) * 4;
				}
				else if (TypeName == "mat4")
				{
					Range.size = sizeof(float) * 16;
				}
				else
				{
					assert(false && "Unsupported push constant type");
				}
				
				// Parse stages if specified, otherwise default to vertex and fragment stages
				if (!TypeNode["stages"].IsNone())
				{
					std::string StagesStr = TypeNode["stages"].As<std::string>();
					Range.stageFlags = ParseShaderStageFlags(StagesStr.c_str(), StagesStr.length());
				}
				else
				{
					Range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				}
				
				PushConstantRanges.push_back(Range);
				currentOffset += Range.size;
			}
		}
	}

	BmRender_PipelineDescription ParsePipelineFromYaml(const std::string& YamlFilePath, VkExtent2D Extent, const PipelineResourceInfo& ResourceInfo, 
		std::vector<VkPipelineShaderStageCreateInfo>& ShaderStages,
		std::vector<BmRender_VertexBinding>& VertexBindings,
		std::vector<BmRender_DescriptorSetLayout>& OutDescriptorSetLayouts,
		std::vector<BmRender_PushConstant>& PushConstantRanges)
	{
		// Clear the vectors first
		//ShaderStages.clear();
		//VertexBindings.clear();
		//VertexAttributes.clear();
		//DescriptorSetLayouts.clear();
		//PushConstantRanges.clear();

		Yaml::Node Root;
		Yaml::Parse(Root, YamlFilePath.c_str());
		Yaml::Node& PipelineNode = GetPipelineNode(Root);

		// Parse pipeline layout
		Yaml::Node& PipelineLayoutNode = GetPipelineLayoutNode(PipelineNode);
		for (auto it = PipelineLayoutNode.Begin(); it != PipelineLayoutNode.End(); it++)
		{
			std::string LayoutName = (*it).second.As<std::string>();
			BmRender_DescriptorSetLayout LayoutHandle = DescriptorSetLayouts[LayoutName];
			OutDescriptorSetLayouts.push_back(LayoutHandle);
		}

		// Parse push constants
		Yaml::Node& PushConstantNode = GetPushConstantNode(PipelineNode);
		if (!PushConstantNode.IsNone())
		{
			std::string PushConstantName = PushConstantNode.As<std::string>();
			
			// Look up the push constant handle in the global map
			auto it = PushConstants.find(PushConstantName);
			if (it != PushConstants.end())
			{
				// Store the handle directly
				PushConstantRanges.push_back(it->second);
			}
		}

		// Parse shader stages
		Yaml::Node& ShadersNode = GetPipelineShadersNode(PipelineNode);
		for (auto it = ShadersNode.Begin(); it != ShadersNode.End(); it++)
		{
			VkPipelineShaderStageCreateInfo ShaderStage = {};
			ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			ShaderStage.stage = ParseShaderStage((*it).first.c_str(), (*it).first.length());
			ShaderStage.pName = "main";
			ShaderStage.module = GetShaderData(Shaders[(*it).second.As<std::string>()])->VulkanShaderModule;
			ShaderStages.push_back(ShaderStage);
		}

		// Parse vertex input
		Yaml::Node& VertexAttributeLayoutNode = GetVertexAttributeLayoutNode(PipelineNode);
		if (!VertexAttributeLayoutNode.IsNone())
		{
			for (auto VertexTypeIt = VertexAttributeLayoutNode.Begin(); VertexTypeIt != VertexAttributeLayoutNode.End(); VertexTypeIt++)
			{
				Yaml::Node& VertexTypeNode = (*VertexTypeIt).second;
				std::string VertexTypeName = ParseNameNode(VertexTypeNode);
				Yaml::Node& AttributesNode = GetVertexAttributesNode(VertexTypeNode);

				VertexBinding_depr VertexBindingDepr = VBindings[VertexTypeName];

				BmRender_VertexBinding BmRenderVertexBinding = { };
				BmRenderVertexBinding.Stride = VertexBindingDepr.Stride;
				BmRenderVertexBinding.InputRate = VertexBindingDepr.InputRate;
				BmRenderVertexBinding.AttributesCount = AttributesNode.Size();
				BmRenderVertexBinding.Attributes = (VertexAttribute*)malloc(sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);

				u32 testIndex = 0;
				for (auto AttrIt = AttributesNode.Begin(); AttrIt != AttributesNode.End(); AttrIt++)
				{
					Yaml::Node& AttributeNode = (*AttrIt).second;
					std::string AttributeName = ParseNameNode(AttributeNode);

					auto bindingAttrIt = VertexBindingDepr.Attributes.find(AttributeName);
					if (bindingAttrIt != VertexBindingDepr.Attributes.end())
					{
						BmRenderVertexBinding.Attributes[testIndex] = bindingAttrIt->second;
					}

					++testIndex;
				}
				
				VertexBindings.push_back(BmRenderVertexBinding);
			}
		}

		// Parse pipeline states
		Yaml::Node& RasterizationNode = GetPipelineRasterizationNode(PipelineNode);
		Yaml::Node& ColorBlendStateNode = GetPipelineColorBlendStateNode(PipelineNode);
		Yaml::Node& ColorBlendAttachmentNode = GetPipelineColorBlendAttachmentNode(PipelineNode);
		Yaml::Node& DepthStencilNode = GetPipelineDepthStencilNode(PipelineNode);
		Yaml::Node& MultisampleNode = GetPipelineMultisampleNode(PipelineNode);
		Yaml::Node& InputAssemblyNode = GetPipelineInputAssemblyNode(PipelineNode);
		Yaml::Node& ViewportStateNode = GetPipelineViewportStateNode(PipelineNode);
		Yaml::Node& ViewportNode = GetViewportNode(PipelineNode);
		Yaml::Node& ScissorNode = GetScissorNode(PipelineNode);

		// Create the final description structure with C arrays
		BmRender_PipelineDescription Description = {};
		Description.Extent = Extent;
		Description.ResourceInfo = ResourceInfo;

		// Convert vectors to C arrays
		Description.ShaderStages = ShaderStages.empty() ? nullptr : ShaderStages.data();
		Description.ShaderStagesCount = static_cast<u32>(ShaderStages.size());
		Description.VertexBindings = VertexBindings.empty() ? nullptr : VertexBindings.data();
		Description.VertexBindingsCount = static_cast<u32>(VertexBindings.size());
		Description.DescriptorSetLayouts = OutDescriptorSetLayouts.empty() ? nullptr : OutDescriptorSetLayouts.data();
		Description.DescriptorSetLayoutsCount = static_cast<u32>(OutDescriptorSetLayouts.size());
		Description.PushConstantRanges = PushConstantRanges.empty() ? nullptr : PushConstantRanges.data();
		Description.PushConstantRangesCount = static_cast<u32>(PushConstantRanges.size());

		Description.RasterizationState = ParsePipelineRasterizationNode(RasterizationNode);
		Description.ColorBlendAttachment = ParsePipelineColorBlendAttachmentNode(ColorBlendAttachmentNode);
		Description.ColorBlendState = ParsePipelineColorBlendStateNode(ColorBlendStateNode);
		Description.DepthStencilState = ParsePipelineDepthStencilNode(DepthStencilNode);
		Description.MultisampleState = ParsePipelineMultisampleNode(MultisampleNode);
		Description.InputAssemblyState = ParsePipelineInputAssemblyNode(InputAssemblyNode);
		Description.ViewportState = ParsePipelineViewportStateNode(ViewportStateNode);

		Description.Viewport = ParseViewportNode(ViewportNode);
		Description.Viewport.width = Extent.width;
		Description.Viewport.height = Extent.height;

		Description.Scissor = ParseScissorNode(ScissorNode);
		Description.Scissor.extent.width = Extent.width;
		Description.Scissor.extent.height = Extent.height;

		return Description;
	}
}

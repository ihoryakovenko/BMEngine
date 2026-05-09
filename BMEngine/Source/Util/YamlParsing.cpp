#include "YamlParsing.h"

#include <SharedLib.h>

#include <Engine/Systems/Memory/MemoryManagmentSystem.h>

extern std::unordered_map<std::string, BmRender_Sampler> Samplers;
extern std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
extern std::unordered_map<std::string, BmRender_Shader> Shaders;
extern std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
extern std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;
extern 	std::unordered_map<std::string, BmRender_PushConstant> PushConstants;

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
		return { };
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

	BmRender_DescriptorSetLayoutBinding ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode)
	{
		BmRender_DescriptorSetLayoutBinding OutBinding = { };
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


	BmRender_PipelineShaderStage ParseShaderStage(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_SHADER_STRINGS))
			return BmRender_PipelineShaderStage::Vertex;
		if (StringMatches(Value, Length, ParseStrings::FRAGMENT_STRINGS))
			return BmRender_PipelineShaderStage::Fragment;
		if (StringMatches(Value, Length, ParseStrings::GEOMETRY_STRINGS))
			return BmRender_PipelineShaderStage::Geometry;
		if (StringMatches(Value, Length, ParseStrings::COMPUTE_STRINGS))
			return BmRender_PipelineShaderStage::Compute;
		if (StringMatches(Value, Length, ParseStrings::TESS_CONTROL_STRINGS))
			return BmRender_PipelineShaderStage::TessControl;
		if (StringMatches(Value, Length, ParseStrings::TESS_EVAL_STRINGS))
			return BmRender_PipelineShaderStage::TessEval;
		if (StringMatches(Value, Length, ParseStrings::TASK_STRINGS))
			assert(false); // Task shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::MESH_STRINGS))
			assert(false); // Mesh shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::RAYGEN_STRINGS))
			assert(false); // Ray gen shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::CLOSEST_HIT_STRINGS))
			assert(false); // Closest hit shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::ANY_HIT_STRINGS))
			assert(false); // Any hit shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::MISS_STRINGS))
			assert(false); // Miss shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::INTERSECTION_STRINGS))
			assert(false); // Intersection shader not supported in PipelineShaderStage enum
		if (StringMatches(Value, Length, ParseStrings::CALLABLE_STRINGS))
			assert(false); // Callable shader not supported in PipelineShaderStage enum

		assert(false);
		return BmRender_PipelineShaderStage::Vertex;
	}

	BmRender_Filter ParseFilter(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEAREST_STRINGS)) return BmRender_Filter::Nearest;
		if (StringMatches(Value, Length, ParseStrings::LINEAR_STRINGS)) return BmRender_Filter::Linear;
		return BmRender_Filter::Linear;
	}

	BmRender_SamplerAddressMode ParseAddressMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REPEAT_STRINGS)) return BmRender_SamplerAddressMode::Repeat;
		if (StringMatches(Value, Length, ParseStrings::MIRRORED_REPEAT_STRINGS)) return BmRender_SamplerAddressMode::MirroredRepeat;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_EDGE_STRINGS)) return BmRender_SamplerAddressMode::ClampToEdge;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_BORDER_STRINGS)) return BmRender_SamplerAddressMode::ClampToBorder;
		if (StringMatches(Value, Length, ParseStrings::MIRROR_CLAMP_TO_EDGE_STRINGS)) return BmRender_SamplerAddressMode::MirrorClampToEdge;
		return BmRender_SamplerAddressMode::ClampToEdge;
	}

	BmRender_BorderColor ParseBorderColor(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FLOAT_TRANSPARENT_BLACK_STRINGS)) return BmRender_BorderColor::FloatTransparentBlack;
		if (StringMatches(Value, Length, ParseStrings::INT_TRANSPARENT_BLACK_STRINGS)) return BmRender_BorderColor::IntTransparentBlack;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_BLACK_STRINGS)) return BmRender_BorderColor::FloatOpaqueBlack;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_BLACK_STRINGS)) return BmRender_BorderColor::IntOpaqueBlack;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_WHITE_STRINGS)) return BmRender_BorderColor::FloatOpaqueWhite;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_WHITE_STRINGS)) return BmRender_BorderColor::IntOpaqueWhite;
		return BmRender_BorderColor::IntOpaqueBlack;
	}

	BmRender_SamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_NEAREST_STRINGS)) return BmRender_SamplerMipmapMode::Nearest;
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_LINEAR_STRINGS)) return BmRender_SamplerMipmapMode::Linear;
		return BmRender_SamplerMipmapMode::Linear;
	}

	BmRender_DescriptorType ParseDescriptorType(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLER_STRINGS)) return BmRender_DescriptorType::Sampler;
		if (StringMatches(Value, Length, ParseStrings::COMBINED_IMAGE_SAMPLER_STRINGS)) return BmRender_DescriptorType::CombinedImageSampler;
		if (StringMatches(Value, Length, ParseStrings::SAMPLED_IMAGE_STRINGS)) return BmRender_DescriptorType::SampledImage;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_IMAGE_STRINGS)) return BmRender_DescriptorType::StorageImage;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_TEXEL_BUFFER_STRINGS)) return BmRender_DescriptorType::UniformTexelBuffer;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_TEXEL_BUFFER_STRINGS)) return BmRender_DescriptorType::StorageTexelBuffer;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_STRINGS)) return BmRender_DescriptorType::UniformBuffer;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_STRINGS)) return BmRender_DescriptorType::StorageBuffer;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_DYNAMIC_STRINGS)) return BmRender_DescriptorType::UniformBufferDynamic;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_DYNAMIC_STRINGS)) return BmRender_DescriptorType::StorageBufferDynamic;
		if (StringMatches(Value, Length, ParseStrings::INPUT_ATTACHMENT_STRINGS)) return BmRender_DescriptorType::InputAttachment;

		return BmRender_DescriptorType::CombinedImageSampler;
	}

	BmRender_DescriptorShaderStage ParseShaderStageFlags(const char* Value, u32 Length)
	{
		u64 flags = 0;

		const char* token = Value;
		const char* end = Value + Length;

		while (token < end)
		{
			while (token < end && (*token == ' ' || *token == '|' || *token == '\t')) token++;
			if (token >= end) break;

			const char* tokenStart = token;
			while (token < end && *token != ' ' && *token != '|' && *token != '\t') token++;

			u32 tokenLength = static_cast<u32>(token - tokenStart);

			if (StringMatches(tokenStart, tokenLength, ParseStrings::VERTEX_BIT_STRINGS)) flags |= static_cast<u64>(BmRender_DescriptorShaderStage::Vertex);
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::FRAGMENT_BIT_STRINGS)) flags |= static_cast<u64>(BmRender_DescriptorShaderStage::Fragment);
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::COMPUTE_BIT_STRINGS)) flags |= static_cast<u64>(BmRender_DescriptorShaderStage::Compute);
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::GEOMETRY_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_CONTROL_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_EVALUATION_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TASK_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MESH_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::RAYGEN_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CLOSEST_HIT_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::ANY_HIT_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MISS_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::INTERSECTION_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CALLABLE_BIT_STRINGS)) assert(false); // Not in DescriptorShaderStage
		}

		return static_cast<BmRender_DescriptorShaderStage>(flags);
	}

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

	BmRender_VertexInputRate ParseVertexInputRate(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_STRINGS)) return BmRender_VertexInputRate::Vertex;
		if (StringMatches(Value, Length, ParseStrings::INSTANCE_STRINGS)) return BmRender_VertexInputRate::Instance;

		assert(false);
		return BmRender_VertexInputRate::Vertex;
	}

	MemoryPropertyFlag ParseMemoryPropertyFlag(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::GPU_LOCAL_STRINGS)) return MemoryPropertyFlag::GPULocal;
		if (StringMatches(Value, Length, ParseStrings::CPU_HOST_COMPATIBLE_STRINGS)) return MemoryPropertyFlag::HostCompatible;

		assert(false);
		return MemoryPropertyFlag::GPULocal;
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
				DescriptorBinding Binding = { };

				// Parse shader type
				if (!(*BindingIt).second["type"].IsNone())
				{
					std::string typeStr = (*BindingIt).second["type"].As<std::string>();
					Binding.Type = ParseShaderType(typeStr.c_str(), typeStr.length());
				}

				// Parse memory property flag (backward compatible with updateFrequency field)
				if (!(*BindingIt).second["updateFrequency"].IsNone())
				{
					std::string freqStr = (*BindingIt).second["updateFrequency"].As<std::string>();
					// Map legacy "Static" -> GPULocal, "PerFrame" -> HostCompatible
					if (freqStr == "Static")
					{
						Binding.MemoryFlag = MemoryPropertyFlag::GPULocal;
					}
					else if (freqStr == "PerFrame")
					{
						Binding.MemoryFlag = MemoryPropertyFlag::HostCompatible;
					}
					else
					{
						Binding.MemoryFlag = ParseMemoryPropertyFlag(freqStr.c_str(), freqStr.length());
					}
				}
				else if (!(*BindingIt).second["memoryFlag"].IsNone())
				{
					std::string flagStr = (*BindingIt).second["memoryFlag"].As<std::string>();
					Binding.MemoryFlag = ParseMemoryPropertyFlag(flagStr.c_str(), flagStr.length());
				}
				else
				{
					Binding.MemoryFlag = MemoryPropertyFlag::GPULocal;
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

	// Deprecated ParseStorageBufferNode removed

	std::string GetBufferName(Yaml::Node& BufferNode)
	{
		if (!BufferNode["name"].IsNone())
		{
			return BufferNode["name"].As<std::string>();
		}
		return { };
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

	std::string ParseShaderNode(Yaml::Node& ShaderNode)
	{
		// Check if it's the new format (with path and PipelineStage)
		if (!ShaderNode["path"].IsNone())
		{
			return ShaderNode["path"].As<std::string>();
		}
		// Fallback to old format (just a string)
		else
		{
			return ShaderNode.As<std::string>();
		}
	}

	BmRender_PipelineShaderStage ParseShaderPipelineStage(Yaml::Node& ShaderNode)
	{
		// Check if it's the new format (with path and PipelineStage)
		if (!ShaderNode["PipelineStage"].IsNone())
		{
			std::string StageStr = ShaderNode["PipelineStage"].As<std::string>();
			return ParseShaderStage(StageStr.c_str(), StageStr.length());
		}
		// Fallback: try to determine stage from shader name
		assert(false);
	}
}

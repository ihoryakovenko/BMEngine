#pragma once

#include "Engine/Systems/Render/RenderResources.h"
#include <RenderInterface.h>

#include <vector>
#include <string>

#include <mini-yaml/yaml/Yaml.hpp>

namespace Util
{
	enum class ShaderType
	{
		Uniform,
		Buffer,
		Sampler2D,
		Sampler2DArray
	};

	struct DescriptorBinding
	{
		ShaderType Type;
		MemoryPropertyFlag MemoryFlag;
		BmRender_DescriptorShaderStage StageFlags;
	};

	struct DescriptorSetLayout
	{
		std::string Name;
		std::vector<DescriptorBinding> Bindings;
	};

	struct VertexBinding_depr
	{
		u32 Stride;
		BmRender_VertexInputRate InputRate;
		std::unordered_map<std::string, VertexAttribute> Attributes;
	};

	namespace ParseStrings
	{
		// Boolean values
		inline constexpr const char* TRUE_STRINGS[] = { "true", "1", "yes", "on" };
		inline constexpr const char* FALSE_STRINGS[] = { "false", "0", "no", "off" };

		// Polygon modes
		inline constexpr const char* FILL_STRINGS[] = { "fill", "FILL" };
		inline constexpr const char* LINE_STRINGS[] = { "line", "LINE" };
		inline constexpr const char* POINT_STRINGS[] = { "point", "POINT" };

		// Cull modes
		inline constexpr const char* NONE_STRINGS[] = { "none", "NONE" };
		inline constexpr const char* BACK_STRINGS[] = { "back", "BACK" };
		inline constexpr const char* FRONT_STRINGS[] = { "front", "FRONT" };
		inline constexpr const char* FRONT_BACK_STRINGS[] = { "front_back", "FRONT_BACK" };

		// Front face
		inline constexpr const char* COUNTER_CLOCKWISE_STRINGS[] = { "counter_clockwise", "COUNTER_CLOCKWISE" };
		inline constexpr const char* CLOCKWISE_STRINGS[] = { "clockwise", "CLOCKWISE" };

		// Blend factors
		inline constexpr const char* ONE_MINUS_SRC_ALPHA_STRINGS[] = { "one_minus_src_alpha", "ONE_MINUS_SRC_ALPHA" };
		inline constexpr const char* ONE_MINUS_DST_ALPHA_STRINGS[] = { "one_minus_dst_alpha", "ONE_MINUS_DST_ALPHA" };
		inline constexpr const char* ONE_MINUS_SRC_COLOR_STRINGS[] = { "one_minus_src_color", "ONE_MINUS_SRC_COLOR" };
		inline constexpr const char* ONE_MINUS_DST_COLOR_STRINGS[] = { "one_minus_dst_color", "ONE_MINUS_DST_COLOR" };
		inline constexpr const char* SRC_COLOR_STRINGS[] = { "src_color", "SRC_COLOR" };
		inline constexpr const char* DST_COLOR_STRINGS[] = { "dst_color", "DST_COLOR" };
		inline constexpr const char* SRC_ALPHA_STRINGS[] = { "src_alpha", "SRC_ALPHA" };
		inline constexpr const char* DST_ALPHA_STRINGS[] = { "dst_alpha", "DST_ALPHA" };
		inline constexpr const char* ZERO_STRINGS[] = { "zero", "ZERO" };
		inline constexpr const char* ONE_STRINGS[] = { "one", "ONE" };

		// Blend operations
		inline constexpr const char* REVERSE_SUBTRACT_STRINGS[] = { "reverse_subtract", "REVERSE_SUBTRACT" };
		inline constexpr const char* SUBTRACT_STRINGS[] = { "subtract", "SUBTRACT" };
		inline constexpr const char* ADD_STRINGS[] = { "add", "ADD" };
		inline constexpr const char* MIN_STRINGS[] = { "min", "MIN" };
		inline constexpr const char* MAX_STRINGS[] = { "max", "MAX" };

		// Compare operations
		inline constexpr const char* NEVER_STRINGS[] = { "never", "NEVER" };
		inline constexpr const char* LESS_STRINGS[] = { "less", "LESS" };
		inline constexpr const char* EQUAL_STRINGS[] = { "equal", "EQUAL" };
		inline constexpr const char* LESS_OR_EQUAL_STRINGS[] = { "less_or_equal", "LESS_OR_EQUAL" };
		inline constexpr const char* GREATER_STRINGS[] = { "greater", "GREATER" };
		inline constexpr const char* NOT_EQUAL_STRINGS[] = { "not_equal", "NOT_EQUAL" };
		inline constexpr const char* GREATER_OR_EQUAL_STRINGS[] = { "greater_or_equal", "GREATER_OR_EQUAL" };
		inline constexpr const char* ALWAYS_STRINGS[] = { "always", "ALWAYS" };

		// Sample counts
		inline constexpr const char* SAMPLE_1_STRINGS[] = { "1" };
		inline constexpr const char* SAMPLE_2_STRINGS[] = { "2" };
		inline constexpr const char* SAMPLE_4_STRINGS[] = { "4" };
		inline constexpr const char* SAMPLE_8_STRINGS[] = { "8" };
		inline constexpr const char* SAMPLE_16_STRINGS[] = { "16" };
		inline constexpr const char* SAMPLE_32_STRINGS[] = { "32" };
		inline constexpr const char* SAMPLE_64_STRINGS[] = { "64" };

		// Primitive topologies
		inline constexpr const char* POINT_LIST_STRINGS[] = { "point_list", "POINT_LIST" };
		inline constexpr const char* LINE_LIST_STRINGS[] = { "line_list", "LINE_LIST" };
		inline constexpr const char* LINE_STRIP_STRINGS[] = { "line_strip", "LINE_STRIP" };
		inline constexpr const char* TRIANGLE_LIST_STRINGS[] = { "triangle_list", "TRIANGLE_LIST" };
		inline constexpr const char* TRIANGLE_STRIP_STRINGS[] = { "triangle_strip", "TRIANGLE_STRIP" };
		inline constexpr const char* TRIANGLE_FAN_STRINGS[] = { "triangle_fan", "TRIANGLE_FAN" };
		inline constexpr const char* LINE_LIST_WITH_ADJACENCY_STRINGS[] = { "line_list_with_adjacency", "LINE_LIST_WITH_ADJACENCY" };
		inline constexpr const char* LINE_STRIP_WITH_ADJACENCY_STRINGS[] = { "line_strip_with_adjacency", "LINE_STRIP_WITH_ADJACENCY" };
		inline constexpr const char* TRIANGLE_LIST_WITH_ADJACENCY_STRINGS[] = { "triangle_list_with_adjacency", "TRIANGLE_LIST_WITH_ADJACENCY" };
		inline constexpr const char* TRIANGLE_STRIP_WITH_ADJACENCY_STRINGS[] = { "triangle_strip_with_adjacency", "TRIANGLE_STRIP_WITH_ADJACENCY" };
		inline constexpr const char* PATCH_LIST_STRINGS[] = { "patch_list", "PATCH_LIST" };

		// Shader stages
		inline constexpr const char* VERTEX_SHADER_STRINGS[] = { "vertex", "Vertex", "VERTEX" };
		inline constexpr const char* FRAGMENT_STRINGS[] = { "fragment", "Fragment", "FRAGMENT" };
		inline constexpr const char* GEOMETRY_STRINGS[] = { "geometry", "GEOMETRY" };
		inline constexpr const char* COMPUTE_STRINGS[] = { "compute", "COMPUTE" };
		inline constexpr const char* TESS_CONTROL_STRINGS[] = { "tess_control", "TESS_CONTROL" };
		inline constexpr const char* TESS_EVAL_STRINGS[] = { "tess_eval", "TESS_EVAL" };
		inline constexpr const char* TASK_STRINGS[] = { "task", "TASK" };
		inline constexpr const char* MESH_STRINGS[] = { "mesh", "MESH" };
		inline constexpr const char* RAYGEN_STRINGS[] = { "raygen", "RAYGEN" };
		inline constexpr const char* CLOSEST_HIT_STRINGS[] = { "closest_hit", "CLOSEST_HIT" };
		inline constexpr const char* ANY_HIT_STRINGS[] = { "any_hit", "ANY_HIT" };
		inline constexpr const char* MISS_STRINGS[] = { "miss", "MISS" };
		inline constexpr const char* INTERSECTION_STRINGS[] = { "intersection", "INTERSECTION" };
		inline constexpr const char* CALLABLE_STRINGS[] = { "callable", "CALLABLE" };

		// Filters
		inline constexpr const char* NEAREST_STRINGS[] = { "NEAREST" };
		inline constexpr const char* LINEAR_STRINGS[] = { "LINEAR" };

		// Address modes
		inline constexpr const char* REPEAT_STRINGS[] = { "REPEAT" };
		inline constexpr const char* MIRRORED_REPEAT_STRINGS[] = { "MIRRORED_REPEAT" };
		inline constexpr const char* CLAMP_TO_EDGE_STRINGS[] = { "CLAMP_TO_EDGE" };
		inline constexpr const char* CLAMP_TO_BORDER_STRINGS[] = { "CLAMP_TO_BORDER" };
		inline constexpr const char* MIRROR_CLAMP_TO_EDGE_STRINGS[] = { "MIRROR_CLAMP_TO_EDGE" };

		// Border colors
		inline constexpr const char* FLOAT_TRANSPARENT_BLACK_STRINGS[] = { "FLOAT_TRANSPARENT_BLACK" };
		inline constexpr const char* INT_TRANSPARENT_BLACK_STRINGS[] = { "INT_TRANSPARENT_BLACK" };
		inline constexpr const char* FLOAT_OPAQUE_BLACK_STRINGS[] = { "FLOAT_OPAQUE_BLACK" };
		inline constexpr const char* INT_OPAQUE_BLACK_STRINGS[] = { "INT_OPAQUE_BLACK" };
		inline constexpr const char* FLOAT_OPAQUE_WHITE_STRINGS[] = { "FLOAT_OPAQUE_WHITE" };
		inline constexpr const char* INT_OPAQUE_WHITE_STRINGS[] = { "INT_OPAQUE_WHITE" };

		// Mipmap modes
		inline constexpr const char* MIPMAP_NEAREST_STRINGS[] = { "NEAREST" };
		inline constexpr const char* MIPMAP_LINEAR_STRINGS[] = { "LINEAR" };

		// Descriptor types
		inline constexpr const char* SAMPLER_STRINGS[] = { "SAMPLER" };
		inline constexpr const char* COMBINED_IMAGE_SAMPLER_STRINGS[] = { "COMBINED_IMAGE_SAMPLER" };
		inline constexpr const char* SAMPLED_IMAGE_STRINGS[] = { "SAMPLED_IMAGE" };
		inline constexpr const char* STORAGE_IMAGE_STRINGS[] = { "STORAGE_IMAGE" };
		inline constexpr const char* UNIFORM_TEXEL_BUFFER_STRINGS[] = { "UNIFORM_TEXEL_BUFFER" };
		inline constexpr const char* STORAGE_TEXEL_BUFFER_STRINGS[] = { "STORAGE_TEXEL_BUFFER" };
		inline constexpr const char* UNIFORM_BUFFER_STRINGS[] = { "UNIFORM_BUFFER" };
		inline constexpr const char* STORAGE_BUFFER_STRINGS[] = { "STORAGE_BUFFER" };
		inline constexpr const char* UNIFORM_BUFFER_DYNAMIC_STRINGS[] = { "UNIFORM_BUFFER_DYNAMIC" };
		inline constexpr const char* STORAGE_BUFFER_DYNAMIC_STRINGS[] = { "STORAGE_BUFFER_DYNAMIC" };
		inline constexpr const char* INPUT_ATTACHMENT_STRINGS[] = { "INPUT_ATTACHMENT" };

		// Shader stage flags
		inline constexpr const char* VERTEX_BIT_STRINGS[] = { "VERTEX_BIT" };
		inline constexpr const char* FRAGMENT_BIT_STRINGS[] = { "FRAGMENT_BIT" };
		inline constexpr const char* GEOMETRY_BIT_STRINGS[] = { "GEOMETRY_BIT" };
		inline constexpr const char* COMPUTE_BIT_STRINGS[] = { "COMPUTE_BIT" };
		inline constexpr const char* TESSELLATION_CONTROL_BIT_STRINGS[] = { "TESSELLATION_CONTROL_BIT" };
		inline constexpr const char* TESSELLATION_EVALUATION_BIT_STRINGS[] = { "TESSELLATION_EVALUATION_BIT" };
		inline constexpr const char* TASK_BIT_STRINGS[] = { "TASK_BIT" };
		inline constexpr const char* MESH_BIT_STRINGS[] = { "MESH_BIT" };
		inline constexpr const char* RAYGEN_BIT_STRINGS[] = { "RAYGEN_BIT" };
		inline constexpr const char* CLOSEST_HIT_BIT_STRINGS[] = { "CLOSEST_HIT_BIT" };
		inline constexpr const char* ANY_HIT_BIT_STRINGS[] = { "ANY_HIT_BIT" };
		inline constexpr const char* MISS_BIT_STRINGS[] = { "MISS_BIT" };
		inline constexpr const char* INTERSECTION_BIT_STRINGS[] = { "INTERSECTION_BIT" };
		inline constexpr const char* CALLABLE_BIT_STRINGS[] = { "CALLABLE_BIT" };

		// Formats
		inline constexpr const char* R32_SFLOAT_STRINGS[] = { "R32_SFLOAT" };
		inline constexpr const char* R32G32_SFLOAT_STRINGS[] = { "R32G32_SFLOAT" };
		inline constexpr const char* R32G32B32_SFLOAT_STRINGS[] = { "R32G32B32_SFLOAT" };
		inline constexpr const char* R32G32B32A32_SFLOAT_STRINGS[] = { "R32G32B32A32_SFLOAT" };
		inline constexpr const char* R32_UINT_STRINGS[] = { "R32_UINT" };

		// Vertex input rates
		inline constexpr const char* VERTEX_STRINGS[] = { "VERTEX" };
		inline constexpr const char* INSTANCE_STRINGS[] = { "INSTANCE" };

		// Buffer types
		inline constexpr const char* GPU_ARENA_STRINGS[] = { "GPUArena", "gpuarena", "GPU_ARENA" };
		inline constexpr const char* GPU_STRINGS[] = { "GPU", "gpu" };

		// Buffer usage flags
		inline constexpr const char* COMBINED_VERTEX_INDEX_FLAG_STRINGS[] = { "CombinedVertexIndexFlag", "COMBINED_VERTEX_INDEX_FLAG" };
		inline constexpr const char* INSTANCE_FLAG_STRINGS[] = { "InstanceFlag", "INSTANCE_FLAG" };
		inline constexpr const char* STORAGE_FLAG_STRINGS[] = { "StorageFlag", "STORAGE_FLAG" };
		inline constexpr const char* UNIFORM_FLAG_STRINGS[] = { "UniformFlag", "UNIFORM_FLAG" };
		inline constexpr const char* VERTEX_FLAG_STRINGS[] = { "VertexFlag", "VERTEX_FLAG" };
		inline constexpr const char* INDEX_FLAG_STRINGS[] = { "IndexFlag", "INDEX_FLAG" };

		// Memory property flags
		inline constexpr const char* GPU_LOCAL_STRINGS[] = { "GPULocal", "GPU_LOCAL" };
		inline constexpr const char* CPU_HOST_COMPATIBLE_STRINGS[] = { "host_compatible" "HOST_COMPATIBLE" };
	}

	Yaml::Node& GetSamplers(Yaml::Node& Root);
	Yaml::Node& GetShaders(Yaml::Node& Root);
	Yaml::Node& GetDescriptorSetLayouts(Yaml::Node& Root);
	Yaml::Node& ParseDescriptorSetLayoutNode(Yaml::Node& Root);
	Yaml::Node& GetVertices(Yaml::Node& Root);
	Yaml::Node& GetVertexBindingNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributesNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributeTypeNode(Yaml::Node& AttributeNode);
	Yaml::Node& GetPushConstantsFromResources(Yaml::Node& Root);

	// Push constant management
	void ParseAndCreatePushConstants(Yaml::Node& PushConstantsNode);

	Yaml::Node& GetSceneResources(Yaml::Node& Root);
	Yaml::Node& GetTextures(Yaml::Node& Root);
	Yaml::Node& GetModels(Yaml::Node& Root);

	std::string GetModelPath(Yaml::Node& ModelNode);
	glm::vec3 GetModelPosition(Yaml::Node& ModelNode);

	std::string ParseNameNode(Yaml::Node& Node);
	std::string ParseShaderNode(Yaml::Node& ShaderNode);
	BmRender_PipelineShaderStage ParseShaderPipelineStage(Yaml::Node& ShaderNode);
	BmRHI_SamplerDescription ParseSamplerNode(Yaml::Node& SamplerNode);
	BmRender_DescriptorSetLayoutBinding ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode);
	void ParseVertexAttributeNode(Yaml::Node& AttributeNode, VertexAttribute* OutAttribute, std::string* OutAttributeName);
	VertexBinding_depr ParseVertexBindingNode(Yaml::Node& BindingNode);

	BmRender_PipelineShaderStage ParseShaderStage(const char* Value, u32 Length);

	BmRender_Filter ParseFilter(const char* Value, u32 Length);
	BmRender_SamplerAddressMode ParseAddressMode(const char* Value, u32 Length);
	BmRender_BorderColor ParseBorderColor(const char* Value, u32 Length);
	BmRender_SamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length);
	BmRender_DescriptorType ParseDescriptorType(const char* Value, u32 Length);
	BmRender_DescriptorShaderStage ParseShaderStageFlags(const char* Value, u32 Length);

	BmRender_AttributeType ParseShaderTypeToAttributeType(const char* Value, u32 Length);
	BmRender_VertexInputRate ParseVertexInputRate(const char* Value, u32 Length);

	MemoryPropertyFlag ParseMemoryPropertyFlag(const char* Value, u32 Length);
	std::string GetBufferName(Yaml::Node& BufferNode);

	ShaderType ParseShaderType(const char* Value, u32 Length);
	std::vector<DescriptorSetLayout> ParseDescriptorSetLayouts(Yaml::Node& DescriptorSetLayoutsNode);
}
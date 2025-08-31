#pragma once

#include <string>
#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Engine/Systems/Render/RenderResources.h"

namespace Util
{
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
		inline constexpr const char* VERTEX_SHADER_STRINGS[] = { "vertex", "VERTEX" };
		inline constexpr const char* FRAGMENT_STRINGS[] = { "fragment", "FRAGMENT" };
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
	}

	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData);
	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode);

	enum class LogType
	{
		Error,
		Warning,
		Info
	};

	void RenderLog(LogType logType, const char* format, ...);
	void RenderLog(LogType LogType, const char* Format, va_list Args);

	struct Log
	{
		template <typename... TArgs>
		static void Error(std::string_view Message, TArgs&&... Args)
		{
			std::cout << "\033[31;5m"; // Set red color
			Print("Error", Message, Args...);
			std::cout << "\033[m"; // Reset red color
		}

		template <typename... TArgs>
		static void Warning(std::string_view Message, TArgs&&... Args)
		{
			std::cout << "\033[33;5m";
			Print("Warning", Message, Args...);
			std::cout << "\033[m";
		}

		template <typename... TArgs>
		static void Info(std::string_view Message, TArgs&&... Args)
		{
			Print("Info", Message, Args...);
		}

		static void GlfwLogError()
		{
			const char* ErrorDescription;
			const int LastErrorCode = glfwGetError(&ErrorDescription);

			const std::string_view ErrorLog = LastErrorCode != GLFW_NO_ERROR ? ErrorDescription : "No GLFW error";
			Error(ErrorLog);
		}

		template <typename... TArgs>
		static void Print(std::string_view Type, std::string_view Message, TArgs&&... Args)
		{
			//std::cout << std::format("{}: {}\n", Type, std::vformat(Message, std::make_format_args(Args...)));
		}
	};

	Yaml::Node& GetSamplers(Yaml::Node& Root);
	Yaml::Node& GetShaders(Yaml::Node& Root);
	Yaml::Node& GetDescriptorSetLayouts(Yaml::Node& Root);
	Yaml::Node& ParseDescriptorSetLayoutNode(Yaml::Node& Root);
	Yaml::Node& GetVertices(Yaml::Node& Root);
	Yaml::Node& GetVertexBindingNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributesNode(Yaml::Node& VertexNode);
	Yaml::Node& GetVertexAttributeFormatNode(Yaml::Node& AttributeNode);
	Yaml::Node& GetPipelineNode(Yaml::Node& Root);
	Yaml::Node& GetPipelineShadersNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineRasterizationNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineColorBlendStateNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineColorBlendAttachmentNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineDepthStencilNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineMultisampleNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineInputAssemblyNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetVertexAttributeLayoutNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetPipelineViewportStateNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetViewportNode(Yaml::Node& PipelineNode);
	Yaml::Node& GetScissorNode(Yaml::Node& PipelineNode);
	
	Yaml::Node& GetSceneResources(Yaml::Node& Root);
	Yaml::Node& GetTextures(Yaml::Node& Root);
	Yaml::Node& GetModels(Yaml::Node& Root);

	std::string ParseNameNode(Yaml::Node& Node);
	std::string ParseShaderNode(Yaml::Node& ShaderNode);
	VkSamplerCreateInfo ParseSamplerNode(Yaml::Node& SamplerNode);
	VkDescriptorSetLayoutBinding ParseDescriptorSetLayoutBindingNode(Yaml::Node& BindingNode);
	void ParseVertexAttributeNode(Yaml::Node& AttributeNode, VulkanHelper::VertexAttribute* OutAttribute, std::string* OutAttributeName);
	VulkanHelper::VertexBinding ParseVertexBindingNode(Yaml::Node& BindingNode);
	VkPipelineRasterizationStateCreateInfo ParsePipelineRasterizationNode(Yaml::Node& RasterizationNode);
	VkPipelineColorBlendStateCreateInfo ParsePipelineColorBlendStateNode(Yaml::Node& ColorBlendStateNode);
	VkPipelineColorBlendAttachmentState ParsePipelineColorBlendAttachmentNode(Yaml::Node& ColorBlendAttachmentNode);
	VkPipelineDepthStencilStateCreateInfo ParsePipelineDepthStencilNode(Yaml::Node& DepthStencilNode);
	VkPipelineMultisampleStateCreateInfo ParsePipelineMultisampleNode(Yaml::Node& MultisampleNode);
	VkPipelineInputAssemblyStateCreateInfo ParsePipelineInputAssemblyNode(Yaml::Node& InputAssemblyNode);
	VkPipelineViewportStateCreateInfo ParsePipelineViewportStateNode(Yaml::Node& ViewportStateNode);
	VkViewport ParseViewportNode(Yaml::Node& ViewportNode);
	VkRect2D ParseScissorNode(Yaml::Node& ScissorNode);

	VkBool32 ParseBool(const char* Value, u32 Length);
	VkPolygonMode ParsePolygonMode(const char* Value, u32 Length);
	VkCullModeFlags ParseCullMode(const char* Value, u32 Length);
	VkFrontFace ParseFrontFace(const char* Value, u32 Length);
	VkColorComponentFlags ParseColorWriteMask(const char* Value, u32 Length);
	VkBlendFactor ParseBlendFactor(const char* Value, u32 Length);
	VkBlendOp ParseBlendOp(const char* Value, u32 Length);
	VkCompareOp ParseCompareOp(const char* Value, u32 Length);
	VkSampleCountFlagBits ParseSampleCount(const char* Value, u32 Length);
	VkPrimitiveTopology ParseTopology(const char* Value, u32 Length);
	VkShaderStageFlagBits ParseShaderStage(const char* Value, u32 Length);

	VkFilter ParseFilter(const char* Value, u32 Length);
	VkSamplerAddressMode ParseAddressMode(const char* Value, u32 Length);
	VkBorderColor ParseBorderColor(const char* Value, u32 Length);
	VkSamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length);
	VkDescriptorType ParseDescriptorType(const char* Value, u32 Length);
	VkShaderStageFlags ParseShaderStageFlags(const char* Value, u32 Length);

	VkFormat ParseFormat(const char* Value, u32 Length);
	VkVertexInputRate ParseVertexInputRate(const char* Value, u32 Length);
	u32 CalculateFormatSize(VkFormat Format);
	u32 CalculateFormatSizeFromString(const char* FormatString, u32 FormatLength);


#ifdef NDEBUG
	static bool EnableValidationLayers = false;
#else
	static bool EnableValidationLayers = true;
#endif

	typedef u8* Model3DData;

	struct Model3DMaterial
	{
		u64 DiffuseTextureHash;
		u64 SpecularTextureHash;
	};

	struct Model3DFileHeader
	{
		u64 VertexDataSize;
		u32 MeshCount;
		u32 MaterialCount;
		u32 UniqueTextureCount;
	};

	struct Model3D
	{
		Model3DFileHeader Header;

		u8* VertexData;
		u64* VerticesCounts;
		u32* IndicesCounts;
		u32* MaterialIndices;
		Model3DMaterial* Materials;
		u64* UniqueTextureHashes;
	};

	void ObjToModel3D(const char* FilePath, const char* OutputPath);

	Model3DData LoadModel3DData(const char* FilePath);
	void ClearModel3DData(Model3DData Data);

	Model3D ParseModel3D(Model3DData Data);
}
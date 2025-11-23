#include "YamlParsing.h"

#include <SharedLib.h>

#include <Engine/Systems/Memory/MemoryManagmentSystem.h>

extern std::unordered_map<std::string, Util::VertexBinding_depr> VBindings;
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

	BmRender_RasterizationState ParsePipelineRasterizationNode(Yaml::Node& RasterizationNode)
	{
		BmRender_RasterizationState OutRasterizationState = { };

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
			OutRasterizationState.lineWidth = RasterizationNode["lineWidth"].As<f32>();
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

	BmRender_ColorBlendState ParsePipelineColorBlendStateNode(Yaml::Node& ColorBlendStateNode)
	{
		BmRender_ColorBlendState OutColorBlendState = { };

		if (!ColorBlendStateNode["logicOpEnable"].IsNone())
			OutColorBlendState.logicOpEnable = ColorBlendStateNode["logicOpEnable"].As<bool>();
		if (!ColorBlendStateNode["attachmentCount"].IsNone())
			OutColorBlendState.attachmentCount = ColorBlendStateNode["attachmentCount"].As<u32>();

		return OutColorBlendState;
	}

	BmRender_ColorBlendAttachment ParsePipelineColorBlendAttachmentNode(Yaml::Node& ColorBlendAttachmentNode)
	{
		BmRender_ColorBlendAttachment OutColorBlendAttachment = { };

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

	BmRender_DepthStencilState ParsePipelineDepthStencilNode(Yaml::Node& DepthStencilNode)
	{
		BmRender_DepthStencilState OutDepthStencilState = { };

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

	BmRender_MultisampleState ParsePipelineMultisampleNode(Yaml::Node& MultisampleNode)
	{
		BmRender_MultisampleState OutMultisampleState = { };

		if (!MultisampleNode["sampleShadingEnable"].IsNone())
			OutMultisampleState.sampleShadingEnable = MultisampleNode["sampleShadingEnable"].As<bool>();
		if (!MultisampleNode["rasterizationSamples"].IsNone())
		{
			std::string rasterizationSamplesStr = MultisampleNode["rasterizationSamples"].As<std::string>();
			OutMultisampleState.rasterizationSamples = ParseSampleCount(rasterizationSamplesStr.c_str(), rasterizationSamplesStr.length());
		}

		return OutMultisampleState;
	}

	BmRender_InputAssemblyState ParsePipelineInputAssemblyNode(Yaml::Node& InputAssemblyNode)
	{
		BmRender_InputAssemblyState OutInputAssemblyState = { };

		if (!InputAssemblyNode["topology"].IsNone())
		{
			std::string topologyStr = InputAssemblyNode["topology"].As<std::string>();
			OutInputAssemblyState.topology = ParseTopology(topologyStr.c_str(), topologyStr.length());
		}
		if (!InputAssemblyNode["primitiveRestartEnable"].IsNone())
			OutInputAssemblyState.primitiveRestartEnable = InputAssemblyNode["primitiveRestartEnable"].As<bool>();

		return OutInputAssemblyState;
	}

	BmRender_ViewportState ParsePipelineViewportStateNode(Yaml::Node& ViewportStateNode)
	{
		BmRender_ViewportState OutViewportState = { };
		if (!ViewportStateNode["viewportCount"].IsNone())
			OutViewportState.viewportCount = ViewportStateNode["viewportCount"].As<u32>();
		if (!ViewportStateNode["scissorCount"].IsNone())
			OutViewportState.scissorCount = ViewportStateNode["scissorCount"].As<u32>();
		return OutViewportState;
	}

	BmRender_Viewport ParseViewportNode(Yaml::Node& ViewportNode)
	{
		BmRender_Viewport OutViewport = { };
		if (!ViewportNode["minDepth"].IsNone())
			OutViewport.MinDepth = ViewportNode["minDepth"].As<f32>();
		if (!ViewportNode["maxDepth"].IsNone())
			OutViewport.MaxDepth = ViewportNode["maxDepth"].As<f32>();
		if (!ViewportNode["x"].IsNone())
			OutViewport.X = ViewportNode["x"].As<f32>();
		if (!ViewportNode["y"].IsNone())
			OutViewport.Y = ViewportNode["y"].As<f32>();
		return OutViewport;
	}

	BmRender_Rect2D ParseScissorNode(Yaml::Node& ScissorNode)
	{
		BmRender_Rect2D OutScissor = { };
		if (!ScissorNode["offsetX"].IsNone())
			OutScissor.Offset.X = ScissorNode["offsetX"].As<s32>();
		if (!ScissorNode["offsetY"].IsNone())
			OutScissor.Offset.Y = ScissorNode["offsetY"].As<s32>();
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

	bool ParseBool(const char* Value, u32 Length)
	{
		if (Length == 0) return VK_FALSE;

		if (StringMatches(Value, Length, ParseStrings::TRUE_STRINGS)) return VK_TRUE;
		if (StringMatches(Value, Length, ParseStrings::FALSE_STRINGS)) return VK_FALSE;

		return VK_FALSE;
	}

	BmRender_PolygonMode ParsePolygonMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FILL_STRINGS)) return BmRender_PolygonMode::Fill;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRINGS)) return BmRender_PolygonMode::Line;
		if (StringMatches(Value, Length, ParseStrings::POINT_STRINGS)) return BmRender_PolygonMode::Point;

		return BmRender_PolygonMode::Fill;
	}

	BmRender_CullModeFlags ParseCullMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NONE_STRINGS)) return BmRender_CullModeFlags::None;
		if (StringMatches(Value, Length, ParseStrings::BACK_STRINGS)) return BmRender_CullModeFlags::Back;
		if (StringMatches(Value, Length, ParseStrings::FRONT_STRINGS)) return BmRender_CullModeFlags::Front;
		if (StringMatches(Value, Length, ParseStrings::FRONT_BACK_STRINGS)) return BmRender_CullModeFlags::FrontAndBack;

		return BmRender_CullModeFlags::Back;
	}

	BmRender_FrontFace ParseFrontFace(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::COUNTER_CLOCKWISE_STRINGS)) return BmRender_FrontFace::CounterClockwise;
		if (StringMatches(Value, Length, ParseStrings::CLOCKWISE_STRINGS)) return BmRender_FrontFace::Clockwise;

		return BmRender_FrontFace::CounterClockwise;
	}

	BmRender_ColorComponentFlags ParseColorWriteMask(const char* Value, u32 Length)
	{
		BmRender_ColorComponentFlags Flags = BmRender_ColorComponentFlags::None;

		for (u32 i = 0; i < Length; ++i)
		{
			if (Value[i] == 'R' || Value[i] == 'r') Flags = static_cast<BmRender_ColorComponentFlags>(static_cast<u32>(Flags) | static_cast<u32>(BmRender_ColorComponentFlags::R));
			if (Value[i] == 'G' || Value[i] == 'g') Flags = static_cast<BmRender_ColorComponentFlags>(static_cast<u32>(Flags) | static_cast<u32>(BmRender_ColorComponentFlags::G));
			if (Value[i] == 'B' || Value[i] == 'b') Flags = static_cast<BmRender_ColorComponentFlags>(static_cast<u32>(Flags) | static_cast<u32>(BmRender_ColorComponentFlags::B));
			if (Value[i] == 'A' || Value[i] == 'a') Flags = static_cast<BmRender_ColorComponentFlags>(static_cast<u32>(Flags) | static_cast<u32>(BmRender_ColorComponentFlags::A));
		}

		if (Flags == BmRender_ColorComponentFlags::None) Flags = BmRender_ColorComponentFlags::RGBA;

		return Flags;
	}

	BmRender_BlendFactor ParseBlendFactor(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_ALPHA_STRINGS)) return BmRender_BlendFactor::OneMinusSrcAlpha;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_ALPHA_STRINGS)) return BmRender_BlendFactor::OneMinusDstAlpha;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_COLOR_STRINGS)) return BmRender_BlendFactor::OneMinusSrcColor;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_COLOR_STRINGS)) return BmRender_BlendFactor::OneMinusDstColor;
		if (StringMatches(Value, Length, ParseStrings::SRC_COLOR_STRINGS)) return BmRender_BlendFactor::SrcColor;
		if (StringMatches(Value, Length, ParseStrings::DST_COLOR_STRINGS)) return BmRender_BlendFactor::DstColor;
		if (StringMatches(Value, Length, ParseStrings::SRC_ALPHA_STRINGS)) return BmRender_BlendFactor::SrcAlpha;
		if (StringMatches(Value, Length, ParseStrings::DST_ALPHA_STRINGS)) return BmRender_BlendFactor::DstAlpha;
		if (StringMatches(Value, Length, ParseStrings::ZERO_STRINGS)) return BmRender_BlendFactor::Zero;
		if (StringMatches(Value, Length, ParseStrings::ONE_STRINGS)) return BmRender_BlendFactor::One;

		return BmRender_BlendFactor::SrcAlpha;
	}

	BmRender_BlendOp ParseBlendOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REVERSE_SUBTRACT_STRINGS)) return BmRender_BlendOp::ReverseSubtract;
		if (StringMatches(Value, Length, ParseStrings::SUBTRACT_STRINGS)) return BmRender_BlendOp::Subtract;
		if (StringMatches(Value, Length, ParseStrings::ADD_STRINGS)) return BmRender_BlendOp::Add;
		if (StringMatches(Value, Length, ParseStrings::MIN_STRINGS)) return BmRender_BlendOp::Min;
		if (StringMatches(Value, Length, ParseStrings::MAX_STRINGS)) return BmRender_BlendOp::Max;

		return BmRender_BlendOp::Add;
	}

	BmRender_CompareOp ParseCompareOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEVER_STRINGS)) return BmRender_CompareOp::Never;
		if (StringMatches(Value, Length, ParseStrings::LESS_STRINGS)) return BmRender_CompareOp::Less;
		if (StringMatches(Value, Length, ParseStrings::EQUAL_STRINGS)) return BmRender_CompareOp::Equal;
		if (StringMatches(Value, Length, ParseStrings::LESS_OR_EQUAL_STRINGS)) return BmRender_CompareOp::LessOrEqual;
		if (StringMatches(Value, Length, ParseStrings::GREATER_STRINGS)) return BmRender_CompareOp::Greater;
		if (StringMatches(Value, Length, ParseStrings::NOT_EQUAL_STRINGS)) return BmRender_CompareOp::NotEqual;
		if (StringMatches(Value, Length, ParseStrings::GREATER_OR_EQUAL_STRINGS)) return BmRender_CompareOp::GreaterOrEqual;
		if (StringMatches(Value, Length, ParseStrings::ALWAYS_STRINGS)) return BmRender_CompareOp::Always;

		return BmRender_CompareOp::Less;
	}

	BmRender_SampleCount ParseSampleCount(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_1_STRINGS)) return BmRender_SampleCount::Count1;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_2_STRINGS)) return BmRender_SampleCount::Count2;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_4_STRINGS)) return BmRender_SampleCount::Count4;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_8_STRINGS)) return BmRender_SampleCount::Count8;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_16_STRINGS)) return BmRender_SampleCount::Count16;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_32_STRINGS)) return BmRender_SampleCount::Count32;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_64_STRINGS)) return BmRender_SampleCount::Count64;

		return BmRender_SampleCount::Count1;
	}

	BmRender_PrimitiveTopology ParseTopology(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::POINT_LIST_STRINGS)) return BmRender_PrimitiveTopology::PointList;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_STRINGS)) return BmRender_PrimitiveTopology::LineList;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_STRINGS)) return BmRender_PrimitiveTopology::LineStrip;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_STRINGS)) return BmRender_PrimitiveTopology::TriangleList;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_STRINGS)) return BmRender_PrimitiveTopology::TriangleStrip;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_FAN_STRINGS)) return BmRender_PrimitiveTopology::TriangleFan;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_WITH_ADJACENCY_STRINGS)) return BmRender_PrimitiveTopology::LineListWithAdjacency;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_WITH_ADJACENCY_STRINGS)) return BmRender_PrimitiveTopology::LineStripWithAdjacency;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_WITH_ADJACENCY_STRINGS)) return BmRender_PrimitiveTopology::TriangleListWithAdjacency;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_WITH_ADJACENCY_STRINGS)) return BmRender_PrimitiveTopology::TriangleStripWithAdjacency;
		if (StringMatches(Value, Length, ParseStrings::PATCH_LIST_STRINGS)) return BmRender_PrimitiveTopology::PatchList;

		return BmRender_PrimitiveTopology::TriangleList;
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
			BmRender_DescriptorShaderStage combinedStageFlags = BmRender_DescriptorShaderStage::None;

			// Calculate total size and combine stage flags
			for (auto TypeIt = ConstantNode.Begin(); TypeIt != ConstantNode.End(); TypeIt++)
			{
				Yaml::Node& TypeNode = (*TypeIt).second;
				std::string TypeName = TypeNode["type"].As<std::string>();

				u32 Size = 0;
				BmRender_DescriptorShaderStage StageFlags = BmRender_DescriptorShaderStage::None;

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
				combinedStageFlags = (BmRender_DescriptorShaderStage)((u64)combinedStageFlags | (u64)StageFlags);
			}

			// Create a single push constant handle for the entire definition
			BmRender_PushConstant PushConstantHandle = BmRender_CreatePushConstant(
				combinedStageFlags,
				0, // offset starts at 0 for the entire push constant
				totalSize
			);

			// Store the single handle
			PushConstants[ConstantName] = PushConstantHandle;
		}
	}

	BmRender_PipelineDescription ParsePipelineFromYaml(const std::string& YamlFilePath, BmRender_Extent2D Extent, const PipelineResourceInfo& ResourceInfo,
		std::vector<BmRender_ShaderStageDescription>& ShaderStages,
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
			BmRender_ShaderStageDescription ShaderStage = { };
			ShaderStage.Shader = Shaders[(*it).second.As<std::string>()];
			ShaderStage.EntryPointFunction = "main";
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
				BmRenderVertexBinding.Attributes = (VertexAttribute*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(VertexAttribute) * BmRenderVertexBinding.AttributesCount);

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
		BmRender_PipelineDescription Description = { };
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
		Description.Viewport.Width = Extent.Width;
		Description.Viewport.Height = Extent.Height;

		Description.Scissor = ParseScissorNode(ScissorNode);
		Description.Scissor.Extent.Width = Extent.Width;
		Description.Scissor.Extent.Height = Extent.Height;

		return Description;
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

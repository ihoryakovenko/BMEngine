#pragma once

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

namespace BMR
{
	static const u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	static const u32 MAX_VERTEX_INPUT_BINDINGS = 16;

	enum BMRLogType
	{
		LogType_Error,
		LogType_Warning,
		LogType_Info
	};

	typedef void (*BMRLogHandler)(BMRLogType, const char*, va_list);

	enum BMRShaderNames
	{
		TerrainVertex,
		TerrainFragment,
		EntityVertex,
		EntityFragment,
		DeferredVertex,
		DeferredFragment,
		SkyBoxVertex,
		SkyBoxFragment,
		DepthVertex,

		ShaderNamesCount
	};

	enum BMRShaderStages
	{
		Vertex,
		Fragment,

		ShaderStagesCount,
		BMRShaderStagesNone
	};

	enum BMRPipelineHandles
	{
		Terrain = 0,
		Entity,
		Deferred,
		SkyBox,
		Depth,

		PipelineHandlesCount,
		PipelineHandlesNone
	};

	enum BMRTextureType
	{
		TextureType_2D,
		TextureType_CUBE
	};

	enum BMRPolygonMode
	{
		Fill = 0,        // Corresponds to VK_POLYGON_MODE_FILL
		Line = 1,        // Corresponds to VK_POLYGON_MODE_LINE
		Point = 2,       // Corresponds to VK_POLYGON_MODE_POINT
		BMRPolygonMode_Max = 3          // Invalid or uninitialized state
	};

	enum BMRCullMode
	{
		None = 0,               // Corresponds to VK_CULL_MODE_NONE
		Front = 1,              // Corresponds to VK_CULL_MODE_FRONT_BIT
		Back = 2,               // Corresponds to VK_CULL_MODE_BACK_BIT
		FrontAndBack = 3,       // Corresponds to VK_CULL_MODE_FRONT_AND_BACK
		BMRCullMode_Max = 4                 // Invalid or uninitialized state
	};

	enum BMRFrontFace
	{
		CounterClockwise = 0,  // Corresponds to VK_FRONT_FACE_COUNTER_CLOCKWISE
		Clockwise = 1,         // Corresponds to VK_FRONT_FACE_CLOCKWISE
		BMRFrontFace_Max = 2                // Invalid or uninitialized state
	};

	enum BMRBlendFactor
	{
		Zero = 0,               // Corresponds to VK_BLEND_FACTOR_ZERO
		One = 1,                // Corresponds to VK_BLEND_FACTOR_ONE
		SrcAlpha = 2,           // Corresponds to VK_BLEND_FACTOR_SRC_ALPHA
		DstAlpha = 3,           // Corresponds to VK_BLEND_FACTOR_DST_ALPHA
		OneMinusSrcAlpha = 4,   // Corresponds to VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
		BMRBlendFactor_Max = 5                 // Invalid or uninitialized state
	};

	enum BMRBlendOp
	{
		Add = 0,                // Corresponds to VK_BLEND_OP_ADD
		Subtract = 1,           // Corresponds to VK_BLEND_OP_SUBTRACT
		ReverseSubtract = 2,    // Corresponds to VK_BLEND_OP_REVERSE_SUBTRACT
		Min = 3,                // Corresponds to VK_BLEND_OP_MIN
		BMRBlendOp_Max = 4                 // Corresponds to VK_BLEND_OP_MAX
	};

	enum BMRCompareOp
	{
		Never = 0,              // Corresponds to VK_COMPARE_OP_NEVER
		Less = 1,               // Corresponds to VK_COMPARE_OP_LESS
		Equal = 2,              // Corresponds to VK_COMPARE_OP_EQUAL
		LessOrEqual = 3,        // Corresponds to VK_COMPARE_OP_LESS_OR_EQUAL
		Greater = 4,            // Corresponds to VK_COMPARE_OP_GREATER
		BMRCompareOp_Max = 5                 // Invalid or uninitialized state
	};

	enum BMRColorComponentFlagBits
	{
		R = 0x00000001,        // Corresponds to VK_COLOR_COMPONENT_R_BIT
		G = 0x00000002,        // Corresponds to VK_COLOR_COMPONENT_G_BIT
		B = 0x00000004,        // Corresponds to VK_COLOR_COMPONENT_B_BIT
		A = 0x00000008,        // Corresponds to VK_COLOR_COMPONENT_A_BIT
		All = R | G | B | A,   // Represents all color components
		BMRColorComponentFlagBits_MaxEnum = 0x7FFFFFFF   // Invalid or uninitialized state
	};

	enum BMRVertexInputRate
	{
		BMRVertexInputRate_Vertex = 0,           // Corresponds to VK_VERTEX_INPUT_RATE_VERTEX
		BMRVertexInputRate_Instance = 1,         // Corresponds to VK_VERTEX_INPUT_RATE_INSTANCE
		BMRVertexInputRate_Max = 0x7FFFFFFF // Invalid or uninitialized state
	};

	enum BMRFormat
	{
		R32_SF,
		R32G32_SF,
		R32G32B32_SF,
		BMRFormat_Max
	};

	static const u32 BMRFormatSizesTable[] =
	{
		sizeof(float),
		sizeof(float) * 2,
		sizeof(float) * 3,
	};

	struct BMRShaderCodeDescription
	{
		BMRPipelineHandles Handle = BMRPipelineHandles::PipelineHandlesNone;
		BMRShaderStages Stage = BMRShaderStages::BMRShaderStagesNone;
		u32* Code = nullptr;
		u32 CodeSize = 0;
	};

	struct BMRConfig
	{
		BMRLogHandler LogHandler = nullptr;
		bool EnableValidationLayers = false;
		BMRShaderCodeDescription RenderShaders[BMRShaderNames::ShaderNamesCount];
		u32 MaxTextures = 0;
	};

	typedef glm::mat4 BMRModel;

	struct BMRUboViewProjection
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	typedef glm::mat4 BMRLightSpaceMatrix;

	struct BMRTextureArrayInfo
	{
		u32 Width = 0;
		u32 Height = 0;
		u32 Format = 0;
		u32 LayersCount = 0;
		u8** Data = nullptr;
	};

	struct BMRDrawEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		u32 MaterialIndex = -1;
		BMRModel Model;
	};

	struct BMRDrawTerrainEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		u32 MaterialIndex = -1;
	};

	struct BMRDrawSkyBoxEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		u32 MaterialIndex = -1;
	};

	struct BMRPointLight
	{
		glm::vec4 Position;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
	};

	struct BMRDirectionLight
	{
		BMRLightSpaceMatrix LightSpaceMatrix;
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct BMRSpotLight
	{
		BMRLightSpaceMatrix LightSpaceMatrix;
		glm::vec3 Position;
		f32 CutOff;
		glm::vec3 Direction;
		f32 OuterCutOff;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
		alignas(16) glm::vec2 Planes;
	};

	struct BMRLightBuffer
	{
		BMRPointLight PointLight;
		BMRDirectionLight DirectionLight;
		BMRSpotLight SpotLight;
	};

	struct BMRDrawScene
	{
		BMRUboViewProjection ViewProjection;

		BMRDrawEntity* DrawEntities = nullptr;
		u32 DrawEntitiesCount = 0;

		BMRDrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		BMRDrawTerrainEntity* DrawTerrainEntities = nullptr;
		u32 DrawTerrainEntitiesCount = 0;

		BMRDrawSkyBoxEntity SkyBox;
		bool DrawSkyBox = false;

		BMRLightBuffer* LightEntity = nullptr;
	};

	struct BMRMaterial
	{
		f32 Shininess;
	};

	struct BMRExtent2D
	{
		u32 Width = 0;
		u32 Height = 0;
	};

	struct BMRVertexInputAttribute
	{
		const char* VertexInputAttributeName = nullptr;
		BMRFormat Format = BMRFormat::BMRFormat_Max;
	};

	struct BMRVertexInputBinding
	{
		const char* VertexInputBindingName = nullptr;

		BMRVertexInputAttribute InputAttributes[MAX_VERTEX_INPUTS_ATTRIBUTES];
		u32 InputAttributesCount = 0;

		u32 Stride = 0;
		BMRVertexInputRate InputRate = BMRVertexInputRate::BMRVertexInputRate_Max;
	};

	struct BMRVertexInput
	{
		BMRVertexInputBinding VertexInputBinding[1];
		u32 VertexInputBindingCount = 0;
	};

	struct BMRPipelineSettings
	{
		const char* PipelineName = nullptr;

		BMRExtent2D Extent;

		bool DepthClampEnable = false;
		bool RasterizerDiscardEnable = false;
		BMRPolygonMode PolygonMode = BMRPolygonMode::BMRPolygonMode_Max;
		f32 LineWidth = 0.0f;
		BMRCullMode CullMode = BMRCullMode::BMRCullMode_Max;
		BMRFrontFace FrontFace = BMRFrontFace::BMRFrontFace_Max;
		bool DepthBiasEnable = false;

		bool LogicOpEnable = false;
		u32 AttachmentCount = 0;
		u32 ColorWriteMask = 0;
		bool BlendEnable = false;
		BMRBlendFactor SrcColorBlendFactor = BMRBlendFactor::BMRBlendFactor_Max;
		BMRBlendFactor DstColorBlendFactor = BMRBlendFactor::BMRBlendFactor_Max;
		BMRBlendOp ColorBlendOp = BMRBlendOp::BMRBlendOp_Max;
		BMRBlendFactor SrcAlphaBlendFactor = BMRBlendFactor::BMRBlendFactor_Max;
		BMRBlendFactor DstAlphaBlendFactor = BMRBlendFactor::BMRBlendFactor_Max;
		BMRBlendOp AlphaBlendOp = BMRBlendOp::BMRBlendOp_Max;

		bool DepthTestEnable = false;
		bool DepthWriteEnable = false;
		BMRCompareOp DepthCompareOp = BMRCompareOp::BMRCompareOp_Max;
		bool DepthBoundsTestEnable = false;
		bool StencilTestEnable = false;
	};
}

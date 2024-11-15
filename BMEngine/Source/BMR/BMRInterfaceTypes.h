#pragma once

#include <vulkan\vulkan.h>

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

	enum BMRShaderStage
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

	struct BMRShaderCodeDescription
	{
		BMRPipelineHandles Handle = BMRPipelineHandles::PipelineHandlesNone;
		BMRShaderStage Stage = BMRShaderStage::BMRShaderStagesNone;
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

	struct BMRMaterial
	{
		f32 Shininess;
	};

	struct BMRVertexInputAttribute
	{
		const char* VertexInputAttributeName = nullptr;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		u32 AttributeOffset = 0;
	};

	struct BMRVertexInputBinding
	{
		const char* VertexInputBindingName = nullptr;

		BMRVertexInputAttribute InputAttributes[MAX_VERTEX_INPUTS_ATTRIBUTES];
		u32 InputAttributesCount = 0;

		u32 Stride = 0;
		VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;
	};

	struct BMRVertexInput
	{
		BMRVertexInputBinding VertexInputBinding[MAX_VERTEX_INPUT_BINDINGS];
		u32 VertexInputBindingCount = 0;
	};

	struct BMRUniform
	{
		union
		{
			VkBuffer Buffer = nullptr;
			VkImage Image;
		};

		VkDeviceMemory Memory = nullptr;
	};

	struct BMRUniformSetAttachmentInfo
	{
		union
		{
			VkDescriptorBufferInfo BufferInfo;
			VkDescriptorImageInfo ImageInfo;
		};

		VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	};

	typedef VkDescriptorSet BMRUniformSet;
	typedef VkDescriptorSetLayout BMRUniformLayout;
	typedef VkRenderPass BMRRenderPass;
	typedef VkImageView BMRImageInterface;

	struct BMRPipeline
	{
		VkPipeline Pipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
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

	struct BMRPipelineSettings
	{
		const char* PipelineName = nullptr;

		VkExtent2D Extent;

		VkBool32 DepthClampEnable = VK_FALSE;
		VkBool32 RasterizerDiscardEnable = VK_FALSE;
		VkPolygonMode PolygonMode = VK_POLYGON_MODE_MAX_ENUM;
		f32 LineWidth = 0.0f;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_MAX_ENUM;
		VkBool32 DepthBiasEnable = VK_FALSE;

		VkBool32 LogicOpEnable = VK_FALSE;
		u32 AttachmentCount = 0;
		u32 ColorWriteMask = 0;
		VkBool32 BlendEnable = VK_FALSE;
		VkBlendFactor SrcColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp ColorBlendOp = VK_BLEND_OP_MAX_ENUM;
		VkBlendFactor SrcAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp AlphaBlendOp = VK_BLEND_OP_MAX_ENUM;

		VkBool32 DepthTestEnable = VK_FALSE;
		VkBool32 DepthWriteEnable = VK_FALSE;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_MAX_ENUM;
		VkBool32 DepthBoundsTestEnable = VK_FALSE;
		VkBool32 StencilTestEnable = VK_FALSE;
	};

	struct BMRSubpassSettings
	{
		const char* SubpassName = nullptr;

		const VkAttachmentReference* ColorAttachmentsReferences = nullptr;
		u32 ColorAttachmentsReferencesCount = 0;

		const VkAttachmentReference* DepthAttachmentReferences = nullptr;

		// First subpass has to not contain InputAttachments
		const VkAttachmentReference* InputAttachmentsReferences = nullptr;
		u32 InputAttachmentsReferencesCount = 0;
	};

	struct BMRRenderPassSettings
	{
		const char* RenderPassName = nullptr;

		// AttachmentDescriptions can be modified if contain undefined format
		VkAttachmentDescription* AttachmentDescriptions = nullptr;
		u32 AttachmentDescriptionsCount = 0;

		const BMRSubpassSettings* SubpassesSettings = nullptr;
		u32 SubpassSettingsCount = 0;

		VkSubpassDependency* SubpassDependencies = nullptr;
		// Has to be 1 more then SubpassSettingsCount
		u32 SubpassDependenciesCount = 0;
	};
}

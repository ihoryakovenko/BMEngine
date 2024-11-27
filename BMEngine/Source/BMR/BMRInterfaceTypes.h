#pragma once

#include <vulkan\vulkan.h>

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

#include "BMRVulkan/BMRVulkan.h"

namespace BMR
{
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

	struct BMRConfig
	{
		BMRLogHandler LogHandler = nullptr;
		bool EnableValidationLayers = false;
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
		VkDescriptorSet TextureSet = nullptr;
		BMRModel Model;
	};

	struct BMRDrawTerrainEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		VkDescriptorSet TextureSet = nullptr;
	};

	struct BMRDrawSkyBoxEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		VkDescriptorSet TextureSet = nullptr;
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
}

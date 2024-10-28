#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

namespace BMR
{
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
		BMRShaderStages Stage = BMRShaderStages::BMRShaderStagesNone;
		u32* Code = nullptr;
		u32 CodeSize = 0;
	};

	struct BMRConfig
	{
		BMRShaderCodeDescription RenderShaders[BMRShaderNames::ShaderNamesCount];

		u32 MaxTextures = 0;
	};

	typedef glm::mat4 BMRModel;

	struct BMRUboViewProjection
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

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
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct BMRSpotLight
	{
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
	};

	struct BMRDrawScene
	{
		BMRUboViewProjection ViewProjection;

		BMRDrawEntity* DrawEntities;
		u32 DrawEntitiesCount;

		BMRDrawEntity* DrawTransparentEntities;
		u32 DrawTransparentEntitiesCount;

		BMRDrawTerrainEntity* DrawTerrainEntities;
		u32 DrawTerrainEntitiesCount;

		BMRDrawSkyBoxEntity SkyBox;
		bool DrawSkyBox = false;
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

	struct BMREntityVertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct BMRTerrainVertex
	{
		f32 Altitude;
	};

	struct BMRSkyBoxVertex
	{
		glm::vec3 Position;
	};
}

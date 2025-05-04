#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanInterface/VulkanInterface.h"

#include "Util/EngineTypes.h"
#include "Engine/Systems/Render/RenderResources.h"

namespace Render
{
	struct TextureArrayInfo
	{
		u32 Width = 0;
		u32 Height = 0;
		u32 Format = 0;
		u32 LayersCount = 0;
		u8** Data = nullptr;
		VkImageViewType ViewType;
		VkImageCreateFlags Flags;
		u32 BaseArrayLayer = 0;
	};

	struct PointLight
	{
		glm::vec4 Position;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
	};

	struct DirectionLight
	{
		glm::mat4 LightSpaceMatrix;
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct SpotLight
	{
		glm::mat4 LightSpaceMatrix;
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

	struct LightBuffer
	{
		PointLight PointLight;
		DirectionLight DirectionLight;
		SpotLight SpotLight;

		glm::vec4 tmp;
		glm::vec4 tmp2;
	};

	struct DrawScene
	{
		RenderResources::DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		RenderResources::DrawEntity SkyBox;
		bool DrawSkyBox = false;

		RenderResources::DrawEntity MapEntity;

		LightBuffer* LightEntity = nullptr;
	};

	struct RenderTexture
	{
		VulkanInterface::UniformImage UniformData;
		VkImageView ImageView = nullptr;
	};

	typedef void (*OnDrawDelegate)(void);

	bool Init();
	void DeInit();

	void LoadIndices(VulkanInterface::IndexBuffer* IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset);
	void LoadVertices(VulkanInterface::VertexBuffer* VertexBuffer, const void* Vertices, u32 VertexSize, u64 VerticesCount, u64 Offset);

	void Draw(const DrawScene* Scene);
}
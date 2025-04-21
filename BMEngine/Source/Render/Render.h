#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanInterface/VulkanInterface.h"

#include "Util/EngineTypes.h"

namespace Render
{
	typedef glm::mat4 BMRModel;

	typedef glm::mat4 BMRLightSpaceMatrix;

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

	struct DrawEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		VkDescriptorSet TextureSet = nullptr;
		BMRModel Model;
	};

	struct DrawMapEntity
	{
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		VkDescriptorSet TextureSet = nullptr;
	};

	struct DrawSkyBoxEntity
	{
		u64 VertexOffset = 0;
		u64 IndexOffset = 0;
		u32 IndicesCount = 0;
		VkDescriptorSet TextureSet = nullptr;
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
		BMRLightSpaceMatrix LightSpaceMatrix;
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct SpotLight
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
		DrawEntity* DrawEntities = nullptr;
		u32 DrawEntitiesCount = 0;

		DrawEntity* DrawTransparentEntities = nullptr;
		u32 DrawTransparentEntitiesCount = 0;

		DrawSkyBoxEntity SkyBox;
		bool DrawSkyBox = false;

		DrawMapEntity MapEntity;

		LightBuffer* LightEntity = nullptr;
	};

	struct RenderTexture
	{
		VulkanInterface::UniformImage UniformData;
		VkImageView ImageView = nullptr;
	};

	typedef void (*OnDrawDelegate)(void);

	void PreRenderInit();
	bool Init();
	void DeInit();

	// Move to engine
	void AddDrawFunction(OnDrawDelegate Delegate);

	VkDescriptorSetLayout TestGetTerrainSkyBoxLayout();
	VkRenderPass TestGetRenderPass();
	void TestUpdateUniforBuffer(VulkanInterface::UniformBuffer* Buffer, u64 DataSize, u64 Offset, const void* Data);

	RenderTexture CreateTexture(TextureArrayInfo* Info);
	RenderTexture CreateEmptyTexture(TextureArrayInfo* Info);
	void UpdateTexture(RenderTexture* Texture, TextureArrayInfo* Info);
	void DestroyTexture(RenderTexture* Texture);

	void LoadIndices(VulkanInterface::IndexBuffer* IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset);
	void LoadVertices(VulkanInterface::VertexBuffer* VertexBuffer, const void* Vertices, u32 VertexSize, u64 VerticesCount, u64 Offset);

	void Draw(const DrawScene* Scene);

	VkImage TmpGetShadowMapArray(int i);
}
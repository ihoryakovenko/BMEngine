#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

#include "Render/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/StaticMeshRender.h"

namespace RenderResources
{
	struct DrawEntity
	{
		u64 StaticMeshIndex;
		u32 MaterialIndex;
		glm::mat4 Model;
	};

	struct Material
	{
		u32 AlbedoTexIndex;
		u32 SpecularTexIndex;
		float Shininess;
	};

	void Init(u64 VertexBufferSize, u64 IndexBufferSize, u32 MaxEntities, u32 MaxTextures);
	void DeInit();

	DrawEntity CreateTerrain(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 Material);
	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height);
	u32 CreateMaterial(Material* Mat);

	DrawEntity* GetEntities(u32* Count);	
	u32 GetTexture2DSRGBIndex(u64 Hash);
	VkDescriptorSetLayout GetBindlesTexturesLayout();
	VkDescriptorSet GetBindlesTexturesSet();




	VkImageView* TmpGetTextureImageViews();
	VkDescriptorSetLayout TmpGetMaterialLayout();
	VkDescriptorSet TmpGetMaterialSet();
}
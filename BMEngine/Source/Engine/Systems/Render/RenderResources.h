#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

#include "Render/VulkanInterface/VulkanInterface.h"

namespace RenderResources
{
	struct ResourceHandle
	{
		u32 Index;
		u32 Generation;
	};

	struct DrawEntity
	{
		u64 VertexOffset;
		u64 IndexOffset;
		u32 IndicesCount;
		u32 TextureIndex;
		glm::mat4 Model;
	};

	void Init(u64 VertexBufferSize, u64 IndexBufferSize, u32 MaxEntities, u32 MaxTextures);
	void DeInit();

	VulkanInterface::VertexBuffer GetVertexBuffer();
	VulkanInterface::IndexBuffer GetIndexBuffer();

	u64 CreateEntity(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 TextureIndex);
	DrawEntity* GetEntities(u32* Count);

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height);
	u32 GetTexture2DSRGBIndex(u64 Hash);


	VkImageView* GetTextureImageViews(u32* Count);
}
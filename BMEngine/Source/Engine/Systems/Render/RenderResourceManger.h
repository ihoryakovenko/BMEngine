#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

#include "Render/VulkanInterface/VulkanInterface.h"

namespace RenderResourceManager
{
	struct DrawEntity
	{
		u64 VertexOffset;
		u64 IndexOffset;
		u32 IndicesCount;
		VkDescriptorSet TextureSet;
		glm::mat4 Model;
	};

	void DeInit();

	void AllocateVertexBuffer(u64 Size);
	void AllocateIndexBuffer(u64 Size);

	VulkanInterface::VertexBuffer GetVertexBuffer();
	VulkanInterface::IndexBuffer GetIndexBuffer();

	// TODO: do something with TextureSet
	u64 CreateEntity(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, VkDescriptorSet TextureSet);

	DrawEntity* GetEntities();
	u64 GetEntitiesCount();
}
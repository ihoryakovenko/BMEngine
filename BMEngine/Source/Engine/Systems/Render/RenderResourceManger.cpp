#include "RenderResourceManger.h"

#include <vulkan/vulkan.h>

#include "Render/Render.h"

namespace RenderResourceManager
{
	static const u64 MaxEntities = 1024;
	static DrawEntity DrawEntities[MaxEntities];
	static u64 Index;

	static VulkanInterface::IndexBuffer IndexBuffer;
	static VulkanInterface::VertexBuffer VertexBuffer;

	void DeInit()
	{
		VulkanInterface::DestroyUniformBuffer(VertexBuffer);
		VulkanInterface::DestroyUniformBuffer(IndexBuffer);
	}

	void AllocateVertexBuffer(u64 Size)
	{
		IndexBuffer = VulkanInterface::CreateIndexBuffer(Size);
	}

	void AllocateIndexBuffer(u64 Size)
	{
		VertexBuffer = VulkanInterface::CreateVertexBuffer(Size);
	}

	VulkanInterface::VertexBuffer GetVertexBuffer()
	{
		return VertexBuffer;
	}

	VulkanInterface::IndexBuffer GetIndexBuffer()
	{
		return IndexBuffer;
	}

	u64 CreateEntity(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 TextureIndex)
	{
		assert(Index < MaxEntities);

		DrawEntity* Entity = DrawEntities + Index;

		Entity->VertexOffset = VertexBuffer.Offset;
		Render::LoadVertices(&VertexBuffer, Vertices, VertexSize, VerticesCount, VertexBuffer.Offset);

		Entity->IndexOffset = IndexBuffer.Offset;
		Render::LoadIndices(&IndexBuffer, Indices, IndicesCount, IndexBuffer.Offset);

		Entity->IndicesCount = IndicesCount;
		Entity->TextureIndex = TextureIndex;
		Entity->Model = glm::mat4(1.0f);

		const u64 EntityIndex = Index;
		++Index;

		return EntityIndex;
	}

	DrawEntity* GetEntities()
	{
		return DrawEntities;
	}

	u64 GetEntitiesCount()
	{
		return Index;
	}
}

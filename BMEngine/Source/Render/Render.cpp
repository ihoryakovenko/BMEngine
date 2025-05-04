#include "Render.h"

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "FrameManager.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"
#include "Engine/Systems/Render/DeferredPass.h"
#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/VulkanHalper.h"
#include "Engine/Systems/Render/DebugUi.h"

namespace Render
{
	bool Init()
	{
		return true;
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();
	}

	void LoadIndices(VulkanInterface::IndexBuffer* IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset)
	{
		assert(Indices);
		assert(IndexBuffer);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		//const VkDeviceSize AlignedSize = VulkanHelper::CalculateBufferAlignedSize(VulkanInterface::GetDevice(), IndexBuffer->Buffer, MeshIndicesSize);
		const VkDeviceSize AlignedSize = MeshIndicesSize;

		VulkanInterface::CopyDataToBuffer(IndexBuffer->Buffer, Offset, MeshIndicesSize, Indices);
	}

	void LoadVertices(VulkanInterface::VertexBuffer* VertexBuffer, const void* Vertices, u32 VertexSize, u64 VerticesCount, u64 Offset)
	{
		assert(VertexBuffer);
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		//const VkDeviceSize AlignedSize = VulkanHelper::CalculateBufferAlignedSize(VulkanInterface::GetDevice(), VertexBuffer->Buffer, MeshVerticesSize);
		const VkDeviceSize AlignedSize = MeshVerticesSize;

		VulkanInterface::CopyDataToBuffer(VertexBuffer->Buffer, Offset, MeshVerticesSize, Vertices);
	}

	void Draw(const DrawScene* Scene)
	{
		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		VulkanInterface::BeginDraw(ImageIndex);

		LightningPass::Draw();

		MainPass::BeginPass();

		TerrainRender::Draw();
		StaticMeshRender::Draw();
		DebugUi::Draw();

		MainPass::EndPass();
		DeferredPass::Draw();

		
		
		VulkanInterface::EndDraw(ImageIndex);
	}
}

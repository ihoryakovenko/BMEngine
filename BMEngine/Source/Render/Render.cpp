#include "Render.h"

#include <vulkan/vulkan.h>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "FrameManager.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"
#include "Engine/Systems/Render/DeferredPass.h"
#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Engine/Systems/Render/DebugUi.h"
#include "Engine/Systems/ResourceManager.h"

namespace Render
{
	bool Init(GLFWwindow* Window, DebugUi::GuiData* DataPtr)
	{
		VulkanInterface::VulkanInterfaceConfig RenderConfig;
		//RenderConfig.MaxTextures = 90;
		RenderConfig.MaxTextures = 500; // TODO: FIX!!!!

		VulkanInterface::Init(Window, RenderConfig);
		RenderResources::Init(MB32, MB32, 512, 256);
		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		TerrainRender::Init();
		//DynamicMapSystem::Init();
		StaticMeshRender::Init();
		DebugUi::Init(Window, DataPtr);

		return true;
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();
		//DynamicMapSystem::DeInit();
		DebugUi::DeInit();
		TerrainRender::DeInit();
		StaticMeshRender::DeInit();

		RenderResources::DeInit();
		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();
	}

	void Draw(FrameManager::ViewProjectionBuffer ViewProjection)
	{
		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		FrameManager::UpdateViewProjection(&ViewProjection);

		VulkanInterface::BeginFrame();

		LightningPass::Draw();

		MainPass::BeginPass();

		TerrainRender::Draw();
		StaticMeshRender::Draw();

		MainPass::EndPass();
		DeferredPass::BeginPass();

		DeferredPass::Draw();
		DebugUi::Draw();

		DeferredPass::EndPass();

		VulkanInterface::EndFrame();
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
}

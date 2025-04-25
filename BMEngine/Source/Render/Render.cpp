#include "Render.h"

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "FrameManager.h"
#include "Engine/Systems/TerrainSystem.h"
#include "Engine/Systems/StaticMeshSystem.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"

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

	void TestUpdateUniforBuffer(VulkanInterface::UniformBuffer* Buffer, u64 DataSize, u64 Offset, const void* Data)
	{
		VulkanInterface::UpdateUniformBuffer(Buffer[VulkanInterface::TestGetImageIndex()], DataSize, Offset,
			Data);
	}

	RenderTexture CreateTexture(TextureArrayInfo* Info)
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Info->Width;
		ImageCreateInfo.extent.height = Info->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Info->LayersCount;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = Info->Flags;

		VulkanInterface::UniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
		InterfaceCreateInfo.Flags = 0;
		InterfaceCreateInfo.ViewType = Info->ViewType;
		InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
		InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		InterfaceCreateInfo.SubresourceRange.levelCount = 1;
		InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		InterfaceCreateInfo.SubresourceRange.layerCount = Info->LayersCount;

		RenderTexture Texture;
		Texture.UniformData = VulkanInterface::CreateUniformImage(&ImageCreateInfo);
		Texture.ImageView = VulkanInterface::CreateImageInterface(&InterfaceCreateInfo, Texture.UniformData.Image);

		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = 0;
		TransitionData.LayerCount = Info->LayersCount;

		// FIRST TRANSITION
		VulkanInterface::TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			&TransitionData, 1);

		VulkanInterface::CopyDataToImage(Texture.UniformData.Image, Info->Width, Info->Height, Info->Format, Info->LayersCount, Info->Data);

		// SECOND TRANSITION
		TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, &TransitionData, 1);

		return Texture;
	}

	RenderTexture CreateEmptyTexture(TextureArrayInfo* Info)
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Info->Width;
		ImageCreateInfo.extent.height = Info->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Info->LayersCount;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = Info->Flags;

		VulkanInterface::UniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
		InterfaceCreateInfo.Flags = 0;
		InterfaceCreateInfo.ViewType = Info->ViewType;
		InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
		InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		InterfaceCreateInfo.SubresourceRange.levelCount = 1;
		InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		InterfaceCreateInfo.SubresourceRange.layerCount = Info->LayersCount;

		RenderTexture Texture;
		Texture.UniformData = VulkanInterface::CreateUniformImage(&ImageCreateInfo);
		Texture.ImageView = VulkanInterface::CreateImageInterface(&InterfaceCreateInfo, Texture.UniformData.Image);

		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = 0;
		TransitionData.LayerCount = Info->LayersCount;
		
		// If texture is empty perform transit to shader optimal layout to allow draw
		VulkanInterface::TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			&TransitionData, 1);

		return Texture;
	}

	void UpdateTexture(RenderTexture* Texture, TextureArrayInfo* Info)
	{
		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = Info->BaseArrayLayer;
		TransitionData.LayerCount = Info->LayersCount;

		VulkanInterface::TransitImageLayout(Texture->UniformData.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			&TransitionData, 1);

		VulkanInterface::CopyDataToImage(Texture->UniformData.Image, Info->Width, Info->Height,
			Info->Format, Info->LayersCount, Info->Data, Info->BaseArrayLayer);
	
		TransitImageLayout(Texture->UniformData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, &TransitionData, 1);
	}

	void DestroyTexture(RenderTexture* Texture)
	{
		VulkanInterface::WaitDevice();
		VulkanInterface::DestroyImageInterface(Texture->ImageView);
		VulkanInterface::DestroyUniformImage(Texture->UniformData);
	}

	void LoadIndices(VulkanInterface::IndexBuffer* IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset)
	{
		assert(Indices);
		assert(IndexBuffer);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanInterface::CopyDataToBuffer(IndexBuffer->Buffer, Offset, MeshIndicesSize, Indices);
		IndexBuffer->Offset += AlignedSize;
	}

	void LoadVertices(VulkanInterface::VertexBuffer* VertexBuffer, const void* Vertices, u32 VertexSize, u64 VerticesCount, u64 Offset)
	{
		assert(VertexBuffer);
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanInterface::CopyDataToBuffer(VertexBuffer->Buffer, Offset, MeshVerticesSize, Vertices);
		VertexBuffer->Offset += AlignedSize;
	}

	void Draw(const DrawScene* Scene)
	{
		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		VulkanInterface::BeginDraw(ImageIndex);

		LightningPass::Draw();

		MainPass::BeginPass();

		TerrainSystem::Draw();
		StaticMeshSystem::Draw();

		MainPass::EndPass();
		
		VulkanInterface::EndDraw(ImageIndex);
	}

	VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % BUFFER_ALIGNMENT != 0)
		{
			Padding = BUFFER_ALIGNMENT - (BufferSize % BUFFER_ALIGNMENT);
		}

		return BufferSize + Padding;
	}

	VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % IMAGE_ALIGNMENT != 0)
		{
			Padding = IMAGE_ALIGNMENT - (BufferSize % IMAGE_ALIGNMENT);
		}

		return BufferSize + Padding;
	}
}

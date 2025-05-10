#include "RenderResources.h"

#include <vulkan/vulkan.h>

#include "Render/Render.h"

#include "Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHalper.h"
#include "Render/Render.h"

namespace RenderResources
{
	struct HandleEntry
	{
		u32 Generation;
		u32 PhysicalIndex;
		bool Alive;
	};

	static u32 MaxEntities = 1024;
	static DrawEntity* DrawEntities;
	static u32 EntityIndex;

	static VulkanInterface::IndexBuffer IndexBuffer;
	static u64 IndexOffset;
	static VulkanInterface::VertexBuffer VertexBuffer;
	static u64 VertexOffset;

	static u32 MaxTextures = 256;
	static VkImage* Textures;
	static VkImageView* TextureViews;
	static VkDeviceMemory* TexturesMemory;
	static std::unordered_map<u64, u32> TexturesPhysicalIndexes;
	static u32 TextureIndex;

	static VkSampler DiffuseSampler;
	static VkSampler SpecularSampler;

	static VkDescriptorSetLayout BindlesTexturesLayout;
	static VkDescriptorSet BindlesTexturesSet;

	static VkBuffer MaterialBuffer;
	static VkDeviceMemory MaterialBufferMemory;
	static VkDescriptorSetLayout MaterialLayout;
	static VkDescriptorSet MaterialSet;
	static u32 MaterialIndex;

	//static std::vector<HandleEntry> handleEntries;
	//static std::array<u32, MaxResources> reverseMapping;
	//static std::vector<u32> freeList;

	void Init(u64 VertexBufferSize, u64 IndexBufferSize, u32 MaximumEntities, u32 MaximumTextures)
	{
		MaxEntities = MaximumEntities;
		MaxTextures = MaximumTextures;

		IndexBuffer = VulkanInterface::CreateIndexBuffer(IndexBufferSize);
		VertexBuffer = VulkanInterface::CreateVertexBuffer(IndexBufferSize);

		DrawEntities = Memory::BmMemoryManagementSystem::Allocate<DrawEntity>(MaxEntities);
		Textures = Memory::BmMemoryManagementSystem::Allocate<VkImage>(MaxTextures);
		TextureViews = Memory::BmMemoryManagementSystem::Allocate<VkImageView>(MaxTextures);
		TexturesMemory = Memory::BmMemoryManagementSystem::Allocate<VkDeviceMemory>(MaxTextures);

		VkDevice Device = VulkanInterface::GetDevice();

		VkSamplerCreateInfo DiffuseSamplerCreateInfo = { };
		DiffuseSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		DiffuseSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		DiffuseSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		DiffuseSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		DiffuseSamplerCreateInfo.mipLodBias = 0.0f;
		DiffuseSamplerCreateInfo.minLod = 0.0f;
		DiffuseSamplerCreateInfo.maxLod = 0.0f;
		DiffuseSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		DiffuseSamplerCreateInfo.maxAnisotropy = 16;

		VkSamplerCreateInfo SpecularSamplerCreateInfo = DiffuseSamplerCreateInfo;
		DiffuseSamplerCreateInfo.maxAnisotropy = 1;

		DiffuseSampler = VulkanInterface::CreateSampler(&DiffuseSamplerCreateInfo);
		SpecularSampler = VulkanInterface::CreateSampler(&SpecularSamplerCreateInfo);

		const VkShaderStageFlags EntitySamplerInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		const VkDescriptorType EntitySamplerDescriptorType[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		const VkDescriptorBindingFlags BindingFlags[2] = {
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };

		// TODO: Get max textures from limits.maxPerStageDescriptorSampledImages;
		BindlesTexturesLayout = VulkanInterface::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, BindingFlags, 2, MaxTextures);
		VulkanInterface::CreateUniformSets(&BindlesTexturesLayout, 1, &BindlesTexturesSet);

		const VkDeviceSize MaterialBufferSize = sizeof(Material) * MaxEntities;
		MaterialBuffer = VulkanHelper::CreateBuffer(Device, MaterialBufferSize, VulkanHelper::BufferUsageFlag::Storage);
		MaterialBufferMemory = VulkanHelper::AllocateAndBindDeviceMemoryForBuffer(VulkanInterface::GetPhysicalDevice(), Device,
			MaterialBuffer, MaterialBufferSize, VulkanHelper::MemoryPropertyFlag::GPULocal);

		VulkanInterface::UniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MaterialBuffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialBufferSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		const VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		const VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags MaterialBindingFlags[1] = { };
		MaterialLayout = VulkanInterface::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, MaterialBindingFlags, 1, 1);
		VulkanInterface::CreateUniformSets(&MaterialLayout, 1, &MaterialSet);
		VulkanInterface::AttachUniformsToSet(MaterialSet, &MaterialAttachmentInfo, 1);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (uint32_t i = 0; i < TextureIndex; ++i)
		{
			vkDestroyImageView(Device, TextureViews[i], nullptr);
			vkDestroyImage(Device, Textures[i], nullptr);
			vkFreeMemory(Device, TexturesMemory[i], nullptr);
		}

		VulkanInterface::DestroyUniformBuffer(VertexBuffer);
		VulkanInterface::DestroyUniformBuffer(IndexBuffer);

		VulkanInterface::DestroyUniformLayout(BindlesTexturesLayout);
		VulkanInterface::DestroyUniformLayout(MaterialLayout);

		VulkanInterface::DestroySampler(DiffuseSampler);
		VulkanInterface::DestroySampler(SpecularSampler);

		vkDestroyBuffer(Device, MaterialBuffer, nullptr);
		vkFreeMemory(Device, MaterialBufferMemory, nullptr);

		Memory::BmMemoryManagementSystem::Free(DrawEntities);
		Memory::BmMemoryManagementSystem::Free(Textures);
		Memory::BmMemoryManagementSystem::Free(TextureViews);
		Memory::BmMemoryManagementSystem::Free(TexturesMemory);
	}

	VulkanInterface::VertexBuffer GetVertexBuffer()
	{
		return VertexBuffer;
	}

	VulkanInterface::IndexBuffer GetIndexBuffer()
	{
		return IndexBuffer;
	}

	u32 CreateEntity(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 MaterialIndex)
	{
		assert(EntityIndex < MaxEntities);

		DrawEntity* Entity = DrawEntities + EntityIndex;
		*Entity = { };

		Entity->VertexOffset = VertexOffset;
		Render::LoadVertices(&VertexBuffer, Vertices, VertexSize, VerticesCount, VertexOffset);
		VertexOffset += VertexSize * VerticesCount;

		Entity->IndexOffset = IndexOffset;
		Render::LoadIndices(&IndexBuffer, Indices, IndicesCount, IndexOffset);
		IndexOffset += sizeof(u32) * IndicesCount;

		Entity->IndicesCount = IndicesCount;
		Entity->MaterialIndex = MaterialIndex;
		Entity->Model = glm::mat4(1.0f);

		const u32 CurrentEntityIndex = EntityIndex;
		++EntityIndex;

		return CurrentEntityIndex;
	}

	DrawEntity* GetEntities(u32* Count)
	{
		*Count = EntityIndex;
		return DrawEntities;
	}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height)
	{
		assert(TextureIndex < MaxTextures);

		VkDevice Device = VulkanInterface::GetDevice();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetTransferCommandBuffer();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();
		
		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Width;
		ImageCreateInfo.extent.height = Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = 0;

		VkResult Result = vkCreateImage(Device, &ImageCreateInfo, nullptr, Textures + TextureIndex);
		if (Result != VK_SUCCESS)
		{
			// TODO: Create log layer
			assert(false);
			//HandleLog(BMRVkLogType_Error, "CreateImage result is %d", Result);
		}

		VkImage Image = Textures[TextureIndex];

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;
		ViewCreateInfo.image = Image;

		VkImageMemoryBarrier2 TransferImageBarrier = { };
		TransferImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		TransferImageBarrier.pNext = nullptr;
		TransferImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		TransferImageBarrier.srcAccessMask = 0;
		TransferImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		TransferImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		TransferImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		TransferImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		TransferImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		TransferImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		TransferImageBarrier.image = Image;
		TransferImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		TransferImageBarrier.subresourceRange.baseMipLevel = 0;
		TransferImageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		TransferImageBarrier.subresourceRange.baseArrayLayer = 0;
		TransferImageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkDependencyInfo TransferDepInfo = { };
		TransferDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		TransferDepInfo.pNext = nullptr;
		TransferDepInfo.dependencyFlags = 0;
		TransferDepInfo.imageMemoryBarrierCount = 1;
		TransferDepInfo.pImageMemoryBarriers = &TransferImageBarrier;
		TransferDepInfo.memoryBarrierCount = 0;
		TransferDepInfo.pMemoryBarriers = nullptr;
		TransferDepInfo.bufferMemoryBarrierCount = 0;
		TransferDepInfo.pBufferMemoryBarriers = nullptr;

		VkImageMemoryBarrier2 PresentationBarrier = { };
		PresentationBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		PresentationBarrier.pNext = nullptr;
		PresentationBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		PresentationBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		PresentationBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		PresentationBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		PresentationBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		PresentationBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		PresentationBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		PresentationBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		PresentationBarrier.image = Image;

		PresentationBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		PresentationBarrier.subresourceRange.baseMipLevel = 0;
		PresentationBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		PresentationBarrier.subresourceRange.baseArrayLayer = 0;
		PresentationBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkDependencyInfo PresentationDepInfo = { };
		PresentationDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		PresentationDepInfo.pNext = nullptr;
		PresentationDepInfo.dependencyFlags = 0;
		PresentationDepInfo.imageMemoryBarrierCount = 1;
		PresentationDepInfo.pImageMemoryBarriers = &PresentationBarrier;
		PresentationDepInfo.memoryBarrierCount = 0;
		PresentationDepInfo.pMemoryBarriers = nullptr;
		PresentationDepInfo.bufferMemoryBarrierCount = 0;
		PresentationDepInfo.pBufferMemoryBarriers = nullptr;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CmdBuffer;

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(VulkanInterface::GetPhysicalDevice(), MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TexturesMemory[TextureIndex] = VulkanInterface::AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(Device, Image, TexturesMemory[TextureIndex], 0);

		Result = vkCreateImageView(Device, &ViewCreateInfo, nullptr, TextureViews + TextureIndex);
		if (Result != VK_SUCCESS)
		{
			// TODO: Create log layer
			assert(false);
			//HandleLog(BMRVkLogType_Error, "vkCreateImageView result is %d", Result);
		}

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(CmdBuffer, &BeginInfo);  // TODO: Fix
		vkCmdPipelineBarrier2(CmdBuffer, &TransferDepInfo);
		vkEndCommandBuffer(CmdBuffer);  // TODO: Fix

		vkQueueSubmit(TransferQueue, 1, &SubmitInfo, nullptr);
		VulkanInterface::WaitDevice(); // TODO: Fix

		VulkanInterface::CopyDataToImage(Image, Width, Height, 4, 1, Data); // TODO: 4 is u32 Format, fix

		vkBeginCommandBuffer(CmdBuffer, &BeginInfo);  // TODO: Fix
		vkCmdPipelineBarrier2(CmdBuffer, &PresentationDepInfo);
		vkEndCommandBuffer(CmdBuffer);  // TODO: Fix

		vkQueueSubmit(TransferQueue, 1, &SubmitInfo, nullptr);
		VulkanInterface::WaitDevice(); // TODO: Fix


		VkDescriptorImageInfo DiffuseImageInfo = { };
		DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DiffuseImageInfo.sampler = DiffuseSampler;
		DiffuseImageInfo.imageView = TextureViews[TextureIndex];

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = SpecularSampler;
		SpecularImageInfo.imageView = TextureViews[TextureIndex];


		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = BindlesTexturesSet;
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = TextureIndex;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = BindlesTexturesSet;
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = TextureIndex;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };

		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 2, Writes, 0, nullptr);

		const u32 CurrentTextureIndex = TextureIndex++;
		TexturesPhysicalIndexes[Hash] = CurrentTextureIndex;

		return CurrentTextureIndex;
	}

	u32 CreateMaterial(Material* Mat)
	{
		assert(MaterialIndex < MaxEntities);

		VulkanInterface::CopyDataToBuffer(MaterialBuffer, sizeof(Material) * MaterialIndex, sizeof(Material), Mat);
		return MaterialIndex++;
	}

	u32 GetTexture2DSRGBIndex(u64 Hash)
	{
		auto it = TexturesPhysicalIndexes.find(Hash);
		if (it != TexturesPhysicalIndexes.end())
		{
			return it->second;
		}

		return 0;
	}

	VkDescriptorSetLayout GetBindlesTexturesLayout()
	{
		return BindlesTexturesLayout;
	}

	VkDescriptorSet GetBindlesTexturesSet()
	{
		return BindlesTexturesSet;
	}





	VkImageView* TmpGetTextureImageViews()
	{
		return TextureViews;;
	}

	VkDescriptorSetLayout TmpGetMaterialLayout()
	{
		return MaterialLayout;
	}

	VkDescriptorSet TmpGetMaterialSet()
	{
		return MaterialSet;
	}

















	//ResourceHandle CreateTexture(VkImage image, VkImageView view)
	//{
	//	u32 LogicalIndex;

	//	if (!freeList.empty())
	//	{
	//		LogicalIndex = freeList.back();
	//		freeList.pop_back();
	//	}
	//	else
	//	{
	//		LogicalIndex = static_cast<u32>(handleEntries.size());
	//		handleEntries.emplace_back();
	//	}

	//	const u32 PhysicalIndex = TextureIndex++;

	//	Textures[PhysicalIndex] = image;
	//	TextureViews[PhysicalIndex] = view;

	//	handleEntries[LogicalIndex].Generation++;
	//	handleEntries[LogicalIndex].Alive = true;
	//	handleEntries[LogicalIndex].PhysicalIndex = PhysicalIndex;

	//	reverseMapping[PhysicalIndex] = LogicalIndex;

	//	return ResourceHandle{ LogicalIndex, handleEntries[LogicalIndex].Generation };
	//}

	//void DestroyTexture(ResourceHandle h)
	//{
	//	if (!IsValid(h)) return;

	//	u32 removedPhys = handleEntries[h.Index].PhysicalIndex;
	//	u32 lastPhys = TextureIndex - 1;

	//	if (removedPhys != lastPhys)
	//	{
	//		Textures[removedPhys] = Textures[lastPhys];
	//		TextureViews[removedPhys] = TextureViews[lastPhys];

	//		u32 movedLogical = reverseMapping[lastPhys];
	//		handleEntries[movedLogical].PhysicalIndex = removedPhys;
	//		reverseMapping[removedPhys] = movedLogical;
	//	}

	//	handleEntries[h.Index].Alive = false;
	//	freeList.push_back(h.Index);
	//	--TextureIndex;
	//}

	//bool IsValid(ResourceHandle h)
	//{
	//	return h.Index < handleEntries.size() &&
	//		handleEntries[h.Index].Alive &&
	//		handleEntries[h.Index].Generation == h.Generation;
	//}

	//VkImage GetImage(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return Textures[PhysicalIndex];
	//}

	//VkImageView GetView(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return TextureViews[PhysicalIndex];
	//}
}

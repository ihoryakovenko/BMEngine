#include "RenderResources.h"

#include <vulkan/vulkan.h>

#include <unordered_map>

#include "Engine/Systems/Render/Render.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/StaticMeshRender.h"

#include <atomic>

namespace RenderResources
{
	struct HandleEntry
	{
		u32 Generation;
		u32 PhysicalIndex;
		bool Alive;
	};

	static u32 MaxEntities = 1024;

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

	//static std::vector<HandleEntry> handleEntries;
	//static std::array<u32, MaxResources> reverseMapping;
	//static std::vector<u32> freeList;

	void Init(u64 VertexBufferSize, u64 IndexBufferSize, u32 MaximumEntities, u32 MaximumTextures)
	{
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		MaxEntities = MaximumEntities;
		MaxTextures = MaximumTextures;

		Textures = Memory::MemoryManagementSystem::Allocate<VkImage>(MaxTextures);
		TextureViews = Memory::MemoryManagementSystem::Allocate<VkImageView>(MaxTextures);
		TexturesMemory = Memory::MemoryManagementSystem::Allocate<VkDeviceMemory>(MaxTextures);

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

		vkDestroyDescriptorSetLayout(Device, BindlesTexturesLayout, nullptr);
		

		vkDestroySampler(Device, DiffuseSampler, nullptr);
		vkDestroySampler(Device, SpecularSampler, nullptr);



		Memory::MemoryManagementSystem::Free(Textures);
		Memory::MemoryManagementSystem::Free(TextureViews);
		Memory::MemoryManagementSystem::Free(TexturesMemory);
	}

	//DrawEntity CreateTerrain(void* Vertices, u32 VertexSize, u64 VerticesCount, u32* Indices, u32 IndicesCount, u32 Material)
	//{
		//DrawEntity Entity = { };

		//Entity.VertexOffset = VertexOffset;
		//const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		//TransferSystem::QueueBufferDataLoad(VertexBuffer.Buffer, VertexOffset, MeshVerticesSize, Vertices);
		//VertexOffset += VertexSize * VerticesCount;

		//Entity.IndexOffset = IndexOffset;
		//VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		//TransferSystem::QueueBufferDataLoad(IndexBuffer.Buffer, IndexOffset, MeshIndicesSize, Indices);
		//IndexOffset += sizeof(u32) * IndicesCount;

		//Entity.IndicesCount = IndicesCount;
		//Entity.MaterialIndex = Material;
		//Entity.Model = glm::mat4(1.0f);

		//return Entity;
	//}

	//DrawEntity* GetEntities(u32* Count)
	//{
	//	Render::RenderState* State = Render::GetRenderState();
	//	*Count = State->DrawEntities.Count;
	//	return State->DrawEntities.Data;
	//}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height)
	{
		assert(TextureIndex < MaxTextures);

		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();
		
		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Width;
		ImageCreateInfo.extent.height = Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
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
			//Util::RenderLog(Util::BMRVkLogType_Error, "CreateImage result is %d", Result);
		}

		VkImage Image = Textures[TextureIndex];

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
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

		TexturesMemory[TextureIndex] = VulkanHelper::AllocateAndBindDeviceMemoryForImage(PhysicalDevice, Device, Image, VulkanHelper::GPULocal);

		Result = vkCreateImageView(Device, &ViewCreateInfo, nullptr, TextureViews + TextureIndex);
		if (Result != VK_SUCCESS)
		{
			// TODO: Create log layer
			assert(false);
			//Util::RenderLog(Util::BMRVkLogType_Error, "vkCreateImageView result is %d", Result);
		}

		Render::QueueImageDataLoad(Image, Width, Height, Data);

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
		return TextureViews;
	}


}

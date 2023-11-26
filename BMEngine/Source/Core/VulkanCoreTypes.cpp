#include "VulkanCoreTypes.h"

#include "Util/Util.h"
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace Core
{
	bool InitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data, const char** ValidationExtensions,
		uint32_t ValidationExtensionsCount, bool EnumerateInstanceLayerProperties)
	{
		// Get count part
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&Data.RequiredExtensionsCount);
		if (Data.RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		if (EnumerateInstanceLayerProperties)
		{
			Result = vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, nullptr);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
				return false;
			}
		}

		// Allocation part
		Data.RequiredAndValidationExtensions = static_cast<const char**>(Util::Memory::Allocate((ValidationExtensionsCount + Data.RequiredExtensionsCount) * sizeof(const char**)));
		Data.AvalibleExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleExtensionsCount * sizeof(VkExtensionProperties)));
		Data.AvalibleValidationLayers = static_cast<VkLayerProperties*>(Util::Memory::Allocate(Data.AvalibleValidationLayersCount * sizeof(VkLayerProperties)));

		// Data structuring part
		Data.RequiredAndValidationExtensionsCount = Data.RequiredExtensionsCount + ValidationExtensionsCount;
		Data.RequiredInstanceExtensions = Data.RequiredAndValidationExtensions + ValidationExtensionsCount;

		// Data assignment part
		for (uint32_t i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data.RequiredAndValidationExtensions[i] = ValidationExtensions[i];
		}

		for (uint32_t i = 0; i < Data.RequiredExtensionsCount; ++i)
		{
			Data.RequiredInstanceExtensions[i] = RequiredInstanceExtensions[i];
		}

		vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, Data.AvalibleExtensions);
		vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, Data.AvalibleValidationLayers);

		return true;
	}

	void DeinitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data)
	{
		Util::Memory::Deallocate(Data.RequiredAndValidationExtensions);
		Util::Memory::Deallocate(Data.AvalibleExtensions);
		Util::Memory::Deallocate(Data.AvalibleValidationLayers);
	}

	bool CheckRequiredInstanceExtensionsSupport(const VkInstanceCreateInfoSetupData& Data)
	{
		for (uint32_t i = 0; i < Data.RequiredAndValidationExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleExtensionsCount; ++j)
			{
				if (std::strcmp(Data.RequiredAndValidationExtensions[i], Data.AvalibleExtensions[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", Data.RequiredAndValidationExtensions[i]);
				return false;
			}
		}

		return true;
	}

	bool CheckValidationLayersSupport(const VkInstanceCreateInfoSetupData& Data, const char** ValidationLeyersToCheck,
		uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < Data.AvalibleValidationLayersCount; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Data.AvalibleValidationLayers[j].layerName) == 0)
				{
					IsLayerAvalible = true;
					break;
				}
			}

			if (!IsLayerAvalible)
			{
				Util::Log().Error("Validation layer {} unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}

	bool InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		// Get count part
		VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, nullptr);
		if (Data.AvalibleDeviceExtensionsCount == 0)
		{
			Util::Log().Error("vkEnumerateDeviceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, nullptr);

		Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, nullptr);
		if (Data.SurfaceFormatsCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfaceFormatsKHR result is {}", static_cast<int>(Result));
			return false;
		}

		Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, nullptr);
		if (Data.PresentModesCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			return false;
		}

		// Allocation part
		Data.AvalibleDeviceExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleDeviceExtensionsCount * sizeof(VkExtensionProperties)));
		Data.FamilyProperties = static_cast<VkQueueFamilyProperties*>(Util::Memory::Allocate(Data.FamilyPropertiesCount * sizeof(VkQueueFamilyProperties)));
		Data.SurfaceFormats = static_cast<VkSurfaceFormatKHR*>(Util::Memory::Allocate(Data.SurfaceFormatsCount * sizeof(VkSurfaceFormatKHR)));
		Data.PresentModes = static_cast<VkPresentModeKHR*>(Util::Memory::Allocate(Data.PresentModesCount * sizeof(VkPresentModeKHR)));

		// Data assignment part
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, Data.AvalibleDeviceExtensions);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, Data.FamilyProperties);
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, Data.SurfaceFormats);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, Data.PresentModes);

		return true;
	}

	void DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data)
	{
		Util::Memory::Deallocate(Data.AvalibleDeviceExtensions);
		Util::Memory::Deallocate(Data.FamilyProperties);
		Util::Memory::Deallocate(Data.SurfaceFormats);
		Util::Memory::Deallocate(Data.PresentModes);
	}

	bool CheckDeviceExtensionsSupport(const VkPhysicalDeviceSetupData& Data, const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize)
	{
		for (uint32_t i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleDeviceExtensionsCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], Data.AvalibleDeviceExtensions[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				Util::Log().Error("Device extension {} unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices GetPhysicalDeviceIndices(const VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (uint32_t i = 0; i < Data.FamilyPropertiesCount; ++i)
		{
			// check if Queue is graphics type
			if (Data.FamilyProperties[i].queueCount > 0 && Data.FamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilys
				Indices.GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentationSupport);
			if (Data.FamilyProperties[i].queueCount > 0 && PresentationSupport)
			{
				Indices.PresentationFamily = i;
			}

			if (Indices.GraphicsFamily >= 0 && Indices.PresentationFamily >= 0)
			{
				break;
			}
		}

		return Indices;
	}

	bool CreateGenericBuffer(const VulkanRenderInstance& RenderInstance, VkDeviceSize BufferSize,
		VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags BufferProperties, GenericBuffer& OutBuffer)
	{
		VkBufferCreateInfo BufferCreateInfo = {};
		BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.size = BufferSize;
		BufferCreateInfo.usage = BufferUsage;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult Result = vkCreateBuffer(RenderInstance.LogicalDevice, &BufferCreateInfo, nullptr, &OutBuffer.Buffer);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(RenderInstance.LogicalDevice, OutBuffer.Buffer, &MemoryRequirements);

		VkMemoryAllocateInfo MemoryAllocInfo = {};
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(RenderInstance.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			BufferProperties);	

		Result = vkAllocateMemory(RenderInstance.LogicalDevice, &MemoryAllocInfo, nullptr, &OutBuffer.BufferMemory);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		vkBindBufferMemory(RenderInstance.LogicalDevice, OutBuffer.Buffer, OutBuffer.BufferMemory, 0);

		return true;
	}

	void DestroyGenericBuffer(const VulkanRenderInstance& RenderInstance, GenericBuffer& Buffer)
	{
		vkDestroyBuffer(RenderInstance.LogicalDevice, Buffer.Buffer, nullptr);
		vkFreeMemory(RenderInstance.LogicalDevice, Buffer.BufferMemory, nullptr);
	}

	void CopyBuffer(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkBuffer DstinationBuffer, VkDeviceSize BufferSize)
	{
		// Todo: Use 1 CommandBuffer for all copy operations?
		VkCommandBuffer TransferCommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = RenderInstance.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferCopy BufferCopyRegion = {};
		BufferCopyRegion.srcOffset = 0;
		BufferCopyRegion.dstOffset = 0;
		BufferCopyRegion.size = BufferSize;

		// Todo copy multiple regions at once?
		vkCmdCopyBuffer(TransferCommandBuffer, SourceBuffer, DstinationBuffer, 1, &BufferCopyRegion);

		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(RenderInstance.GraphicsQueue);

		vkFreeCommandBuffers(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, 1, &TransferCommandBuffer);
	}

	void CopyBufferToImage(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkImage Image, uint32_t Width, uint32_t Height)
	{
		// Todo: Use 1 CommandBuffer for all copy operations?
		VkCommandBuffer TransferCommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = RenderInstance.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferImageCopy ImageRegion = {};
		ImageRegion.bufferOffset = 0;
		ImageRegion.bufferRowLength = 0;
		ImageRegion.bufferImageHeight = 0;
		ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRegion.imageSubresource.mipLevel = 0;
		ImageRegion.imageSubresource.baseArrayLayer = 0;						// Starting array layer (if array)
		ImageRegion.imageSubresource.layerCount = 1;
		ImageRegion.imageOffset = { 0, 0, 0 };
		ImageRegion.imageExtent = { Width, Height, 1 };

		// Todo copy multiple regions at once?
		vkCmdCopyBufferToImage(TransferCommandBuffer, SourceBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);

		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(RenderInstance.GraphicsQueue);

		vkFreeCommandBuffers(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, 1, &TransferCommandBuffer);
	}

	void CreateTexture(VulkanRenderInstance& RenderInstance, stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize)
	{
		assert(RenderInstance.TextureImagesCount < RenderInstance.MaxTextures);

		GenericBuffer StagingBuffer;
		CreateGenericBuffer(RenderInstance, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			StagingBuffer);

		void* Data;
		vkMapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory, 0, ImageSize, 0, &Data);
		memcpy(Data, TextureData, static_cast<size_t>(ImageSize));
		vkUnmapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory);

		ImageBuffer ImageBuffeObject;
		ImageBuffeObject.TextureImage = CreateImage(RenderInstance, Width, Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ImageBuffeObject.TextureImagesMemory);

		// Todo: Create beginCommandBuffer function?
		VkCommandBuffer CommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = RenderInstance.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &AllocInfo, &CommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

		VkImageMemoryBarrier ImageMemoryBarrier = {};
		ImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
		ImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
		ImageMemoryBarrier.image = ImageBuffeObject.TextureImage;											// Image being accessed and modified as part of barrier
		ImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
		ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
		ImageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
		ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer to start alterations on
		ImageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from baseArrayLayer

		ImageMemoryBarrier.srcAccessMask = 0;								// Memory access stage transition must after...
		ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(
			CommandBuffer,
			SrcStage, DstStage,		// Pipeline stages (match to src and dst AccessMasks)
			0,						// Dependency flags
			0, nullptr,				// Memory Barrier count + data
			0, nullptr,				// Buffer Memory Barrier count + data
			1, &ImageMemoryBarrier	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(CommandBuffer);

		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;

		vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(RenderInstance.GraphicsQueue);

		// Copy image data
		CopyBufferToImage(RenderInstance, StagingBuffer.Buffer, ImageBuffeObject.TextureImage, Width, Height);
		
		ImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition from
		ImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

		vkCmdPipelineBarrier(
			CommandBuffer,
			SrcStage, DstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageMemoryBarrier
		);

		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(RenderInstance.GraphicsQueue);

		vkFreeCommandBuffers(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, 1, &CommandBuffer);
		DestroyGenericBuffer(RenderInstance, StagingBuffer);

		//CreateImageView
		ImageBuffeObject.TextureImageView = CreateImageView(RenderInstance, ImageBuffeObject.TextureImage,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		//CreateTextureDescriptor
		VkDescriptorSetAllocateInfo SetAllocInfo = {};
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = RenderInstance.SamplerDescriptorPool;
		SetAllocInfo.descriptorSetCount = 1;
		SetAllocInfo.pSetLayouts = &RenderInstance.SamplerSetLayout;

		VkResult result = vkAllocateDescriptorSets(RenderInstance.LogicalDevice, &SetAllocInfo, &RenderInstance.SamplerDescriptorSets[RenderInstance.TextureImagesCount]);
		if (result != VK_SUCCESS)
		{
			//return -1
		}

		VkDescriptorImageInfo ImageInfo = {};
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// Image layout when in use
		ImageInfo.imageView = ImageBuffeObject.TextureImageView;									// Image to bind to set
		ImageInfo.sampler = RenderInstance.TextureSampler;									// Sampler to use for set

		// Descriptor Write Info
		VkWriteDescriptorSet DescriptorWrite = {};
		DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescriptorWrite.dstSet = RenderInstance.SamplerDescriptorSets[RenderInstance.TextureImagesCount];
		DescriptorWrite.dstBinding = 0;
		DescriptorWrite.dstArrayElement = 0;
		DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		DescriptorWrite.descriptorCount = 1;
		DescriptorWrite.pImageInfo = &ImageInfo;

		// Todo: create descriptor sets for multiple textures?
		vkUpdateDescriptorSets(RenderInstance.LogicalDevice, 1, &DescriptorWrite, 0, nullptr);

		RenderInstance.TextureImageBuffer[RenderInstance.TextureImagesCount] = ImageBuffeObject;
		++RenderInstance.TextureImagesCount;
	}

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, uint32_t AllowedTypes, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

		for (uint32_t MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((AllowedTypes & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties)	// Desired property bit flags are part of memory type's property flags
			{
				// This memory type is valid, so return its index
				return MemoryTypeIndex;
			}
		}

		// Todo Error?
		return 0;
	}

	VkImageView CreateImageView(const VulkanRenderInstance& RenderInstance, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags)
	{
		VkImageViewCreateInfo ViewCreateInfo = {};
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;	

		// Create image view and return it
		VkImageView ImageView;
		VkResult Result = vkCreateImageView(RenderInstance.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		return ImageView;
	}

	VkImage CreateImage(const VulkanRenderInstance& RenderInstance, uint32_t Width, uint32_t Height,
		VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags,
		VkDeviceMemory* OutImageMemory)
	{
		VkImageCreateInfo ImageCreateInfo = {};
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;						// Type of image (1D, 2D, or 3D)
		ImageCreateInfo.extent.width = Width;								// Width of image extent
		ImageCreateInfo.extent.height = Height;								// Height of image extent
		ImageCreateInfo.extent.depth = 1;									// Depth of image (just 1, no 3D aspect)
		ImageCreateInfo.mipLevels = 1;										// Number of mipmap levels
		ImageCreateInfo.arrayLayers = 1;									// Number of levels in image array
		ImageCreateInfo.format = Format;									// Format type of image
		ImageCreateInfo.tiling = Tiling;									// How image data should be "tiled" (arranged for optimal reading)
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout of image data on creation
		ImageCreateInfo.usage = UseFlags;									// Bit flags defining what image will be used for
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Whether image can be shared between queues

		// Create image
		VkImage Image;
		VkResult Result = vkCreateImage(RenderInstance.LogicalDevice, &ImageCreateInfo, nullptr, &Image);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// CREATE MEMORY FOR IMAGE

		// Get memory requirements for a type of image
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(RenderInstance.LogicalDevice, Image, &MemoryRequirements);

		// Allocate memory using image requirements and user defined properties
		VkMemoryAllocateInfo MemoryAllocInfo = {};
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(RenderInstance.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			PropFlags);

		Result = vkAllocateMemory(RenderInstance.LogicalDevice, &MemoryAllocInfo, nullptr, OutImageMemory);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// Connect memory to image
		vkBindImageMemory(RenderInstance.LogicalDevice, Image, *OutImageMemory, 0);

		return Image;
	}

	bool InitMainInstance(MainInstance& Instance, bool IsValidationLayersEnabled)
	{
		const char* ValidationExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		const uint32_t ValidationExtensionsSize = IsValidationLayersEnabled ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

		Core::VkInstanceCreateInfoSetupData InstanceCreateInfoData;
		if (!Core::InitVkInstanceCreateInfoSetupData(InstanceCreateInfoData, ValidationExtensions, ValidationExtensionsSize, IsValidationLayersEnabled))
		{
			return false;
		}

		if (!Core::CheckRequiredInstanceExtensionsSupport(InstanceCreateInfoData))
		{
			Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);
			return false;
		}

		VkApplicationInfo ApplicationInfo = { };
		ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ApplicationInfo.pApplicationName = "Blank App";
		ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.pEngineName = "BMEngine";
		ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
			MessengerCreateInfo.pUserData = nullptr;

			const char* ValidationLayers[] = {
				"VK_LAYER_KHRONOS_validation"
			};
			const uint32_t ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

			Core::CheckValidationLayersSupport(InstanceCreateInfoData, ValidationLayers, ValidationLayersSize);

			CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredAndValidationExtensionsCount;
			CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredAndValidationExtensions;
			CreateInfo.pNext = &MessengerCreateInfo;
			CreateInfo.enabledLayerCount = ValidationLayersSize;
			CreateInfo.ppEnabledLayerNames = ValidationLayers;
		}
		else
		{
			CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredExtensionsCount;
			CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredInstanceExtensions;
			CreateInfo.ppEnabledLayerNames = nullptr;
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}

		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

			return false;
		}

		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

		if (IsValidationLayersEnabled)
		{
			Util::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
		}

		return true;
	}

	void DeinitMainInstance(MainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			Util::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	bool InitVulkanRenderInstance(VulkanRenderInstance& RenderInstance, VkInstance VulkanInstance, GLFWwindow* Window)
	{
		RenderInstance.VulkanInstance = VulkanInstance;
		RenderInstance.Window = Window;

		if (glfwCreateWindowSurface(RenderInstance.VulkanInstance, RenderInstance.Window, nullptr, &RenderInstance.Surface) != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		// Function: SetupPhysicalDevice
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(RenderInstance.VulkanInstance, &DevicesCount, nullptr);

		VkPhysicalDevice* DeviceList = static_cast<VkPhysicalDevice*>(Util::Memory::Allocate(DevicesCount * sizeof(VkPhysicalDevice)));
		vkEnumeratePhysicalDevices(RenderInstance.VulkanInstance, &DevicesCount, DeviceList);

		Core::VkPhysicalDeviceSetupData VkPhysicalDeviceSetupData;

		Core::PhysicalDeviceIndices PhysicalDeviceIndices;
		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

		const char* DeviceExtensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		const uint32_t DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

		for (uint32_t i = 0; i < DevicesCount; ++i)
		{
			if (!Core::InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData, DeviceList[i], RenderInstance.Surface))
			{
				break;
			}

			if (!Core::CheckDeviceExtensionsSupport(VkPhysicalDeviceSetupData, DeviceExtensions, DeviceExtensionsSize))
			{
				break;
			}

			PhysicalDeviceIndices = Core::GetPhysicalDeviceIndices(VkPhysicalDeviceSetupData, DeviceList[i], RenderInstance.Surface);

			if (PhysicalDeviceIndices.GraphicsFamily < 0 || PhysicalDeviceIndices.PresentationFamily < 0)
			{
				Util::Log().Warning("PhysicalDeviceIndices are not initialized");
				break;
			}

			VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceList[i], RenderInstance.Surface, &SurfaceCapabilities);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
				break;
			}

			// Todo support second device if first is not suitable
			RenderInstance.PhysicalDevice = DeviceList[i];

			// ID, name, type, vendor, etc
			vkGetPhysicalDeviceProperties(DeviceList[i], &RenderInstance.PhysicalDeviceProperties);
			//RenderInstance.PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
			/*
			// geo shader, tess shader, wide lines, etc*/
			VkPhysicalDeviceFeatures Features;
			vkGetPhysicalDeviceFeatures(DeviceList[i], &Features);

			if (!Features.samplerAnisotropy)
			{
				break; //  Todo: support if device doues not support Anisotropy
			}
		}

		Util::Memory::Deallocate(DeviceList);

		if (RenderInstance.PhysicalDevice == nullptr)
		{
			Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);
			Util::Log().Error("No physical devices found");
			return false;
		}
		// Function end: SetupPhysicalDevice

		// Function: CreateLogicalDevice
		const float Priority = 1.0f;

		// One family can suppurt graphics and presentation
		// In that case create mulltiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = {};
		uint32_t FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};
		DeviceFeatures.samplerAnisotropy = VK_TRUE; // Todo: get from configs

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = (FamilyIndicesSize);
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		// Queues are created at the same time as the device
		VkResult Result = vkCreateDevice(RenderInstance.PhysicalDevice, &DeviceCreateInfo, nullptr, &RenderInstance.LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end: CreateLogicalDevice

		// Function: SetupQueues
		vkGetDeviceQueue(RenderInstance.LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily), 0, &RenderInstance.GraphicsQueue);
		vkGetDeviceQueue(RenderInstance.LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily), 0, &RenderInstance.PresentationQueue);

		if (RenderInstance.GraphicsQueue == nullptr && RenderInstance.PresentationQueue == nullptr)
		{
			return false;
		}
		// Function end: SetupQueues

		// Function: GetBestSurfaceFormat
		// Return most common format
		VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
		// All formats avalible
		if (VkPhysicalDeviceSetupData.SurfaceFormatsCount == 1 && VkPhysicalDeviceSetupData.SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.SurfaceFormatsCount; ++i)
			{
				VkSurfaceFormatKHR Format = VkPhysicalDeviceSetupData.SurfaceFormats[i];
				if ((Format.format == VK_FORMAT_R8G8B8_UNORM || Format.format == VK_FORMAT_B8G8R8A8_UNORM)
					&& Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					SurfaceFormat = Format;
				}
			}
		}

		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			Util::Log().Error("SurfaceFormat is undefined");
			return false;
		}
		// Function end: GetBestSurfaceFormat

		// Function: GetBestPresentationMode
		// Optimal presentation mode
		VkPresentModeKHR PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.PresentModesCount; ++i)
		{
			if (VkPhysicalDeviceSetupData.PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				PresentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);

		// Has to be present by spec
		if (PresentationMode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Util::Log().Warning("Using default VK_PRESENT_MODE_FIFO_KHR");
		}
		// Function end: GetBestPresentationMode

		// Function: GetBestSwapExtent
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			RenderInstance.SwapExtent = SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(RenderInstance.Window, &Width, &Height);

			Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			RenderInstance.SwapExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
		// Function end: GetBestSwapExtent

		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		uint32_t ImageCount = SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (SurfaceCapabilities.maxImageCount > 0
			&& SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = SurfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = RenderInstance.Surface;
		SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = RenderInstance.SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		uint32_t Indices[] = {
			static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily),
			static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily)
		};

		if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
		{
			// Less efficient mode
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapchainCreateInfo.queueFamilyIndexCount = 2;
			SwapchainCreateInfo.pQueueFamilyIndices = Indices;
		}
		else
		{
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapchainCreateInfo.queueFamilyIndexCount = 0;
			SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		// Used if old cwap chain been destroyed and this one replaces it
		SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		Result = vkCreateSwapchainKHR(RenderInstance.LogicalDevice, &SwapchainCreateInfo, nullptr, &RenderInstance.VulkanSwapchain);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
			return false;
		}

		// Get swap chain images
		vkGetSwapchainImagesKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, &RenderInstance.SwapchainImagesCount, nullptr);

		VkImage* Images = static_cast<VkImage*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkImage)));
		vkGetSwapchainImagesKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, &RenderInstance.SwapchainImagesCount, Images);

		RenderInstance.ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkImageView)));

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			VkImageView ImageView = CreateImageView(RenderInstance, Images[i], SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
			if (ImageView == nullptr)
			{
				Util::Log().Error("vkCreateImageView result is {}", static_cast<int>(Result));
				Util::Memory::Deallocate(Images);
				Util::Memory::Deallocate(RenderInstance.ImageViews);
				return false;
			}

			RenderInstance.ImageViews[i] = ImageView;
		}

		Util::Memory::Deallocate(Images);
		// Function end: CreateSwapchain

		// Function ChooseSupportedFormat
		VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
		VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkFormat BackupFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

		for (uint32_t i = 0; i < 2; ++i)
		{
			VkFormatProperties FormatProperties;
			vkGetPhysicalDeviceFormatProperties(RenderInstance.PhysicalDevice, DepthFormat, &FormatProperties);

			if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				break;
			}

			DepthFormat = BackupFormats[i];
			Util::Log::Warning("Failed to find optimal DepthFormat");
		}
		// Function end ChooseSupportedFormat

		// Function CreateRenderPass
		const uint32_t SubpassesCount = 2;
		VkSubpassDescription Subpasses[SubpassesCount];
		
		// Subpass 1 attachments and references
		VkAttachmentDescription SubpassOneColorAttachment = {};
		SubpassOneColorAttachment.format = ColorFormat;
		SubpassOneColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SubpassOneColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SubpassOneColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SubpassOneColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SubpassOneColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription SubpassOneDepthAttachment = {};
		SubpassOneDepthAttachment.format = DepthFormat;
		SubpassOneDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SubpassOneDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SubpassOneDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SubpassOneDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SubpassOneDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference SubpassOneColourAttachmentReference = {};
		SubpassOneColourAttachmentReference.attachment = 1; // this index is related to Attachments array
		SubpassOneColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference SubpassOneDepthAttachmentReference = {};
		SubpassOneDepthAttachmentReference.attachment = 2; // this index is related to Attachments array
		SubpassOneDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		Subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[0].colorAttachmentCount = 1;
		Subpasses[0].pColorAttachments = &SubpassOneColourAttachmentReference;
		Subpasses[0].pDepthStencilAttachment = &SubpassOneDepthAttachmentReference;

		// Subpass 2 attachments and references
		
		VkAttachmentDescription SwapchainColorAttachment = {};
		SwapchainColorAttachment.format = SurfaceFormat.format;
		SwapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SwapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SwapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		SwapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SwapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Framebuffer data will be stored as an image, but images can be given different data layouts
		// to give optimal use for certain operations
		SwapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
		SwapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

		VkAttachmentReference SwapchainColorAttachmentReference = {};
		// Todo: do not forget about position in array AttachmentDescriptions
		SwapchainColorAttachmentReference.attachment = 0;
		SwapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		const uint32_t InputReferencesCount = 2;
		VkAttachmentReference InputReferences[InputReferencesCount];
		InputReferences[0].attachment = 1; // SubpassOneColorAttachment
		InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		InputReferences[1].attachment = 2; // SubpassOneDepthAttachment
		InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[1].colorAttachmentCount = 1;
		Subpasses[1].pColorAttachments = &SwapchainColorAttachmentReference;
		Subpasses[1].inputAttachmentCount = InputReferencesCount;
		Subpasses[1].pInputAttachments = InputReferences;

		// Subpass dependencies

		// Need to determine when layout transitions occur using subpass dependencies
		const uint32_t SubpassDependenciesSize = 3;
		VkSubpassDependency SubpassDependencies[SubpassDependenciesSize];

		// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		// Transition must happen after...
		SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
		SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
		SubpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
		// But must happen before...
		SubpassDependencies[0].dstSubpass = 0;
		SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[0].dependencyFlags = 0;

		// Subpass 1 layout (colour/depth) to Subpass 2 layout (shader read)
		SubpassDependencies[1].srcSubpass = 0;
		SubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[1].dstSubpass = 1;
		SubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		SubpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		SubpassDependencies[1].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		SubpassDependencies[2].srcSubpass = 0;
		SubpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
		// But must happen before...
		SubpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		SubpassDependencies[2].dependencyFlags = 0;

		// Todo: do not copy atachments to array
		const uint32_t AttachmentDescriptionsCount = 3;
		// must match .attachment
		VkAttachmentDescription AttachmentDescriptions[AttachmentDescriptionsCount] =
			{ SwapchainColorAttachment, SubpassOneColorAttachment, SubpassOneDepthAttachment };

		// Create info for Render Pass
		VkRenderPassCreateInfo RenderPassCreateInfo = {};
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = AttachmentDescriptionsCount;
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
		RenderPassCreateInfo.subpassCount = SubpassesCount;
		RenderPassCreateInfo.pSubpasses = Subpasses;
		RenderPassCreateInfo.dependencyCount = SubpassDependenciesSize;
		RenderPassCreateInfo.pDependencies = SubpassDependencies;

		Result = vkCreateRenderPass(RenderInstance.LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderInstance.RenderPass);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
			return false;
		}

		// Function end CreateRenderPath

		// Function: CreateDescriptorSetLayout
		VkDescriptorSetLayoutBinding VpLayoutBinding = {};
		VpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
		VpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
		VpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		// UNIFORM BUFFER SETUP CODE
		//VkDescriptorSetLayoutBinding ModelLayoutBinding = {};
		//ModelLayoutBinding.binding = 1;
		//ModelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		//ModelLayoutBinding.descriptorCount = 1;
		//ModelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		//ModelLayoutBinding.pImmutableSamplers = nullptr;

		//const uint32_t BindingCount = 2;
		//VkDescriptorSetLayoutBinding Bindings[BindingCount] = { VpLayoutBinding, ModelLayoutBinding };

		const uint32_t BindingCount = 1;
		VkDescriptorSetLayoutBinding Bindings[BindingCount] = { VpLayoutBinding };

		// Create Descriptor Set Layout with given bindings
		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;					// Number of binding infos
		LayoutCreateInfo.pBindings = Bindings;		// Array of binding infos

		Result = vkCreateDescriptorSetLayout(RenderInstance.LogicalDevice, &LayoutCreateInfo, nullptr, &RenderInstance.DescriptorSetLayout);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		//Create input attachment image descriptor set layout
		// Colour Input Binding
		VkDescriptorSetLayoutBinding ColourInputLayoutBinding = {};
		ColourInputLayoutBinding.binding = 0;
		ColourInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		ColourInputLayoutBinding.descriptorCount = 1;
		ColourInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// Depth Input Binding
		VkDescriptorSetLayoutBinding DepthInputLayoutBinding = {};
		DepthInputLayoutBinding.binding = 1;
		DepthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DepthInputLayoutBinding.descriptorCount = 1;
		DepthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// Array of input attachment bindings
		const uint32_t InputBindingsCount = 2;
		// Todo: do not copy InputBindings
		VkDescriptorSetLayoutBinding InputBindings[InputBindingsCount] = {ColourInputLayoutBinding, DepthInputLayoutBinding};

		// Create a descriptor set layout for input attachments
		VkDescriptorSetLayoutCreateInfo InputLayoutCreateInfo = {};
		InputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		InputLayoutCreateInfo.bindingCount = InputBindingsCount;
		InputLayoutCreateInfo.pBindings = InputBindings;

		// Create Descriptor Set Layout
		Result = vkCreateDescriptorSetLayout(RenderInstance.LogicalDevice, &InputLayoutCreateInfo, nullptr, &RenderInstance.InputSetLayout);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// Texture binding info
		VkDescriptorSetLayoutBinding SamplerLayoutBinding = {};
		SamplerLayoutBinding.binding = 0;
		SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SamplerLayoutBinding.descriptorCount = 1;
		SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SamplerLayoutBinding.pImmutableSamplers = nullptr;

		// Create a Descriptor Set Layout with given bindings for texture
		VkDescriptorSetLayoutCreateInfo TextureLayoutCreateInfo = {};
		TextureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		TextureLayoutCreateInfo.bindingCount = 1;
		TextureLayoutCreateInfo.pBindings = &SamplerLayoutBinding;

		// Create Descriptor Set Layout
		Result = vkCreateDescriptorSetLayout(RenderInstance.LogicalDevice, &TextureLayoutCreateInfo, nullptr, &RenderInstance.SamplerSetLayout);
		if (Result != VK_SUCCESS)
		{
			return false;
		}
		// Function end CreateDescriptorSetLayout

		// Function: CreatePushConstantRange
		RenderInstance.PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		RenderInstance.PushConstantRange.offset = 0;
		// Todo: check constant and model size?
		RenderInstance.PushConstantRange.size = sizeof(Model);
		// Function end CreatePushConstantRange

		// Function: CreateGraphicsPipeline
		// TODO FIX!!!
		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");

		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");


		//Build shader modules
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = VertexShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(VertexShaderCode.data());

		VkShaderModule VertexShaderModule = nullptr;
		Result = vkCreateShaderModule(RenderInstance.LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		ShaderModuleCreateInfo.codeSize = FragmentShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(FragmentShaderCode.data());

		VkShaderModule FragmentShaderModule = nullptr;
		Result = vkCreateShaderModule(RenderInstance.LogicalDevice, &ShaderModuleCreateInfo, nullptr, &FragmentShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		VkPipelineShaderStageCreateInfo VertexShaderCreateInfo = {};
		VertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VertexShaderCreateInfo.module = VertexShaderModule;
		VertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo FragmentShaderCreateInfo = {};
		FragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		FragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		FragmentShaderCreateInfo.module = FragmentShaderModule;
		FragmentShaderCreateInfo.pName = "main";

		const uint32_t ShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo ShaderStages[ShaderStagesCount] = { VertexShaderCreateInfo , FragmentShaderCreateInfo };

		// How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
		VkVertexInputBindingDescription VertexInputBindingDescription = {};
		VertexInputBindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
		VertexInputBindingDescription.stride = sizeof(Core::Vertex);						// Size of a single vertex object
		VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
		// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

		// How the data for an attribute is defined within a vertex
		const uint32_t VertexInputBindingDescriptionCount = 3;
		VkVertexInputAttributeDescription AttributeDescriptions[VertexInputBindingDescriptionCount];

		// Position Attribute
		AttributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
		AttributeDescriptions[0].location = 0;							// Location in shader where data will be read from
		AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
		AttributeDescriptions[0].offset = offsetof(Core::Vertex, Position);		// Where this attribute is defined in the data for a single vertex

		// Color Attribute
		AttributeDescriptions[1].binding = 0;
		AttributeDescriptions[1].location = 1;
		AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		AttributeDescriptions[1].offset = offsetof(Core::Vertex, Color);

		// Texture Attribute
		AttributeDescriptions[2].binding = 0;
		AttributeDescriptions[2].location = 2;
		AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		AttributeDescriptions[2].offset = offsetof(Core::Vertex, TextureCoords);

		// Vertex input
		VkPipelineVertexInputStateCreateInfo VertexInputCreateInfo = {};
		VertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		VertexInputCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
		VertexInputCreateInfo.vertexAttributeDescriptionCount = VertexInputBindingDescriptionCount;
		VertexInputCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions;

		// Inputassembly
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport and scissor
		VkViewport Viewport = {};
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = static_cast<float>(RenderInstance.SwapExtent.width);
		Viewport.height = static_cast<float>(RenderInstance.SwapExtent.height);
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = {};
		Scissor.offset = { 0, 0 };
		Scissor.extent = RenderInstance.SwapExtent;

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};
		ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo.viewportCount = 1;
		ViewportStateCreateInfo.pViewports = &Viewport;
		ViewportStateCreateInfo.scissorCount = 1;
		ViewportStateCreateInfo.pScissors = &Scissor;

		// Dynamic states TODO
		// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
		//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
		//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(2);
		//DynamicStateCreateInfo.pDynamicStates = DynamicStates;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};
		RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices
		RasterizationStateCreateInfo.lineWidth = 1.0f;						// How thick lines should be when drawn
		RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
		RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
		RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

		// Multisampling
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = {};
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		// Blending

		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
		ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colors to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachmentState.blendEnable = VK_TRUE;													// Enable blending

		// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
		ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
		//			   (new color alpha * new color) + ((1 - new color alpha) * old color)

		ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

		VkPipelineColorBlendStateCreateInfo ColorBlendingCreateInfo = {};
		ColorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColorBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
		ColorBlendingCreateInfo.attachmentCount = 1;
		ColorBlendingCreateInfo.pAttachments = &ColorBlendAttachmentState;

		// Pipeline layout
		const uint32_t DescriptorSetLayoutsCount = 2;
		VkDescriptorSetLayout DescriptorSetLayouts[DescriptorSetLayoutsCount] = { RenderInstance.DescriptorSetLayout, RenderInstance.SamplerSetLayout };

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = DescriptorSetLayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &RenderInstance.PushConstantRange;

		Result = vkCreatePipelineLayout(RenderInstance.LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &RenderInstance.PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			return false;
		}

		// Depth stencil testing
		VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo = {};
		DepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencilCreateInfo.depthTestEnable = VK_TRUE;				// Enable checking depth to determine fragment write
		DepthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// Enable writing to depth buffer (to replace old values)
		DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// Comparison operation that allows an overwrite (is in front)
		DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth Bounds Test: Does the depth value exist between two bounds
		DepthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable Stencil Test

		// Graphics pipeline creation
		const uint32_t GraphicsPipelineCreateInfosCount = 1;
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.stageCount = ShaderStagesCount;					// Number of shader stages
		GraphicsPipelineCreateInfo.pStages = ShaderStages;							// List of shader stages
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputCreateInfo;		// All the fixed function pipeline states
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState = nullptr;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlendingCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencilCreateInfo;
		GraphicsPipelineCreateInfo.layout = RenderInstance.PipelineLayout;							// Pipeline Layout pipeline should use
		GraphicsPipelineCreateInfo.renderPass = RenderInstance.RenderPass;							// Render pass description the pipeline is compatible with
		GraphicsPipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)


		Result = vkCreateGraphicsPipelines(RenderInstance.LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount, &GraphicsPipelineCreateInfo, nullptr, &RenderInstance.GraphicsPipeline);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			return false;
		}

		vkDestroyShaderModule(RenderInstance.LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(RenderInstance.LogicalDevice, VertexShaderModule, nullptr);
		// Function end: CreateGraphicsPipeline
		
		
		RenderInstance.DepthBuffers = static_cast<GenericImageBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(GenericImageBuffer)));
		RenderInstance.ColorBuffers = static_cast<GenericImageBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(GenericImageBuffer)));

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			// Function CreateDepthBuffer
			RenderInstance.DepthBuffers[i].Image = CreateImage(RenderInstance, RenderInstance.SwapExtent.width, RenderInstance.SwapExtent.height,
				DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&RenderInstance.DepthBuffers[i].ImageMemory);

			RenderInstance.DepthBuffers[i].ImageView = CreateImageView(RenderInstance, RenderInstance.DepthBuffers[i].Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
			// Function end CreateDepthBuffer

			// Function CreateColorBuffer
			// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT says that image can be used only as attachment. Used only for sub pass
			RenderInstance.ColorBuffers[i].Image = CreateImage(RenderInstance, RenderInstance.SwapExtent.width, RenderInstance.SwapExtent.height,
				ColorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&RenderInstance.ColorBuffers[i].ImageMemory);

			RenderInstance.ColorBuffers[i].ImageView = CreateImageView(RenderInstance, RenderInstance.ColorBuffers[i].Image, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			// Function end CreateDepthBuffer
		}

		RenderInstance.SwapchainFramebuffers = static_cast<VkFramebuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkFramebuffer)));

		// Function CreateFrameBuffers
		// Create a framebuffer for each swap chain image
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			const uint32_t AttachmentsCount = 3;
			VkImageView Attachments[AttachmentsCount] = {
				RenderInstance.ImageViews[i],
				// Todo: do not forget about position in array AttachmentDescriptions
				RenderInstance.ColorBuffers[i].ImageView,
				RenderInstance.DepthBuffers[i].ImageView
			};

			VkFramebufferCreateInfo FramebufferCreateInfo = {};
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.renderPass = RenderInstance.RenderPass;								// Render Pass layout the Framebuffer will be used with
			FramebufferCreateInfo.attachmentCount = AttachmentsCount;
			FramebufferCreateInfo.pAttachments = Attachments;							// List of attachments (1:1 with Render Pass)
			FramebufferCreateInfo.width = RenderInstance.SwapExtent.width;								// Framebuffer width
			FramebufferCreateInfo.height = RenderInstance.SwapExtent.height;							// Framebuffer height
			FramebufferCreateInfo.layers = 1;											// Framebuffer layers

			Result = vkCreateFramebuffer(RenderInstance.LogicalDevice, &FramebufferCreateInfo, nullptr, &RenderInstance.SwapchainFramebuffers[i]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateFramebuffer result is {}", static_cast<int>(Result));
				return false;
			}
		}
		// Function end CreateFrameBuffers

		// Function CreateCommandPool
		VkCommandPoolCreateInfo PoolInfo = {};
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = PhysicalDeviceIndices.GraphicsFamily;	// Queue Family type that buffers from this command pool will use

		// Create a Graphics Queue Family Command Pool
		Result = vkCreateCommandPool(RenderInstance.LogicalDevice, &PoolInfo, nullptr, &RenderInstance.GraphicsCommandPool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateCommandPool result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end CreateCommandPool

		// Function CreateCommandBuffers
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = RenderInstance.GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(RenderInstance.SwapchainImagesCount);

		// Allocate command buffers and place handles in array of buffers
		RenderInstance.CommandBuffers = static_cast<VkCommandBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkCommandBuffer)));
		Result = vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &CommandBufferAllocateInfo, RenderInstance.CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end CreateCommandBuffers

		// UNIFORM BUFFER SETUP CODE
		// Function AllocateDynamicBufferTransferSpace
		// Calculate alignment of model data
		//RenderInstance.ModelUniformAlignment = static_cast<uint32_t>((sizeof(UboModel) + RenderInstance.PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment - 1)
		//	& ~(RenderInstance.PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment - 1));

		//// Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
		//RenderInstance.ModelTransferSpace = (UboModel*)_aligned_malloc(RenderInstance.ModelUniformAlignment * RenderInstance.MaxObjects, RenderInstance.ModelUniformAlignment);
		//// Function end AllocateDynamicBufferTransferSpace

		// Function CreateSynchronisation
		RenderInstance.ImageAvalible = static_cast<VkSemaphore*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkSemaphore)));
		RenderInstance.RenderFinished = static_cast<VkSemaphore*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkSemaphore)));
		RenderInstance.DrawFences = static_cast<VkFence*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkFence)));

		VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Fence creation information
		VkFenceCreateInfo FenceCreateInfo = {};
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < RenderInstance.MaxFrameDraws; i++)
		{
			if (vkCreateSemaphore(RenderInstance.LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderInstance.ImageAvalible[i]) != VK_SUCCESS ||
				vkCreateSemaphore(RenderInstance.LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderInstance.RenderFinished[i]) != VK_SUCCESS ||
				vkCreateFence(RenderInstance.LogicalDevice, &FenceCreateInfo, nullptr, &RenderInstance.DrawFences[i]) != VK_SUCCESS)
			{
				Util::Log().Error("CreateSynchronisation error");
				return false;
			}
		}
		// Function end CreateSynchronisation

		// Function CreateUniformBuffers
		const VkDeviceSize VpBufferSize = sizeof(Core::VulkanRenderInstance::UboViewProjection);
		//const VkDeviceSize ModelBufferSize = RenderInstance.ModelUniformAlignment * RenderInstance.MaxObjects;

		RenderInstance.VpUniformBuffers = static_cast<GenericBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(GenericBuffer)));
		// UNIFORM BUFFER SETUP CODE
		//RenderInstance.ModelDynamicUniformBuffers = static_cast<GenericBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(GenericBuffer)));
	
		// Create Uniform buffers
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			CreateGenericBuffer(RenderInstance, VpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, RenderInstance.VpUniformBuffers[i]);

			// UNIFORM BUFFER SETUP CODE
			//CreateGenericBuffer(RenderInstance, ModelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, RenderInstance.ModelDynamicUniformBuffers[i]);
		}

		// Function end CreateUniformBuffers

		// Function CreateDescriptorPool
		VkDescriptorPoolSize VpPoolSize = {};
		VpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VpPoolSize.descriptorCount = RenderInstance.SwapchainImagesCount;

		// UNIFORM BUFFER SETUP CODE
		//VkDescriptorPoolSize ModelPoolSize = {};
		//ModelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		//ModelPoolSize.descriptorCount = RenderInstance.SwapchainImagesCount;

		//const uint32_t PoolSizeCount = 2;
		//VkDescriptorPoolSize PoolSizes[PoolSizeCount] = { VpPoolSize, ModelPoolSize };

		const uint32_t PoolSizeCount = 1;
		VkDescriptorPoolSize PoolSizes[PoolSizeCount] = { VpPoolSize };

		// Data to create Descriptor Pool
		VkDescriptorPoolCreateInfo PoolCreateInfo = {};
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = RenderInstance.SwapchainImagesCount;	// Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount;										// Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes;									// Pool Sizes to create pool with

		// Create Descriptor Pool
		Result = vkCreateDescriptorPool(RenderInstance.LogicalDevice, &PoolCreateInfo, nullptr, &RenderInstance.DescriptorPool);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// Texture sampler pool
		VkDescriptorPoolSize SamplerPoolSize = {};
		// Todo: support VK_DESCRIPTOR_TYPE_SAMPLER and VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE separatly
		SamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SamplerPoolSize.descriptorCount = RenderInstance.MaxObjects; // Todo: max objects or max textures?

		VkDescriptorPoolCreateInfo SamplerPoolCreateInfo = {};
		SamplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		SamplerPoolCreateInfo.maxSets = RenderInstance.MaxObjects;
		SamplerPoolCreateInfo.poolSizeCount = 1;
		SamplerPoolCreateInfo.pPoolSizes = &SamplerPoolSize;

		Result = vkCreateDescriptorPool(RenderInstance.LogicalDevice, &SamplerPoolCreateInfo, nullptr, &RenderInstance.SamplerDescriptorPool);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// CREATE INPUT ATTACHMENT DESCRIPTOR POOL
		VkDescriptorPoolSize ColourInputPoolSize = {};
		ColourInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		ColourInputPoolSize.descriptorCount = RenderInstance.SwapchainImagesCount;

		VkDescriptorPoolSize DepthInputPoolSize = {};
		DepthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DepthInputPoolSize.descriptorCount = RenderInstance.SwapchainImagesCount;

		// Todo: do not copy VkDescriptorPoolSize
		const uint32_t InputPoolSizesCount = 2;
		VkDescriptorPoolSize InputPoolSizes[InputPoolSizesCount] = { ColourInputPoolSize, DepthInputPoolSize };

		VkDescriptorPoolCreateInfo InputPoolCreateInfo = {};
		InputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		InputPoolCreateInfo.maxSets = RenderInstance.SwapchainImagesCount;
		InputPoolCreateInfo.poolSizeCount = InputPoolSizesCount;
		InputPoolCreateInfo.pPoolSizes = InputPoolSizes;

		Result = vkCreateDescriptorPool(RenderInstance.LogicalDevice, &InputPoolCreateInfo, nullptr, &RenderInstance.InputDescriptorPool);
		if (Result != VK_SUCCESS)
		{
			return -1;
		}
		// Function end CreateDescriptorPool

		// Function CreateDescriptorSets
		RenderInstance.DescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkDescriptorSet)));
		RenderInstance.SamplerDescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(RenderInstance.MaxTextures * sizeof(VkDescriptorSet)));
		RenderInstance.InputDescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkDescriptorSet)));

		VkDescriptorSetLayout* SetLayouts = static_cast<VkDescriptorSetLayout*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkDescriptorSetLayout)));
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			SetLayouts[i] = RenderInstance.DescriptorSetLayout;
		}

		// Descriptor Set Allocation Info
		VkDescriptorSetAllocateInfo SetAllocInfo = {};
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = RenderInstance.DescriptorPool;									// Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = RenderInstance.SwapchainImagesCount;	// Number of sets to allocate
		SetAllocInfo.pSetLayouts = SetLayouts;									// Layouts to use to allocate sets (1:1 relationship)

		// Allocate descriptor sets (multiple)
		Result = vkAllocateDescriptorSets(RenderInstance.LogicalDevice, &SetAllocInfo, RenderInstance.DescriptorSets);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		VkDescriptorBufferInfo VpBufferInfo = {};
		// UNIFORM BUFFER SETUP CODE
		//VkDescriptorBufferInfo ModelBufferInfo = {};
		// Update all of descriptor set buffer bindings
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			// Todo: validate
			VpBufferInfo.buffer = RenderInstance.VpUniformBuffers[i].Buffer;
			VpBufferInfo.offset = 0;
			VpBufferInfo.range = sizeof(Core::VulkanRenderInstance::UboViewProjection);

			VkWriteDescriptorSet VpSetWrite = {};
			VpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			VpSetWrite.dstSet = RenderInstance.DescriptorSets[i];
			VpSetWrite.dstBinding = 0;
			VpSetWrite.dstArrayElement = 0;
			VpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VpSetWrite.descriptorCount = 1;
			VpSetWrite.pBufferInfo = &VpBufferInfo;

			// UNIFORM BUFFER SETUP CODE
			//ModelBufferInfo.buffer = RenderInstance.ModelDynamicUniformBuffers[i].Buffer;
			//ModelBufferInfo.offset = 0;
			//ModelBufferInfo.range = RenderInstance.ModelUniformAlignment;

			//VkWriteDescriptorSet ModelSetWrite = {};
			//ModelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			//ModelSetWrite.dstSet = RenderInstance.DescriptorSets[i];
			//ModelSetWrite.dstBinding = 1;
			//ModelSetWrite.dstArrayElement = 0;
			//ModelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			//ModelSetWrite.descriptorCount = 1;
			//ModelSetWrite.pBufferInfo = &ModelBufferInfo;

			//const uint32_t WriteSetsCount = 2;
			//VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite, ModelSetWrite };

			const uint32_t WriteSetsCount = 1;
			// Todo: do not copy
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite };

			// Todo: use one vkUpdateDescriptorSets call to update all descriptors
			vkUpdateDescriptorSets(RenderInstance.LogicalDevice, WriteSetsCount, WriteSets, 0, nullptr);
		}

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			SetLayouts[i] = RenderInstance.InputSetLayout;
		}

		SetAllocInfo.descriptorPool = RenderInstance.InputDescriptorPool;
		SetAllocInfo.descriptorSetCount = RenderInstance.SwapchainImagesCount;

		Result = vkAllocateDescriptorSets(RenderInstance.LogicalDevice, &SetAllocInfo, RenderInstance.InputDescriptorSets);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// Todo: move this and previus loop to function?
		for (size_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			// Todo: move from loop?
			VkDescriptorImageInfo ColourAttachmentDescriptor = {};
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = RenderInstance.ColorBuffers[i].ImageView;
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet ColourWrite = {};
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = RenderInstance.InputDescriptorSets[i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkDescriptorImageInfo DepthAttachmentDescriptor = {};
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = RenderInstance.DepthBuffers[i].ImageView;
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet DepthWrite = {};
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = RenderInstance.InputDescriptorSets[i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;


			// Todo: do not copy
			const uint32_t CetWritesCount = 2;
			VkWriteDescriptorSet SetWrites[CetWritesCount] = {ColourWrite, DepthWrite};

			// Update descriptor sets
			vkUpdateDescriptorSets(RenderInstance.LogicalDevice, CetWritesCount, SetWrites, 0, nullptr);
		}

		Util::Memory::Deallocate(SetLayouts);
		// Function end CreateDescriptorSets

		// Function CreateTextureSampler
		VkSamplerCreateInfo SamplerCreateInfo = {};
		SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
		SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U (x) direction
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V (y) direction
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in W (z) direction
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		SamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		SamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		SamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		SamplerCreateInfo.anisotropyEnable = VK_TRUE;						// Enable Anisotropy
		SamplerCreateInfo.maxAnisotropy = 16; // Todo: support in config

		Result = vkCreateSampler(RenderInstance.LogicalDevice, &SamplerCreateInfo, nullptr, &RenderInstance.TextureSampler);
		if (Result != VK_SUCCESS)
		{
			return false;
		}
		// Function end CreateTextureSampler

		return true;
	}

	bool LoadMesh(VulkanRenderInstance& RenderInstance, Mesh Mesh)
	{
		assert(RenderInstance.DrawableObjectsCount < RenderInstance.MaxObjects);

		const VkDeviceSize VerticesBufferSize = sizeof(Vertex) * Mesh.MeshVerticesCount;
		const VkDeviceSize IndecesBufferSize = sizeof(uint32_t) * Mesh.MeshIndicesCount;
		const VkDeviceSize StagingBufferSize = VerticesBufferSize > IndecesBufferSize ? VerticesBufferSize : IndecesBufferSize;
		void* data = nullptr;

		// Todo: Load many meshes using one buffer?
		GenericBuffer StagingBuffer;
		CreateGenericBuffer(RenderInstance, StagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer);

		// Vertex buffer
		vkMapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory, 0, VerticesBufferSize, 0, &data);
		std::memcpy(data, Mesh.MeshVertices, (size_t)(VerticesBufferSize));
		vkUnmapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory);

		CreateGenericBuffer(RenderInstance, VerticesBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].VertexBuffer);

		CopyBuffer(RenderInstance, StagingBuffer.Buffer, RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].VertexBuffer.Buffer, VerticesBufferSize);

		RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].VerticesCount = Mesh.MeshVerticesCount;
		
		// Index buffer
		vkMapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory, 0, IndecesBufferSize, 0, &data);
		std::memcpy(data, Mesh.MeshIndices, (size_t)(IndecesBufferSize));
		vkUnmapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory);

		CreateGenericBuffer(RenderInstance, IndecesBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].IndexBuffer);

		CopyBuffer(RenderInstance, StagingBuffer.Buffer, RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].IndexBuffer.Buffer, IndecesBufferSize);

		RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].IndicesCount = Mesh.MeshIndicesCount;

		DestroyGenericBuffer(RenderInstance, StagingBuffer);

		RenderInstance.DrawableObjects[RenderInstance.DrawableObjectsCount].Model = glm::mat4(1.0f);

		++RenderInstance.DrawableObjectsCount;

		return true;
	}

	bool Draw(VulkanRenderInstance& RenderInstance)
	{
		vkWaitForFences(RenderInstance.LogicalDevice, 1, &RenderInstance.DrawFences[RenderInstance.CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(RenderInstance.LogicalDevice, 1, &RenderInstance.DrawFences[RenderInstance.CurrentFrame]);

		// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		uint32_t ImageIndex;
		vkAcquireNextImageKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, std::numeric_limits<uint64_t>::max(),
			RenderInstance.ImageAvalible[RenderInstance.CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

		// Function RecordCommands
		VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkRenderPassBeginInfo RenderPassBeginInfo = {};
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = RenderInstance.RenderPass;							// Render Pass to begin
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };						// Start point of render pass in pixels
		RenderPassBeginInfo.renderArea.extent = RenderInstance.SwapExtent;				// Size of region to run render pass on (starting at offset)

		const uint32_t ClearValuesSize = 2;
		VkClearValue ClearValues[ClearValuesSize];
		// Todo: do not forget about position in array AttachmentDescriptions
		ClearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
		ClearValues[1].depthStencil.depth = 1.0f;
		
		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.clearValueCount = ClearValuesSize;

		RenderPassBeginInfo.framebuffer = RenderInstance.SwapchainFramebuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(RenderInstance.CommandBuffers[ImageIndex], &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			return false;
		}

		// Begin Render Pass
		vkCmdBeginRenderPass(RenderInstance.CommandBuffers[ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind Pipeline to be used in render pass
		vkCmdBindPipeline(RenderInstance.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, RenderInstance.GraphicsPipeline);

		// TODO: Support rework to not create identical index buffers
		for (uint32_t j = 0; j < RenderInstance.DrawableObjectsCount; ++j)
		{
			VkBuffer VertexBuffers[] = { RenderInstance.DrawableObjects[j].VertexBuffer.Buffer };					// Buffers to bind
			VkDeviceSize Offsets[] = { 0 };												// Offsets into buffers being bound
			vkCmdBindVertexBuffers(RenderInstance.CommandBuffers[ImageIndex], 0, 1, VertexBuffers, Offsets);

			vkCmdBindIndexBuffer(RenderInstance.CommandBuffers[ImageIndex], RenderInstance.DrawableObjects[j].IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			// UNIFORM BUFFER SETUP CODE
			//uint32_t DynamicOffset = RenderInstance.ModelUniformAlignment * j;

			vkCmdPushConstants(RenderInstance.CommandBuffers[ImageIndex], RenderInstance.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
				0, sizeof(Model), &RenderInstance.DrawableObjects[j].Model);

			// Todo: do not record textureId on each frame?
			const uint32_t DescriptorSetGroupCount = 2;
			VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] = { RenderInstance.DescriptorSets[ImageIndex],
				RenderInstance.SamplerDescriptorSets[RenderInstance.DrawableObjects[j].TextureId] };

			vkCmdBindDescriptorSets(RenderInstance.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, RenderInstance.PipelineLayout,
				0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			// Execute pipeline
			//vkCmdDraw(RenderInstance.CommandBuffers[i], RenderInstance.DrawableObject.VerticesCount, 1, 0, 0);
			vkCmdDrawIndexed(RenderInstance.CommandBuffers[ImageIndex], RenderInstance.DrawableObjects[j].IndicesCount, 1, 0, 0, 0);
		}

		// End Render Pass
		vkCmdEndRenderPass(RenderInstance.CommandBuffers[ImageIndex]);

		Result = vkEndCommandBuffer(RenderInstance.CommandBuffers[ImageIndex]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end RecordCommands

		//UpdateUniformBuffer
		void* Data;
		vkMapMemory(RenderInstance.LogicalDevice, RenderInstance.VpUniformBuffers[ImageIndex].BufferMemory, 0, sizeof(Core::VulkanRenderInstance::UboViewProjection),
			0, &Data);
		std::memcpy(Data, &RenderInstance.ViewProjection, sizeof(Core::VulkanRenderInstance::UboViewProjection));
		vkUnmapMemory(RenderInstance.LogicalDevice, RenderInstance.VpUniformBuffers[ImageIndex].BufferMemory);

		// UNIFORM BUFFER SETUP CODE
		//for (uint32_t i = 0; i < RenderInstance.DrawableObjectsCount; ++i)
		//{
		//	UboModel* ThisModel = (UboModel*)((uint64_t)RenderInstance.ModelTransferSpace + (i * RenderInstance.ModelUniformAlignment));
		//	*ThisModel = RenderInstance.DrawableObjects[i].Model;
		//}

		//vkMapMemory(RenderInstance.LogicalDevice, RenderInstance.ModelDynamicUniformBuffers[ImageIndex].BufferMemory, 0,
		//	RenderInstance.ModelUniformAlignment * RenderInstance.DrawableObjectsCount, 0, &Data);

		//std::memcpy(Data, RenderInstance.ModelTransferSpace, RenderInstance.ModelUniformAlignment * RenderInstance.DrawableObjectsCount);
		//vkUnmapMemory(RenderInstance.LogicalDevice, RenderInstance.ModelDynamicUniformBuffers[ImageIndex].BufferMemory);

		// -- SUBMIT COMMAND BUFFER TO RENDER --
		// Queue submission information
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		submitInfo.pWaitSemaphores = &RenderInstance.ImageAvalible[RenderInstance.CurrentFrame];				// List of semaphores to wait on
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
		submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		submitInfo.pCommandBuffers = &RenderInstance.CommandBuffers[ImageIndex];		// Command buffer to submit
		submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
		submitInfo.pSignalSemaphores = &RenderInstance.RenderFinished[RenderInstance.CurrentFrame];	// Semaphores to signal when command buffer finishes

		// Submit command buffer to queue
		Result = vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &submitInfo, RenderInstance.DrawFences[RenderInstance.CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
			return false;
		}

		// -- PRESENT RENDERED IMAGE TO SCREEN --
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		presentInfo.pWaitSemaphores = &RenderInstance.RenderFinished[RenderInstance.CurrentFrame];			// Semaphores to wait on
		presentInfo.swapchainCount = 1;											// Number of swapchains to present to
		presentInfo.pSwapchains = &RenderInstance.VulkanSwapchain;									// Swapchains to present images to
		presentInfo.pImageIndices = &ImageIndex;								// Index of images in swapchains to present

		// Present image
		Result = vkQueuePresentKHR(RenderInstance.PresentationQueue, &presentInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
			return false;
		}

		RenderInstance.CurrentFrame = (RenderInstance.CurrentFrame + 1) % RenderInstance.MaxFrameDraws;

		return true;
	}

	void DeinitVulkanRenderInstance(VulkanRenderInstance& RenderInstance)
	{
		vkDeviceWaitIdle(RenderInstance.LogicalDevice);

		// UNIFORM BUFFER SETUP CODE
		//_aligned_free(RenderInstance.ModelTransferSpace);

		vkDestroyDescriptorPool(RenderInstance.LogicalDevice, RenderInstance.InputDescriptorPool, nullptr);

		vkDestroySampler(RenderInstance.LogicalDevice, RenderInstance.TextureSampler, nullptr);

		vkDestroyDescriptorSetLayout(RenderInstance.LogicalDevice, RenderInstance.InputSetLayout, nullptr);

		for (uint32_t i = 0; i < RenderInstance.TextureImagesCount; ++i)
		{
			vkDestroyImageView(RenderInstance.LogicalDevice, RenderInstance.TextureImageBuffer[i].TextureImageView, nullptr);
			vkDestroyImage(RenderInstance.LogicalDevice, RenderInstance.TextureImageBuffer[i].TextureImage, nullptr);
			vkFreeMemory(RenderInstance.LogicalDevice, RenderInstance.TextureImageBuffer[i].TextureImagesMemory, nullptr);
		}

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			vkDestroyImageView(RenderInstance.LogicalDevice, RenderInstance.DepthBuffers[i].ImageView, nullptr);
			vkDestroyImage(RenderInstance.LogicalDevice, RenderInstance.DepthBuffers[i].Image, nullptr);
			vkFreeMemory(RenderInstance.LogicalDevice, RenderInstance.DepthBuffers[i].ImageMemory, nullptr);

			vkDestroyImageView(RenderInstance.LogicalDevice, RenderInstance.ColorBuffers[i].ImageView, nullptr);
			vkDestroyImage(RenderInstance.LogicalDevice, RenderInstance.ColorBuffers[i].Image, nullptr);
			vkFreeMemory(RenderInstance.LogicalDevice, RenderInstance.ColorBuffers[i].ImageMemory, nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.DepthBuffers);
		Util::Memory::Deallocate(RenderInstance.ColorBuffers);

		Util::Memory::Deallocate(RenderInstance.SamplerDescriptorSets);
		Util::Memory::Deallocate(RenderInstance.DescriptorSets);

		vkDestroyDescriptorPool(RenderInstance.LogicalDevice, RenderInstance.SamplerDescriptorPool, nullptr);
		vkDestroyDescriptorPool(RenderInstance.LogicalDevice, RenderInstance.DescriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(RenderInstance.LogicalDevice, RenderInstance.SamplerSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(RenderInstance.LogicalDevice, RenderInstance.DescriptorSetLayout, nullptr);
		
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			Core::DestroyGenericBuffer(RenderInstance, RenderInstance.VpUniformBuffers[i]);
			// UNIFORM BUFFER SETUP CODE
			//Core::DestroyGenericBuffer(RenderInstance, RenderInstance.ModelDynamicUniformBuffers[i]);
		}

		Util::Memory::Deallocate(RenderInstance.VpUniformBuffers);
		// UNIFORM BUFFER SETUP CODE
		//Util::Memory::Deallocate(RenderInstance.ModelDynamicUniformBuffers);

		for (uint32_t i = 0; i < RenderInstance.DrawableObjectsCount; ++i)
		{
			DestroyGenericBuffer(RenderInstance, RenderInstance.DrawableObjects[i].VertexBuffer);
			DestroyGenericBuffer(RenderInstance, RenderInstance.DrawableObjects[i].IndexBuffer);
		}

		for (size_t i = 0; i < RenderInstance.MaxFrameDraws; i++)
		{
			vkDestroySemaphore(RenderInstance.LogicalDevice, RenderInstance.RenderFinished[i], nullptr);
			vkDestroySemaphore(RenderInstance.LogicalDevice, RenderInstance.ImageAvalible[i], nullptr);
			vkDestroyFence(RenderInstance.LogicalDevice, RenderInstance.DrawFences[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.DrawFences);
		Util::Memory::Deallocate(RenderInstance.RenderFinished);
		Util::Memory::Deallocate(RenderInstance.ImageAvalible);

		Util::Memory::Deallocate(RenderInstance.CommandBuffers);

		vkDestroyCommandPool(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, nullptr);

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			vkDestroyFramebuffer(RenderInstance.LogicalDevice, RenderInstance.SwapchainFramebuffers[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.SwapchainFramebuffers);

		vkDestroyPipeline(RenderInstance.LogicalDevice, RenderInstance.GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(RenderInstance.LogicalDevice, RenderInstance.PipelineLayout, nullptr);
		vkDestroyRenderPass(RenderInstance.LogicalDevice, RenderInstance.RenderPass, nullptr);

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			vkDestroyImageView(RenderInstance.LogicalDevice, RenderInstance.ImageViews[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.ImageViews);

		vkDestroySwapchainKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, nullptr);
		vkDestroySurfaceKHR(RenderInstance.VulkanInstance, RenderInstance.Surface, nullptr);
		vkDestroyDevice(RenderInstance.LogicalDevice, nullptr);
	}
}

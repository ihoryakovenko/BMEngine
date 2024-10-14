#include "VulkanHelper.h"

#include "Util/Util.h"
#include <cassert>

namespace Core
{
	bool CreateShader(VkDevice LogicalDevice, const u32* Code, u32 CodeSize,
		VkShaderModule &VertexShaderModule)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = CodeSize;
		ShaderModuleCreateInfo.pCode = Code;

		VkResult Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}

	VkImageView CreateImageView(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
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
		VkResult Result = vkCreateImageView(LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			// Todo Error?
			return nullptr;
		}

		return ImageView;
	}

	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

		for (u32 MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((AllowedTypes & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties)	// Desired property bit flags are part of memory type's property flags
			{
				// This memory type is valid, so return its index
				return MemoryTypeIndex;
			}
		}

		assert(false);
		return 0;
	}

	VkImage CreateImage(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 Width, u32 Height,
		VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags,
		VkDeviceMemory* OutImageMemory)
	{
		VkImageCreateInfo ImageCreateInfo = { };
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
		VkResult Result = vkCreateImage(LogicalDevice, &ImageCreateInfo, nullptr, &Image);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// CREATE MEMORY FOR IMAGE

		// Get memory requirements for a type of image
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(LogicalDevice, Image, &MemoryRequirements);

		// Allocate memory using image requirements and user defined properties
		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits,
			PropFlags);

		Result = vkAllocateMemory(LogicalDevice, &MemoryAllocInfo, nullptr, OutImageMemory);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// Connect memory to image
		vkBindImageMemory(LogicalDevice, Image, *OutImageMemory, 0);

		return Image;
	}
}
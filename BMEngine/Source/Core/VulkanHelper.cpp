#include "VulkanHelper.h"

#include "Util/Util.h"
#include <cassert>

namespace Core
{
	bool CreateShader(VkDevice LogicalDevice, const uint32_t* Code, uint32_t CodeSize,
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

	VkDescriptorPool CreateDescriptorPool(VkDevice LogicalDevice, VkDescriptorPoolSize* PoolSizes, uint32_t PoolSizeCount, uint32_t MaxDescriptorCount)
	{
		Util::Log().Info("Creating descriptor pool. Size count: {}", PoolSizeCount);

		for (int i = 0; i < PoolSizeCount; ++i)
		{
			Util::Log().Info("Type: {}, Count: {}", static_cast<int>(PoolSizes[i].type), PoolSizes[i].descriptorCount);
		}

		Util::Log().Info("Maximum descriptor count: {}", MaxDescriptorCount);

		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = MaxDescriptorCount; // Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount; // Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes; // Pool Sizes to create pool with

		VkDescriptorPool Pool;
		VkResult Result = vkCreateDescriptorPool(LogicalDevice, &PoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorPool result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Pool;
	}
}
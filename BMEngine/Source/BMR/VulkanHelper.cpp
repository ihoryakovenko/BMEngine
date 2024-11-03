#include "VulkanHelper.h"

#include <cassert>
#include <stdarg.h>

namespace BMR
{
	static BMRLogHandler LogHandler = nullptr;

	void SetLogHandler(BMRLogHandler Handler)
	{
		LogHandler = Handler;
	}

	void HandleLog(BMRLogType LogType, const char* Format, ...)
	{
		if (LogHandler != nullptr)
		{
			va_list Args;
			va_start(Args, Format);
			LogHandler(LogType, Format, Args);
			va_end(Args);
		}
	}

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
			HandleLog(BMRLogType::LogType_Error, "vkCreateShaderModule result is %d", Result);
			return false;
		}

		return true;
	}

	VkImageView CreateImageView(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags, VkImageViewType Type, u32 LayerCount)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = Type;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = LayerCount;

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
}
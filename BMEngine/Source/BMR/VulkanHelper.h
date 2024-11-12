#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"
#include "BMR/BMRInterfaceTypes.h"

namespace BMR
{
	void SetLogHandler(BMRLogHandler Handler);

	void HandleLog(BMRLogType LogType, const char* Format, ...);

	bool CreateShader(VkDevice LogicalDevice, const u32* Code, u32 CodeSize,
		VkShaderModule& VertexShaderModule);

	VkImageView CreateImageView(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags, VkImageViewType Type, u32 LayerCount, u32 BaseArrayLayer = 0);

	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties);

	VkExtent2D ToVkExtent2D(BMRExtent2D Extent);
	VkPolygonMode ToVkPolygonMode(BMRPolygonMode Mode);
	VkCullModeFlags ToVkCullMode(BMRCullMode Mode);
	VkFrontFace ToVkFrontFace(BMRFrontFace Face);
	VkBlendFactor ToVkBlendFactor(BMRBlendFactor Factor);
	VkBlendOp ToVkBlendOp(BMRBlendOp Op);
	VkCompareOp ToVkCompareOp(BMRCompareOp Op);
	VkColorComponentFlags ToVkColorComponentFlags(BMRColorComponentFlagBits Flags);
}

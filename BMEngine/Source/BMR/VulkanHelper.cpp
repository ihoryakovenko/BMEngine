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
		VkImageAspectFlags AspectFlags, VkImageViewType Type, u32 LayerCount, u32 BaseArrayLayer)
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
		ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
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

	VkExtent2D ToVkExtent2D(BMRExtent2D Extent)
	{
		return { Extent.Width, Extent.Height };
	}

	VkPolygonMode ToVkPolygonMode(BMRPolygonMode Mode)
	{
		switch (Mode)
		{
			case Fill: return VK_POLYGON_MODE_FILL;
			case Line: return VK_POLYGON_MODE_LINE;
			case Point: return VK_POLYGON_MODE_POINT;
			default: return VK_POLYGON_MODE_FILL;
		}
	}

	VkCullModeFlags ToVkCullMode(BMRCullMode Mode)
	{
		switch (Mode)
		{
			case None: return VK_CULL_MODE_NONE;
			case Front: return VK_CULL_MODE_FRONT_BIT;
			case Back: return VK_CULL_MODE_BACK_BIT;
			case FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
			default: return VK_CULL_MODE_NONE;
		}
	}

	VkFrontFace ToVkFrontFace(BMRFrontFace Face)
	{
		switch (Face)
		{
			case CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			case Clockwise: return VK_FRONT_FACE_CLOCKWISE;
			default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

	VkBlendFactor ToVkBlendFactor(BMRBlendFactor Factor)
	{
		switch (Factor)
		{
			case Zero: return VK_BLEND_FACTOR_ZERO;
			case One: return VK_BLEND_FACTOR_ONE;
			case SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
			case DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
			case OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			default: return VK_BLEND_FACTOR_ONE;
		}
	}

	VkBlendOp ToVkBlendOp(BMRBlendOp Op)
	{
		switch (Op)
		{
			case Add: return VK_BLEND_OP_ADD;
			case Subtract: return VK_BLEND_OP_SUBTRACT;
			case ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case Min: return VK_BLEND_OP_MIN;
			default: return VK_BLEND_OP_ADD;
		}
	}

	VkCompareOp ToVkCompareOp(BMRCompareOp Op)
	{
		switch (Op)
		{
			case Never: return VK_COMPARE_OP_NEVER;
			case Less: return VK_COMPARE_OP_LESS;
			case Equal: return VK_COMPARE_OP_EQUAL;
			case LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case Greater: return VK_COMPARE_OP_GREATER;
			default: return VK_COMPARE_OP_ALWAYS;
		}
	}

	VkColorComponentFlags ToVkColorComponentFlags(BMRColorComponentFlagBits flags)
	{
		return static_cast<VkColorComponentFlags>(flags);
	}

	VkVertexInputRate ToVkVertexInputRate(BMRVertexInputRate Rare)
	{
		switch (Rare)
		{
			case BMRVertexInputRate_Vertex:   return VK_VERTEX_INPUT_RATE_VERTEX;
			case BMRVertexInputRate_Instance: return VK_VERTEX_INPUT_RATE_INSTANCE;
			default:       return VK_VERTEX_INPUT_RATE_MAX_ENUM;
		}
	}

	VkFormat ToVkFormat(BMRFormat Format)
	{
		switch (Format)
		{
			case R32_SF:		  return VK_FORMAT_R32_SFLOAT;
			case R32G32_SF:       return VK_FORMAT_R32G32_SFLOAT;
			case R32G32B32_SF:    return VK_FORMAT_R32G32B32_SFLOAT;
			default:              return VK_FORMAT_UNDEFINED;
		}
	}
}
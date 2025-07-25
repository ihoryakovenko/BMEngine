#pragma once

#include <unordered_map>
#include <string>

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

struct GLFWwindow;

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

#define VULKAN_CHECK_RESULT(call) \
	{ \
		const VkResult result = (call); \
		if (result != VK_SUCCESS) { \
			Util::RenderLog(Util::BMRVkLogType_Error, "%s returned %d at %s:%d", #call, result, __FILE__, __LINE__); \
		} \
	}

namespace VulkanHelper
{
	inline constexpr const u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	inline constexpr const u32 MAX_VERTEX_INPUT_BINDINGS = 16;

	// String constants for parsing functions
	namespace ParseStrings
	{
		// Boolean values
		inline constexpr const char* TRUE_STRINGS[] = { "true", "1", "yes", "on" };
		inline constexpr const char* FALSE_STRINGS[] = { "false", "0", "no", "off" };

		// Polygon modes
		inline constexpr const char* FILL_STRINGS[] = { "fill", "FILL" };
		inline constexpr const char* LINE_STRINGS[] = { "line", "LINE" };
		inline constexpr const char* POINT_STRINGS[] = { "point", "POINT" };

		// Cull modes
		inline constexpr const char* NONE_STRINGS[] = { "none", "NONE" };
		inline constexpr const char* BACK_STRINGS[] = { "back", "BACK" };
		inline constexpr const char* FRONT_STRINGS[] = { "front", "FRONT" };
		inline constexpr const char* FRONT_BACK_STRINGS[] = { "front_back", "FRONT_BACK" };

		// Front face
		inline constexpr const char* COUNTER_CLOCKWISE_STRINGS[] = { "counter_clockwise", "COUNTER_CLOCKWISE" };
		inline constexpr const char* CLOCKWISE_STRINGS[] = { "clockwise", "CLOCKWISE" };

		// Blend factors
		inline constexpr const char* ONE_MINUS_SRC_ALPHA_STRINGS[] = { "one_minus_src_alpha", "ONE_MINUS_SRC_ALPHA" };
		inline constexpr const char* ONE_MINUS_DST_ALPHA_STRINGS[] = { "one_minus_dst_alpha", "ONE_MINUS_DST_ALPHA" };
		inline constexpr const char* ONE_MINUS_SRC_COLOR_STRINGS[] = { "one_minus_src_color", "ONE_MINUS_SRC_COLOR" };
		inline constexpr const char* ONE_MINUS_DST_COLOR_STRINGS[] = { "one_minus_dst_color", "ONE_MINUS_DST_COLOR" };
		inline constexpr const char* SRC_COLOR_STRINGS[] = { "src_color", "SRC_COLOR" };
		inline constexpr const char* DST_COLOR_STRINGS[] = { "dst_color", "DST_COLOR" };
		inline constexpr const char* SRC_ALPHA_STRINGS[] = { "src_alpha", "SRC_ALPHA" };
		inline constexpr const char* DST_ALPHA_STRINGS[] = { "dst_alpha", "DST_ALPHA" };
		inline constexpr const char* ZERO_STRINGS[] = { "zero", "ZERO" };
		inline constexpr const char* ONE_STRINGS[] = { "one", "ONE" };

		// Blend operations
		inline constexpr const char* REVERSE_SUBTRACT_STRINGS[] = { "reverse_subtract", "REVERSE_SUBTRACT" };
		inline constexpr const char* SUBTRACT_STRINGS[] = { "subtract", "SUBTRACT" };
		inline constexpr const char* ADD_STRINGS[] = { "add", "ADD" };
		inline constexpr const char* MIN_STRINGS[] = { "min", "MIN" };
		inline constexpr const char* MAX_STRINGS[] = { "max", "MAX" };

		// Compare operations
		inline constexpr const char* NEVER_STRINGS[] = { "never", "NEVER" };
		inline constexpr const char* LESS_STRINGS[] = { "less", "LESS" };
		inline constexpr const char* EQUAL_STRINGS[] = { "equal", "EQUAL" };
		inline constexpr const char* LESS_OR_EQUAL_STRINGS[] = { "less_or_equal", "LESS_OR_EQUAL" };
		inline constexpr const char* GREATER_STRINGS[] = { "greater", "GREATER" };
		inline constexpr const char* NOT_EQUAL_STRINGS[] = { "not_equal", "NOT_EQUAL" };
		inline constexpr const char* GREATER_OR_EQUAL_STRINGS[] = { "greater_or_equal", "GREATER_OR_EQUAL" };
		inline constexpr const char* ALWAYS_STRINGS[] = { "always", "ALWAYS" };

		// Sample counts
		inline constexpr const char* SAMPLE_1_STRINGS[] = { "1" };
		inline constexpr const char* SAMPLE_2_STRINGS[] = { "2" };
		inline constexpr const char* SAMPLE_4_STRINGS[] = { "4" };
		inline constexpr const char* SAMPLE_8_STRINGS[] = { "8" };
		inline constexpr const char* SAMPLE_16_STRINGS[] = { "16" };
		inline constexpr const char* SAMPLE_32_STRINGS[] = { "32" };
		inline constexpr const char* SAMPLE_64_STRINGS[] = { "64" };

		// Primitive topologies
		inline constexpr const char* POINT_LIST_STRINGS[] = { "point_list", "POINT_LIST" };
		inline constexpr const char* LINE_LIST_STRINGS[] = { "line_list", "LINE_LIST" };
		inline constexpr const char* LINE_STRIP_STRINGS[] = { "line_strip", "LINE_STRIP" };
		inline constexpr const char* TRIANGLE_LIST_STRINGS[] = { "triangle_list", "TRIANGLE_LIST" };
		inline constexpr const char* TRIANGLE_STRIP_STRINGS[] = { "triangle_strip", "TRIANGLE_STRIP" };
		inline constexpr const char* TRIANGLE_FAN_STRINGS[] = { "triangle_fan", "TRIANGLE_FAN" };
		inline constexpr const char* LINE_LIST_WITH_ADJACENCY_STRINGS[] = { "line_list_with_adjacency", "LINE_LIST_WITH_ADJACENCY" };
		inline constexpr const char* LINE_STRIP_WITH_ADJACENCY_STRINGS[] = { "line_strip_with_adjacency", "LINE_STRIP_WITH_ADJACENCY" };
		inline constexpr const char* TRIANGLE_LIST_WITH_ADJACENCY_STRINGS[] = { "triangle_list_with_adjacency", "TRIANGLE_LIST_WITH_ADJACENCY" };
		inline constexpr const char* TRIANGLE_STRIP_WITH_ADJACENCY_STRINGS[] = { "triangle_strip_with_adjacency", "TRIANGLE_STRIP_WITH_ADJACENCY" };
		inline constexpr const char* PATCH_LIST_STRINGS[] = { "patch_list", "PATCH_LIST" };

		// Shader stages
		inline constexpr const char* VERTEX_SHADER_STRINGS[] = { "vertex", "VERTEX" };
		inline constexpr const char* FRAGMENT_STRINGS[] = { "fragment", "FRAGMENT" };
		inline constexpr const char* GEOMETRY_STRINGS[] = { "geometry", "GEOMETRY" };
		inline constexpr const char* COMPUTE_STRINGS[] = { "compute", "COMPUTE" };
		inline constexpr const char* TESS_CONTROL_STRINGS[] = { "tess_control", "TESS_CONTROL" };
		inline constexpr const char* TESS_EVAL_STRINGS[] = { "tess_eval", "TESS_EVAL" };
		inline constexpr const char* TASK_STRINGS[] = { "task", "TASK" };
		inline constexpr const char* MESH_STRINGS[] = { "mesh", "MESH" };
		inline constexpr const char* RAYGEN_STRINGS[] = { "raygen", "RAYGEN" };
		inline constexpr const char* CLOSEST_HIT_STRINGS[] = { "closest_hit", "CLOSEST_HIT" };
		inline constexpr const char* ANY_HIT_STRINGS[] = { "any_hit", "ANY_HIT" };
		inline constexpr const char* MISS_STRINGS[] = { "miss", "MISS" };
		inline constexpr const char* INTERSECTION_STRINGS[] = { "intersection", "INTERSECTION" };
		inline constexpr const char* CALLABLE_STRINGS[] = { "callable", "CALLABLE" };

		// Filters
		inline constexpr const char* NEAREST_STRINGS[] = { "NEAREST" };
		inline constexpr const char* LINEAR_STRINGS[] = { "LINEAR" };

		// Address modes
		inline constexpr const char* REPEAT_STRINGS[] = { "REPEAT" };
		inline constexpr const char* MIRRORED_REPEAT_STRINGS[] = { "MIRRORED_REPEAT" };
		inline constexpr const char* CLAMP_TO_EDGE_STRINGS[] = { "CLAMP_TO_EDGE" };
		inline constexpr const char* CLAMP_TO_BORDER_STRINGS[] = { "CLAMP_TO_BORDER" };
		inline constexpr const char* MIRROR_CLAMP_TO_EDGE_STRINGS[] = { "MIRROR_CLAMP_TO_EDGE" };

		// Border colors
		inline constexpr const char* FLOAT_TRANSPARENT_BLACK_STRINGS[] = { "FLOAT_TRANSPARENT_BLACK" };
		inline constexpr const char* INT_TRANSPARENT_BLACK_STRINGS[] = { "INT_TRANSPARENT_BLACK" };
		inline constexpr const char* FLOAT_OPAQUE_BLACK_STRINGS[] = { "FLOAT_OPAQUE_BLACK" };
		inline constexpr const char* INT_OPAQUE_BLACK_STRINGS[] = { "INT_OPAQUE_BLACK" };
		inline constexpr const char* FLOAT_OPAQUE_WHITE_STRINGS[] = { "FLOAT_OPAQUE_WHITE" };
		inline constexpr const char* INT_OPAQUE_WHITE_STRINGS[] = { "INT_OPAQUE_WHITE" };

		// Mipmap modes
		inline constexpr const char* MIPMAP_NEAREST_STRINGS[] = { "NEAREST" };
		inline constexpr const char* MIPMAP_LINEAR_STRINGS[] = { "LINEAR" };

		// Descriptor types
		inline constexpr const char* SAMPLER_STRINGS[] = { "SAMPLER" };
		inline constexpr const char* COMBINED_IMAGE_SAMPLER_STRINGS[] = { "COMBINED_IMAGE_SAMPLER" };
		inline constexpr const char* SAMPLED_IMAGE_STRINGS[] = { "SAMPLED_IMAGE" };
		inline constexpr const char* STORAGE_IMAGE_STRINGS[] = { "STORAGE_IMAGE" };
		inline constexpr const char* UNIFORM_TEXEL_BUFFER_STRINGS[] = { "UNIFORM_TEXEL_BUFFER" };
		inline constexpr const char* STORAGE_TEXEL_BUFFER_STRINGS[] = { "STORAGE_TEXEL_BUFFER" };
		inline constexpr const char* UNIFORM_BUFFER_STRINGS[] = { "UNIFORM_BUFFER" };
		inline constexpr const char* STORAGE_BUFFER_STRINGS[] = { "STORAGE_BUFFER" };
		inline constexpr const char* UNIFORM_BUFFER_DYNAMIC_STRINGS[] = { "UNIFORM_BUFFER_DYNAMIC" };
		inline constexpr const char* STORAGE_BUFFER_DYNAMIC_STRINGS[] = { "STORAGE_BUFFER_DYNAMIC" };
		inline constexpr const char* INPUT_ATTACHMENT_STRINGS[] = { "INPUT_ATTACHMENT" };

		// Shader stage flags
		inline constexpr const char* VERTEX_BIT_STRINGS[] = { "VERTEX_BIT" };
		inline constexpr const char* FRAGMENT_BIT_STRINGS[] = { "FRAGMENT_BIT" };
		inline constexpr const char* GEOMETRY_BIT_STRINGS[] = { "GEOMETRY_BIT" };
		inline constexpr const char* COMPUTE_BIT_STRINGS[] = { "COMPUTE_BIT" };
		inline constexpr const char* TESSELLATION_CONTROL_BIT_STRINGS[] = { "TESSELLATION_CONTROL_BIT" };
		inline constexpr const char* TESSELLATION_EVALUATION_BIT_STRINGS[] = { "TESSELLATION_EVALUATION_BIT" };
		inline constexpr const char* TASK_BIT_STRINGS[] = { "TASK_BIT" };
		inline constexpr const char* MESH_BIT_STRINGS[] = { "MESH_BIT" };
		inline constexpr const char* RAYGEN_BIT_STRINGS[] = { "RAYGEN_BIT" };
		inline constexpr const char* CLOSEST_HIT_BIT_STRINGS[] = { "CLOSEST_HIT_BIT" };
		inline constexpr const char* ANY_HIT_BIT_STRINGS[] = { "ANY_HIT_BIT" };
		inline constexpr const char* MISS_BIT_STRINGS[] = { "MISS_BIT" };
		inline constexpr const char* INTERSECTION_BIT_STRINGS[] = { "INTERSECTION_BIT" };
		inline constexpr const char* CALLABLE_BIT_STRINGS[] = { "CALLABLE_BIT" };

		// Formats
		inline constexpr const char* R32_SFLOAT_STRINGS[] = { "R32_SFLOAT" };
		inline constexpr const char* R32G32_SFLOAT_STRINGS[] = { "R32G32_SFLOAT" };
		inline constexpr const char* R32G32B32_SFLOAT_STRINGS[] = { "R32G32B32_SFLOAT" };
		inline constexpr const char* R32G32B32A32_SFLOAT_STRINGS[] = { "R32G32B32A32_SFLOAT" };
		inline constexpr const char* R32_UINT_STRINGS[] = { "R32_UINT" };

		// Vertex input rates
		inline constexpr const char* VERTEX_STRINGS[] = { "VERTEX" };
		inline constexpr const char* INSTANCE_STRINGS[] = { "INSTANCE" };
	}

	enum BufferUsageFlag
	{
		UniformFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		StagingFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		StorageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VertexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		IndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		CombinedVertexIndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		InstanceFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	};

	enum MemoryPropertyFlag
	{
		GPULocal = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		HostCompatible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	struct PhysicalDeviceIndices
	{
		s32 GraphicsFamily;
		s32 PresentationFamily;
		s32 TransferFamily;
	};

	struct DeviceMemoryAllocResult
	{
		VkDeviceMemory Memory;
		u64 Alignment;
		u64 Size;
	};

	struct Shader
	{
		VkShaderStageFlagBits Stage;
		const char* Code;
		u32 CodeSize;

		// TODO: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		Memory::DynamicHeapArray<char> ShaderCode;
	};

	struct PipelineSettings
	{
		Memory::DynamicHeapArray<VkPipelineShaderStageCreateInfo> Shaders;

		VkExtent2D Extent;
		VkPipelineRasterizationStateCreateInfo RasterizationState;
		VkPipelineColorBlendStateCreateInfo ColorBlendState;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineDepthStencilStateCreateInfo DepthStencilState;
		VkPipelineMultisampleStateCreateInfo MultisampleState;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
		VkPipelineViewportStateCreateInfo ViewportState;
		VkViewport Viewport;
		VkRect2D Scissor;
	};

	struct AttachmentData
	{
		u32 ColorAttachmentCount;
		VkFormat ColorAttachmentFormats[16]; // get max attachments from device
		VkFormat DepthAttachmentFormat;
		VkFormat StencilAttachmentFormat;
	};

	struct PipelineResourceInfo
	{
		AttachmentData PipelineAttachmentData;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct RenderPipeline
	{
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	struct VertexAttribute
	{
		VkFormat Format;
		u32 Offset;
	};

	struct VertexBinding
	{
		u32 Stride;
		VkVertexInputRate InputRate;
		std::unordered_map<std::string, VertexAttribute> Attributes;
	};

	Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface, const VkSurfaceFormatKHR* AvailableFormats, u32 Count);
	Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties();
	Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties();
	Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** RequiredInstanceExtensions, u32 RequiredExtensionsCount,
		const char** ValidationExtensions, u32 ValidationExtensionsCount);
	Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance);
	Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice);
	Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice);
	PhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties);
	VkExtent2D GetBestSwapExtent(VkPhysicalDevice PhysicalDevice, GLFWwindow* WindowHandler, VkSurfaceKHR Surface);
	VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);

	bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount);
	bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
	bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
		const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);
	bool CheckFormatSupport(VkPhysicalDevice PhysicalDevice, VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);
	bool CheckFormats(VkPhysicalDevice PhDevice);

	void PrintDeviceData(VkPhysicalDeviceProperties* DeviceProperties, VkPhysicalDeviceFeatures* AvailableFeatures);

	VkDeviceSize CalculateBufferAlignedSize(VkDevice Device, VkBuffer Buffer, u64 BufferSize);
	VkDeviceSize CalculateImageAlignedSize(VkDevice Device, VkImage Image, u64 ImageSize);

	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer, MemoryPropertyFlag Properties);
	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkImage Image, MemoryPropertyFlag Properties);

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag);

	void UpdateHostCompatibleBufferMemory(VkDevice Device, VkDeviceMemory Memory, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);

	bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger);
	bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator);
	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData);

	VkBool32 ParseBool(const char* Value, u32 Length);
	VkPolygonMode ParsePolygonMode(const char* Value, u32 Length);
	VkCullModeFlags ParseCullMode(const char* Value, u32 Length);
	VkFrontFace ParseFrontFace(const char* Value, u32 Length);
	VkColorComponentFlags ParseColorWriteMask(const char* Value, u32 Length);
	VkBlendFactor ParseBlendFactor(const char* Value, u32 Length);
	VkBlendOp ParseBlendOp(const char* Value, u32 Length);
	VkCompareOp ParseCompareOp(const char* Value, u32 Length);
	VkSampleCountFlagBits ParseSampleCount(const char* Value, u32 Length);
	VkPrimitiveTopology ParseTopology(const char* Value, u32 Length);
	VkShaderStageFlagBits ParseShaderStage(const char* Value, u32 Length);
	
	VkFilter ParseFilter(const char* Value, u32 Length);
	VkSamplerAddressMode ParseAddressMode(const char* Value, u32 Length);
	VkBorderColor ParseBorderColor(const char* Value, u32 Length);
	VkSamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length);
	VkDescriptorType ParseDescriptorType(const char* Value, u32 Length);
	VkShaderStageFlags ParseShaderStageFlags(const char* Value, u32 Length);
	
	VkFormat ParseFormat(const char* Value, u32 Length);
	VkVertexInputRate ParseVertexInputRate(const char* Value, u32 Length);
	u32 CalculateFormatSize(VkFormat Format);
	u32 CalculateFormatSizeFromString(const char* FormatString, u32 FormatLength);
}
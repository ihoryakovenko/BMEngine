#pragma once

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
	static const u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	static const u32 MAX_VERTEX_INPUT_BINDINGS = 16;

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
	};

	struct VertexInputAttribute
	{
		const char* VertexInputAttributeName = nullptr;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		u32 AttributeOffset = 0;
	};

	struct BMRVertexInputBinding
	{
		const char* VertexInputBindingName = nullptr;

		VertexInputAttribute InputAttributes[MAX_VERTEX_INPUTS_ATTRIBUTES];
		u32 InputAttributesCount = 0;

		u32 Stride = 0;
		VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;
	};

	struct PipelineSettings
	{
		VkExtent2D Extent;

		VkBool32 DepthClampEnable = VK_FALSE;
		VkBool32 RasterizerDiscardEnable = VK_FALSE;
		VkPolygonMode PolygonMode = VK_POLYGON_MODE_MAX_ENUM;
		f32 LineWidth = 0.0f;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_MAX_ENUM;
		VkBool32 DepthBiasEnable = VK_FALSE;

		VkBool32 LogicOpEnable = VK_FALSE;
		u32 AttachmentCount = 0;
		u32 ColorWriteMask = 0;
		VkBool32 BlendEnable = VK_FALSE;
		VkBlendFactor SrcColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp ColorBlendOp = VK_BLEND_OP_MAX_ENUM;
		VkBlendFactor SrcAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp AlphaBlendOp = VK_BLEND_OP_MAX_ENUM;

		VkBool32 DepthTestEnable = VK_FALSE;
		VkBool32 DepthWriteEnable = VK_FALSE;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_MAX_ENUM;
		VkBool32 DepthBoundsTestEnable = VK_FALSE;
		VkBool32 StencilTestEnable = VK_FALSE;
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
	VkPipeline BatchPipelineCreation(VkDevice Device, const Shader* Shaders, u32 ShadersCount,
		const BMRVertexInputBinding* VertexInputBinding, u32 VertexInputBindingCount,
		const PipelineSettings* Settings, const PipelineResourceInfo* ResourceInfo);


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
}
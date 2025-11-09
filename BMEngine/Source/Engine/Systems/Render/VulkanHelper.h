#pragma once

#include <unordered_map>
#include <string>

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

#include "RenderInterface.h"

struct GLFWwindow;

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

#define VULKAN_CHECK_RESULT(call) \
	{ \
		const VkResult result = (call); \
		if (result != VK_SUCCESS) { \
			Util::RenderLog(Util::LogType::Error, "%s returned %d at %s:%d", #call, result, __FILE__, __LINE__); \
		} \
	}

namespace VulkanHelper
{
	inline constexpr u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	inline constexpr u32 MAX_VERTEX_INPUT_BINDINGS = 16;
	inline constexpr u32 MAX_DRAW_FRAMES = 3;

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

	struct RenderPipeline
	{
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface, const VkSurfaceFormatKHR* AvailableFormats, u32 Count);

	void GetRequiredInstanceExtensions(const char** RequiredInstanceExtensions, u32 RequiredExtensionsCount,
		const char** ValidationExtensions, u32 ValidationExtensionsCount, const char** OutInstanceExtensions);

	PhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties);
	VkExtent2D GetBestSwapExtent(VkPhysicalDevice PhysicalDevice, GLFWwindow* WindowHandler, VkSurfaceKHR Surface);
	VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);

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

	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer, MemoryPropertyFlag Properties, VkBufferUsageFlags BufferUsageFlags, const VkAllocationCallbacks* Allocator);
	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkImage Image, MemoryPropertyFlag Properties, const VkAllocationCallbacks* Allocator);

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag, const VkAllocationCallbacks* Allocator);

	void UpdateHostCompatibleBufferMemory(VkDevice Device, VkDeviceMemory Memory, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);

	u32 GetFormatAlignment(VkFormat Format);
	u32 CalculateFormatSize(VkFormat Format);

	bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger);
	bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator);
	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData);

	void ApplyStageBarrier(VkBufferMemoryBarrier2* Barrier, BmRender_PipelineSyncStage Stage);

	VkShaderStageFlags DescriptorShaderStageToVkShaderStage(BmRender_DescriptorShaderStage stage);
	VkShaderStageFlagBits PipelineShaderStageToVkShaderStage(BmRender_PipelineShaderStage stage);
	VkPipelineStageFlags PipelineSyncToVkPipelineStage(BmRender_PipelineSyncStage stage);
	VkImageAspectFlags ImageTypeToVkImageAspectFlags(BmRender_ImageType Type);
}
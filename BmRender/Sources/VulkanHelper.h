#pragma once

#include <SharedLib.h>

#include <vulkan/vulkan.h>


#include "RenderInterface.h"

struct GLFWwindow;

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

#define VULKAN_CHECK_RESULT(call) \
	{ \
		const VkResult result = (call); \
		if (result != VK_SUCCESS) { \
			RenderLog(LogType::Error, "%s returned %d at %s:%d", #call, result, __FILE__, __LINE__); \
		} \
	}

inline constexpr u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
inline constexpr u32 MAX_VERTEX_INPUT_BINDINGS = 16;

enum class LogType
{
	Error,
	Warning,
	Info
};

void RenderLog(LogType logType, const char* format, ...);
void RenderLog(LogType LogType, const char* Format, va_list Args);

enum class BufferUsageFlag
{
	UniformFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	StagingFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	StorageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	VertexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	IndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	CombinedVertexIndexFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	InstanceFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	IndirectDrawBufferFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
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

DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer, MemoryPropertyFlag Properties, VkBufferUsageFlags BufferUsageFlags, const VkAllocationCallbacks* Allocator);
DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkImage Image, MemoryPropertyFlag Properties, const VkAllocationCallbacks* Allocator);

VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag, const VkAllocationCallbacks* Allocator);

void UpdateHostCompatibleBufferMemory(VkDevice Device, VkDeviceMemory Memory, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);

u32 CalculateFormatSize(VkFormat Format);

bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
	const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger);
bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
	const VkAllocationCallbacks* Allocator);
VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData);

VkShaderStageFlags DescriptorShaderStageToVkShaderStage(BmRender_DescriptorShaderStage stage);
VkShaderStageFlagBits PipelineShaderStageToVkShaderStage(BmRender_PipelineShaderStage stage);
VkPipelineStageFlags PipelineSyncToVkPipelineStage(BmRender_PipelineSyncStage stage);
VkPipelineBindPoint PipelineTypeToVkPipelineBindPoint(BmRender_PipelineType Type);
VkImageAspectFlags ImageTypeToVkImageAspectFlags(BmRender_ImageType Type);
VkDescriptorPoolCreateFlags DescriptorPoolTypeToVkFlags(BmRender_DescriptorPoolType Type);
VkMemoryPropertyFlags MemoryPropertyFlagToVkFlags(MemoryPropertyFlag Flag);
s32 GetQueueFamilyIndexFromQueueType(BmRender_QueueType QueueType, const PhysicalDeviceIndices& Indices);
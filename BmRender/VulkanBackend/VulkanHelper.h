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

u32 CalculateFormatSize(BmRender_Format Format);

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

VkFilter FilterToVkFilter(BmRender_Filter Filter);
VkSamplerMipmapMode SamplerMipmapModeToVk(BmRender_SamplerMipmapMode Mode);
VkSamplerAddressMode SamplerAddressModeToVk(BmRender_SamplerAddressMode Mode);
VkCompareOp CompareOpToVk(BmRender_CompareOp Op);
VkBorderColor BorderColorToVk(BmRender_BorderColor Color);
VkImageLayout ImageLayoutToVk(BmRender_ImageLayout Layout);
VkAttachmentLoadOp AttachmentLoadOpToVk(BmRender_AttachmentLoadOp Op);
VkAttachmentStoreOp AttachmentStoreOpToVk(BmRender_AttachmentStoreOp Op);
VkDescriptorType DescriptorTypeToVk(BmRender_DescriptorType Type);
BmRender_DescriptorType VkDescriptorTypeToBmRender(VkDescriptorType Type);
VkIndexType IndexTypeToVk(BmRender_IndexType Type);
BmRender_Format VkFormatToBmRender(VkFormat Format);
VkSurfaceFormatKHR SurfaceFormatToVk(BmRender_SurfaceFormat SurfaceFormat);
BmRender_SurfaceFormat VkSurfaceFormatToBmRender(VkSurfaceFormatKHR SurfaceFormat);

// 2D types conversions
VkOffset2D Offset2DToVk(BmRender_Offset2D Offset);
BmRender_Offset2D VkOffset2DToBmRender(VkOffset2D Offset);
VkExtent2D Extent2DToVk(BmRender_Dimensions Extent);
BmRender_Dimensions VkExtent2DToBmRender(VkExtent2D Extent);
VkViewport ViewportToVk(BmRender_Viewport Viewport);
BmRender_Viewport VkViewportToBmRender(VkViewport Viewport);
VkRect2D Rect2DToVk(BmRender_Rect2D Rect);
BmRender_Rect2D VkRect2DToBmRender(VkRect2D Rect);

// Clear value conversions
VkClearColorValue ClearColorValueToVk(BmRender_ClearColorValue ClearValue);
BmRender_ClearColorValue VkClearColorValueToBmRender(VkClearColorValue ClearValue);
VkClearDepthStencilValue ClearDepthStencilValueToVk(BmRender_ClearDepthStencilValue ClearValue);
BmRender_ClearDepthStencilValue VkClearDepthStencilValueToBmRender(VkClearDepthStencilValue ClearValue);

// Descriptor pool size conversion
VkDescriptorPoolSize DescriptorPoolSizeToVk(BmRender_DescriptorPoolSize PoolSize);
BmRender_DescriptorPoolSize VkDescriptorPoolSizeToBmRender(VkDescriptorPoolSize PoolSize);

// Shader stage flags conversion
VkShaderStageFlags ShaderStageFlagsToVk(BmRender_DescriptorShaderStage StageFlags);
BmRender_DescriptorShaderStage VkShaderStageFlagsToBmRender(VkShaderStageFlags StageFlags);
VkPipelineStageFlags PipelineStageFlagsToVk(BmRender_PipelineSyncStage StageFlags);
BmRender_PipelineSyncStage VkPipelineStageFlagsToBmRender(VkPipelineStageFlags StageFlags);

// Pipeline state conversion functions
VkPolygonMode PolygonModeToVk(BmRender_PolygonMode Mode);
VkCullModeFlags CullModeFlagsToVk(BmRender_CullModeFlags Flags);
VkFrontFace FrontFaceToVk(BmRender_FrontFace Face);
VkColorComponentFlags ColorComponentFlagsToVk(BmRender_ColorComponentFlags Flags);
VkBlendFactor BlendFactorToVk(BmRender_BlendFactor Factor);
VkBlendOp BlendOpToVk(BmRender_BlendOp Op);
VkPrimitiveTopology PrimitiveTopologyToVk(BmRender_PrimitiveTopology Topology);
VkSampleCountFlagBits SampleCountToVk(BmRender_SampleCount Count);

VkPipelineRasterizationStateCreateInfo RasterizationStateToVk(const BmRender_RasterizationState& State);
VkPipelineColorBlendAttachmentState ColorBlendAttachmentToVk(const BmRender_ColorBlendAttachment& Attachment);
VkPipelineColorBlendStateCreateInfo ColorBlendStateToVk(const BmRender_ColorBlendState& State);
VkPipelineDepthStencilStateCreateInfo DepthStencilStateToVk(const BmRender_DepthStencilState& State);
VkPipelineMultisampleStateCreateInfo MultisampleStateToVk(const BmRender_MultisampleState& State);
VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateToVk(const BmRender_InputAssemblyState& State);
VkPipelineViewportStateCreateInfo ViewportStateToVk(const BmRender_ViewportState& State);
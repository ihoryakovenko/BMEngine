#pragma once

#include <vulkan/vulkan.h>

#include <stb_image.h>

#include "BMRInterfaceTypes.h"
#include "Util/EngineTypes.h"
#include <Memory/MemoryManagmentSystem.h>

namespace BMR
{
	static const u32 MAX_IMAGES = 1024;
	static const u32 MB64 = 1024 * 1024 * 64;
	static const u32 MB128 = 1024 * 1024 * 128;
	static const u32 BUFFER_ALIGNMENT = 64;
	static const u32 IMAGE_ALIGNMENT = 4096;
	static const u32 MAX_DRAW_FRAMES = 3;
	static const u32 MAX_LIGHT_SOURCES = 2;
	static const u32 MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE = 8;
	static const u32 MAX_DESCRIPTOR_BINDING_PER_SET = 16;

	struct BMRPhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};
	
	struct BMRMainInstance
	{
		static BMRMainInstance CreateMainInstance(const char** RequiredExtensions, u32 RequiredExtensionsCount,
			bool IsValidationLayersEnabled, const char* ValidationLayers[], u32 ValidationLayersSize);
		static void DestroyMainInstance(BMRMainInstance& Instance);

		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct BMRSwapchainInstance
	{
		static BMRSwapchainInstance CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
			BMRPhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
			VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent);

		static void DestroySwapchainInstance(VkDevice LogicalDevice, BMRSwapchainInstance& Instance);

		static VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
			VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
			BMRPhysicalDeviceIndices DeviceIndices);
		static VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
			VkSurfaceKHR Surface);
		static Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);

		VkSwapchainKHR VulkanSwapchain = nullptr;
		u32 ImagesCount = 0;
		VkImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent = { };
	};

	struct BMRDeviceInstance
	{
		void Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
			u32 DeviceExtensionsSize);

		static Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance);
		static Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice);
		static Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice);
		static BMRPhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
			VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
			VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BMRPhysicalDeviceIndices Indices,
			VkPhysicalDeviceFeatures AvailableFeatures);
		static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
			const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);

		VkPhysicalDevice PhysicalDevice = nullptr;
		BMRPhysicalDeviceIndices Indices;
		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures AvailableFeatures;
	};

	struct BMRViewportInstance
	{
		VkSurfaceKHR Surface = nullptr;

		BMRSwapchainInstance ViewportSwapchain;

		VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	//struct BMRUniformSet
	//{
	//	VkDescriptorSetLayout Layout = nullptr;
	//	const BMRUniformBuffer* BMRUniformBuffers = nullptr;
	//	u32 BuffersCount = 0;
	//};

	//struct BMRUniformPushBufferDescription
	//{
	//	VkShaderStageFlags StageFlags = 0;
	//	u32 Offset = 0;
	//	u32 Size = 0;
	//};

	//struct BMRUniformInput
	//{
	//	const VkDescriptorSetLayout* UniformLayout = nullptr;
	//	u32 layoutCount = 0;

	//	VkPushConstantRange PushConstants;
	//};

	struct BMRPipelineShaderDescription
	{
		VkShaderStageFlagBits Stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		const char* EntryPoint = nullptr;
		u32* Code = nullptr;
		u32 CodeSize = 0;
	};

	struct BMRSPipelineShaderInfo
	{
		VkPipelineShaderStageCreateInfo Infos[BMRShaderStage::ShaderStagesCount];
		u32 InfosCounter = 0;
	};

	struct BMRPipelineResourceInfo
	{
		VkRenderPass RenderPass = nullptr;
		u32 SubpassIndex = -1;

		VkPipelineLayout PipelineLayout = nullptr;
	};
}

#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct QueueFamilyIndices
	{
		int _GraphicsFamily = -1;
		int _PresentationFamily = -1;

		bool IsValid() const
		{
			return _GraphicsFamily >= 0 && _PresentationFamily >= 0;
		}

		bool Init(VkPhysicalDevice Device, VkSurfaceKHR Surface);
	};

	struct SwapchainDetails
	{
		VkSurfaceCapabilitiesKHR _SurfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> _Formats;
		std::vector<VkPresentModeKHR> _PresentationModes;

		bool Init(VkPhysicalDevice Device, VkSurfaceKHR Surface);
	};

	struct VulkanUtil
	{
		static bool IsInstanceExtensionSupported(const char* Extension);
		static bool IsDeviceExtensionSupported(VkPhysicalDevice Device, const char* Extension);
		static bool IsDeviceSuitable(VkPhysicalDevice Device);
		static bool IsValidationLayerSupported(const char* Layer);
		static bool GetRequiredInstanceExtensions(std::vector<const char*>& InstanceExtensions);
		static void GetEnabledValidationLayers(VkInstanceCreateInfo& CreateInfo);
		
		static void SetupDebugMessenger(VkInstance Instance);
		static void DestroyDebugMessenger(VkInstance Instance);
		static void GetDebugCreateInfo(VkInstanceCreateInfo& CreateInfo);

	private:
		static VkDebugUtilsMessengerCreateInfoEXT* GetDebugMessengerCreateInfo();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData);

		static void CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
			const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger,
			const VkAllocationCallbacks* Allocator);

	public:
		// TODO validate
		static inline std::vector<const char*> _DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

	private:
		static inline std::vector<const char*> _ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		static inline VkDebugUtilsMessengerEXT _DebugMessenger;

#ifdef NDEBUG
		static inline bool _EnableValidationLayers = false;
#else
		static inline bool _EnableValidationLayers = true;
#endif
	};
}

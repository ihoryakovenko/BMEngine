#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct VulkanUtil
	{
		static bool IsValidationLayerSupported(const char* Layer);
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
#ifdef NDEBUG
		static inline bool _EnableValidationLayers = false;
#else
		static inline bool _EnableValidationLayers = true;
#endif

	private:
		static inline std::vector<const char*> _ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		static inline VkDebugUtilsMessengerEXT _DebugMessenger;
	};
}

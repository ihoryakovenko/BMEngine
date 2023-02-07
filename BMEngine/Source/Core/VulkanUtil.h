#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct QueueFamilyIndices
	{
		int _GraphicsFamily = -1;

		bool IsValid() const
		{
			return _GraphicsFamily >= 0;
		}

		static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice Device);
	};

	struct VulkanUtil
	{
		static bool IsInstanceExtensionSupported(const char* Extension);
		static bool IsDeviceSuitable(VkPhysicalDevice Device);
		static bool IsValidationLayerSupported(const char* Layer);
		static bool GetRequiredExtensions(std::vector<const char*>& InstanceExtensions);
		static void SetEnabledValidationLayers(VkInstanceCreateInfo& CreateInfo);
		
		static void SetupDebugMessenger(VkInstance Instance);
		static void DestroyDebugMessenger(VkInstance Instance);
		static void SetDebugCreateInfo(VkInstanceCreateInfo& CreateInfo);

	private:
		static VkDebugUtilsMessengerCreateInfoEXT* GetDebugCreateInfo();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData);

		static void CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
			const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger,
			const VkAllocationCallbacks* Allocator);

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

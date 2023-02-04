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
		static void GetEnabledValidationLayers(uint32_t& EnabledLayerCount, const char* const*& EnabledLayerNames);

		static inline std::vector<const char*> _ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef NDEBUG
		static inline bool _EnableValidationLayers = false;
#else
		static inline bool _EnableValidationLayers = true;
#endif
	};
}

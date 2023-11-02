#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	// Contains dynamic data for VkInstanceCreateInfo
	// Deinit if InitVkInstanceCreateInfoData returns true
	struct VkInstanceCreateInfoData
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = nullptr;

		const char** RequiredAndValidationExtensions = nullptr;
		uint32_t RequiredAndValidationExtensionsCount = 0;

		uint32_t AvalibleExtensionsCount = 0;
		VkExtensionProperties* AvalibleExtensions = nullptr;

		uint32_t AvalibleValidationLayersCount = 0;
		VkLayerProperties* AvalibleValidationLayers = nullptr;

		void* Buffer = nullptr;
	};

	bool InitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data,
		const char** ValidationExtensions, uint32_t ValidationExtensionsSize, bool EnumerateInstanceLayerProperties);

	void DeinitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data);

	bool CheckRequiredInstanceExtensionsSupport(VkInstanceCreateInfoData& Data);
	bool CheckValidationLayersSupport(VkInstanceCreateInfoData& Data, const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize);
}

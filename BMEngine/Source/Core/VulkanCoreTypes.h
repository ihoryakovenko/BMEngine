#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	struct VkInstanceCreateInfoData
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = nullptr;

		const char** RequiredAndValidationExtensions = nullptr;
		uint32_t RequiredAndValidationExtensionsCount = 0;

		uint32_t AvalibleExtensionsCount = 0;
		VkExtensionProperties* AvalibleExtensions = nullptr;

		void* Buffer = nullptr;
	};

	bool InitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data);
	void DeinitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data);

	bool CheckRequiredInstanceExtensionsSupport(VkInstanceCreateInfoData& Data);
}

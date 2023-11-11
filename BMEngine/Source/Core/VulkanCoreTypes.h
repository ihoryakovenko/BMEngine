#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Core
{
	// Deinit if InitVkInstanceCreateInfoSetupData returns true
	struct VkInstanceCreateInfoSetupData
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = nullptr;

		const char** RequiredAndValidationExtensions = nullptr;
		uint32_t RequiredAndValidationExtensionsCount = 0;

		uint32_t AvalibleExtensionsCount = 0;
		VkExtensionProperties* AvalibleExtensions = nullptr;

		uint32_t AvalibleValidationLayersCount = 0;
		VkLayerProperties* AvalibleValidationLayers = nullptr;
	};

	bool InitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data,
		const char** ValidationExtensions, uint32_t ValidationExtensionsSize, bool EnumerateInstanceLayerProperties);

	void DeinitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data);

	bool CheckRequiredInstanceExtensionsSupport(const VkInstanceCreateInfoSetupData& Data);
	bool CheckValidationLayersSupport(const VkInstanceCreateInfoSetupData& Data, const char** ValidationLeyersToCheck,
		uint32_t ValidationLeyersToCheckSize);

	// Deinit if InitVkPhysicalDeviceSetupData returns true
	struct VkPhysicalDeviceSetupData
	{
		uint32_t AvalibleDeviceExtensionsCount = 0;
		VkExtensionProperties* AvalibleDeviceExtensions = nullptr;

		uint32_t FamilyPropertiesCount = 0;
		VkQueueFamilyProperties* FamilyProperties = nullptr;

		uint32_t SurfaceFormatsCount = 0;
		VkSurfaceFormatKHR* SurfaceFormats = nullptr;

		uint32_t PresentModesCount = 0;
		VkPresentModeKHR* PresentModes = nullptr;
	};

	bool InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	void DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data);

	bool CheckDeviceExtensionsSupport(const VkPhysicalDeviceSetupData& Data, const char** ExtensionsToCheck,
		uint32_t ExtensionsToCheckSize);

	struct PhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};

	PhysicalDeviceIndices GetPhysicalDeviceIndices(const VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
	};
}

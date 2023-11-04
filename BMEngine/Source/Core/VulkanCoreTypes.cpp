#include "VulkanCoreTypes.h"

#include "Util/Util.h"

namespace Core
{
	bool InitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data, const char** ValidationExtensions,
		uint32_t ValidationExtensionsCount, bool EnumerateInstanceLayerProperties)
	{
		// Get count part
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&Data.RequiredExtensionsCount);
		if (Data.RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		if (EnumerateInstanceLayerProperties)
		{
			Result = vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, nullptr);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
				return false;
			}
		}

		// Allocation part
		Data.RequiredAndValidationExtensions = static_cast<const char**>(Util::Memory::Allocate((ValidationExtensionsCount + Data.RequiredExtensionsCount) * sizeof(const char**)));
		Data.AvalibleExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleExtensionsCount * sizeof(VkExtensionProperties)));
		Data.AvalibleValidationLayers = static_cast<VkLayerProperties*>(Util::Memory::Allocate(Data.AvalibleValidationLayersCount * sizeof(VkLayerProperties)));

		// Data structuring part
		Data.RequiredAndValidationExtensionsCount = Data.RequiredExtensionsCount + ValidationExtensionsCount;
		Data.RequiredInstanceExtensions = Data.RequiredAndValidationExtensions + ValidationExtensionsCount;

		// Data assignment part
		for (uint32_t i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data.RequiredAndValidationExtensions[i] = ValidationExtensions[i];
		}

		for (uint32_t i = 0; i < Data.RequiredExtensionsCount; ++i)
		{
			Data.RequiredInstanceExtensions[i] = RequiredInstanceExtensions[i];
		}

		vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, Data.AvalibleExtensions);
		vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, Data.AvalibleValidationLayers);

		return true;
	}

	void DeinitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data)
	{
		Util::Memory::Deallocate(Data.RequiredAndValidationExtensions);
		Util::Memory::Deallocate(Data.AvalibleExtensions);
		Util::Memory::Deallocate(Data.AvalibleValidationLayers);
	}

	bool CheckRequiredInstanceExtensionsSupport(const VkInstanceCreateInfoSetupData& Data)
	{
		for (uint32_t i = 0; i < Data.RequiredAndValidationExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleExtensionsCount; ++j)
			{
				if (std::strcmp(Data.RequiredAndValidationExtensions[i], Data.AvalibleExtensions[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", Data.RequiredAndValidationExtensions[i]);
				return false;
			}
		}

		return true;
	}

	bool CheckValidationLayersSupport(const VkInstanceCreateInfoSetupData& Data, const char** ValidationLeyersToCheck,
		uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < Data.AvalibleValidationLayersCount; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Data.AvalibleValidationLayers[j].layerName) == 0)
				{
					IsLayerAvalible = true;
					break;
				}
			}

			if (!IsLayerAvalible)
			{
				Util::Log().Error("Validation layer {} unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}

	bool InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		// Get count part
		VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, nullptr);
		if (Data.AvalibleDeviceExtensionsCount == 0)
		{
			Util::Log().Error("vkEnumerateDeviceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, nullptr);

		Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, nullptr);
		if (Data.SurfaceFormatsCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfaceFormatsKHR result is {}", static_cast<int>(Result));
			return false;
		}

		Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, nullptr);
		if (Data.PresentModesCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			return false;
		}

		// Allocation part
		Data.AvalibleDeviceExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleDeviceExtensionsCount * sizeof(VkExtensionProperties)));
		Data.FamilyProperties = static_cast<VkQueueFamilyProperties*>(Util::Memory::Allocate(Data.FamilyPropertiesCount * sizeof(VkQueueFamilyProperties)));
		Data.SurfaceFormats = static_cast<VkSurfaceFormatKHR*>(Util::Memory::Allocate(Data.SurfaceFormatsCount * sizeof(VkSurfaceFormatKHR)));
		Data.PresentModes = static_cast<VkPresentModeKHR*>(Util::Memory::Allocate(Data.PresentModesCount * sizeof(VkPresentModeKHR)));

		// Data assignment part
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, Data.AvalibleDeviceExtensions);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, Data.FamilyProperties);
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, Data.SurfaceFormats);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, Data.PresentModes);

		return true;
	}

	void DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data)
	{
		Util::Memory::Deallocate(Data.AvalibleDeviceExtensions);
		Util::Memory::Deallocate(Data.FamilyProperties);
		Util::Memory::Deallocate(Data.SurfaceFormats);
		Util::Memory::Deallocate(Data.PresentModes);
	}

	bool CheckDeviceExtensionsSupport(const VkPhysicalDeviceSetupData& Data, const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize)
	{
		for (uint32_t i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleDeviceExtensionsCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], Data.AvalibleDeviceExtensions[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				Util::Log().Error("Device extension {} unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices GetPhysicalDeviceIndices(const VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (uint32_t i = 0; i < Data.FamilyPropertiesCount; ++i)
		{
			// check if Queue is graphics type
			if (Data.FamilyProperties[i].queueCount > 0 && Data.FamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilys
				Indices.GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentationSupport);
			if (Data.FamilyProperties[i].queueCount > 0 && PresentationSupport)
			{
				Indices.PresentationFamily = i;
			}

			if (Indices.GraphicsFamily >= 0 && Indices.PresentationFamily >= 0)
			{
				break;
			}
		}

		return Indices;
	}
}

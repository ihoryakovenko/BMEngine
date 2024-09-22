#include "VulkanCoreTypes.h"

#include "Util/Util.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace Core
{
	bool Core::CheckRequiredInstanceExtensionsSupport(ExtensionPropertiesData AvailableExtensions, RequiredInstanceExtensionsData RequiredExtensions)
	{
		for (uint32_t i = 0; i < RequiredExtensions.Count; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < AvailableExtensions.Count; ++j)
			{
				if (std::strcmp(RequiredExtensions.Extensions[i], AvailableExtensions.ExtensionProperties[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", RequiredExtensions.Extensions[i]);
				return false;
			}
		}

		return true;
	}

	bool Core::CheckValidationLayersSupport(AvailableInstanceLayerPropertiesData& Data, const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < Data.Count; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Data.Properties[j].layerName) == 0)
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

	bool CheckDeviceExtensionsSupport(ExtensionPropertiesData Data, const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize)
	{
		for (uint32_t i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < Data.Count; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], Data.ExtensionProperties[j].extensionName) == 0)
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

	MainInstance CreateMainInstance(RequiredInstanceExtensionsData RequiredExtensions,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize)
	{
		VkApplicationInfo ApplicationInfo = { };
		ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ApplicationInfo.pApplicationName = "Blank App";
		ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.pEngineName = "BMEngine";
		ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };
		CreateInfo.enabledExtensionCount = RequiredExtensions.Count;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Extensions;

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
			MessengerCreateInfo.pUserData = nullptr;

			CreateInfo.pNext = &MessengerCreateInfo;
			CreateInfo.enabledLayerCount = ValidationLayersSize;
			CreateInfo.ppEnabledLayerNames = ValidationLayers;
		}
		else
		{
			CreateInfo.ppEnabledLayerNames = nullptr;
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}

		MainInstance Instance;
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			assert(false);
		}

		if (IsValidationLayersEnabled)
		{
			const bool Result = Util::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
			assert(Result);
		}

		return Instance;
	}

	void DestroyMainInstance(MainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			Util::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices GetPhysicalDeviceIndices(QueueFamilyPropertiesData Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (uint32_t i = 0; i < Data.Count; ++i)
		{
			// check if Queue is graphics type
			if (Data.Properties[i].queueCount > 0 && Data.Properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilies
				Indices.GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentationSupport);
			if (Data.Properties[i].queueCount > 0 && PresentationSupport)
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

	VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window)
	{
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(Window, &Width, &Height);

			Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
	}

	RequiredInstanceExtensionsData CreateRequiredInstanceExtensionsData(const char** ValidationExtensions,
		uint32_t ValidationExtensionsCount)
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&RequiredExtensionsCount);
		if (RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}

		RequiredInstanceExtensionsData Data;
		Data.ValidationExtensionsCount = ValidationExtensionsCount;
		Data.Count = RequiredExtensionsCount + Data.ValidationExtensionsCount;

		if (Data.ValidationExtensionsCount == 0)
		{
			Data.Extensions = RequiredInstanceExtensions;
			return Data;
		}

		Data.Extensions = static_cast<const char**>(Util::Memory::Allocate(Data.Count * sizeof(const char**)));

		for (uint32_t i = 0; i < RequiredExtensionsCount; ++i)
		{
			Data.Extensions[i] = RequiredInstanceExtensions[i];
			Util::Log().Info("Requested {} extension", Data.Extensions[i]);
		}

		for (uint32_t i = 0; i < Data.ValidationExtensionsCount; ++i)
		{
			Data.Extensions[i + RequiredExtensionsCount] = ValidationExtensions[i];
			Util::Log().Info("Requested {} extension", Data.Extensions[i]);
		}

		return Data;
	}

	void DestroyRequiredInstanceExtensionsData(RequiredInstanceExtensionsData& Data)
	{
		if (Data.ValidationExtensionsCount > 0)
		{
			Util::Memory::Deallocate(Data.Extensions);
			Data.Extensions = nullptr;
		}
	}

	ExtensionPropertiesData CreateAvailableExtensionPropertiesData()
	{
		ExtensionPropertiesData Data;

		const VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Data.Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		Data.ExtensionProperties = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.Count * sizeof(VkExtensionProperties)));
		vkEnumerateInstanceExtensionProperties(nullptr, &Data.Count, Data.ExtensionProperties);

		return Data;
	}

	AvailableInstanceLayerPropertiesData CreateAvailableInstanceLayerPropertiesData()
	{
		AvailableInstanceLayerPropertiesData Data;

		const VkResult Result = vkEnumerateInstanceLayerProperties(&Data.Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		Data.Properties = static_cast<VkLayerProperties*>(Util::Memory::Allocate(Data.Count * sizeof(VkLayerProperties)));
		vkEnumerateInstanceLayerProperties(&Data.Count, Data.Properties);

		return Data;
	}

	void DestroyAvailableInstanceLayerPropertiesData(AvailableInstanceLayerPropertiesData& Data)
	{
		Util::Memory::Deallocate(Data.Properties);
		Data.Properties = nullptr;
	}

}

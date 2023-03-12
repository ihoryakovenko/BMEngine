#include <vector>

#include "VulkanUtil.h"

#include "Util/Log.h"

namespace Core
{
	// TODO check function
	bool QueueFamilyIndices::Init(VkPhysicalDevice Device, VkSurfaceKHR Surface)
	{
		uint32_t FamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(Device, &FamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> FamilyProperties(FamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(Device, &FamilyCount, FamilyProperties.data());

		int i = 0;
		for (const auto& QueueFamily : FamilyProperties)
		{
			// check if Queue is graphics type
			if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite _GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilys
				_GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &PresentationSupport);
			if (QueueFamily.queueCount > 0 && PresentationSupport)
			{
				_PresentationFamily = i;
			}

			if (IsValid())
			{
				return true;
			}

			++i;
		}

		Util::Log().Warning("Device does not support required indices");

		_GraphicsFamily = -1;
		_PresentationFamily = -i;
		return false;
	}

	bool VulkanUtil::IsInstanceExtensionSupported(const char* Extension)
	{
		static const std::vector<VkExtensionProperties>& AvalibleExtensions = []()
		{
			uint32_t ExtensionsCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, nullptr);

			std::vector<VkExtensionProperties> Extensions(ExtensionsCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, Extensions.data());

			return Extensions;
		}();

		for (const auto& AvalibleExtension : AvalibleExtensions)
		{
			if (std::strcmp(Extension, AvalibleExtension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool VulkanUtil::IsDeviceExtensionSupported(VkPhysicalDevice Device, const char* Extension)
	{
		uint32_t ExtensionsCount = 0;
		vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionsCount, nullptr);

		std::vector<VkExtensionProperties> AvalibleDeviceExtensions(ExtensionsCount);
		vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionsCount, AvalibleDeviceExtensions.data());

		for (const auto& AvalibleExtension : AvalibleDeviceExtensions)
		{
			if (std::strcmp(Extension, AvalibleExtension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool VulkanUtil::IsDeviceSuitable(VkPhysicalDevice Device)
	{
		/*
		// ID, name, type, vendor, etc
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);

		// geo shader, tess shader, wide lines, etc
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device, &Features);
		*/

		for (const char* Extension : _DeviceExtensions)
		{
			if (!IsDeviceExtensionSupported(Device, Extension))
			{
				Util::Log().Error("Device extension {} unsupported", Extension);
				return false;
			}
		}

		return true;
	}

	bool VulkanUtil::IsValidationLayerSupported(const char* Layer)
	{
		static const std::vector<VkLayerProperties>& AvailableLayers = []()
		{
			uint32_t LayerCount = 0;
			vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

			std::vector<VkLayerProperties> Layers(LayerCount);
			vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());

			return Layers;
		}();

		for (const auto& AvalibleLayer : AvailableLayers)
		{
			if (std::strcmp(Layer, AvalibleLayer.layerName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool VulkanUtil::GetRequiredInstanceExtensions(std::vector<const char*>& InstanceExtensions)
	{
		uint32_t ExtensionsCount = 0;
		const char** Extensions = glfwGetRequiredInstanceExtensions(&ExtensionsCount);
		if (Extensions == nullptr)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		for (uint32_t i = 0; i < ExtensionsCount; ++i)
		{
			const char* Extension = Extensions[i];
			if (!IsInstanceExtensionSupported(Extension))
			{
				Util::Log().Error("Extension {} unsupported", Extension);
				return false;
			}

			InstanceExtensions.push_back(Extension);
		}

		if (_EnableValidationLayers)
		{
			InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return true;
	}

	void VulkanUtil::GetEnabledValidationLayers(VkInstanceCreateInfo& CreateInfo)
	{
		if (_EnableValidationLayers)
		{
			std::erase_if(_ValidationLayers, [](const char* Layer)
			{
				if (!VulkanUtil::IsValidationLayerSupported(Layer))
				{
					Util::Log().Error("Validation layer {} unsupported", Layer);
					return true;
				}
				
				return false;
			});


			CreateInfo.enabledLayerCount = static_cast<uint32_t>(_ValidationLayers.size());
			CreateInfo.ppEnabledLayerNames = VulkanUtil::_ValidationLayers.data();
			return;
		}

		CreateInfo.ppEnabledLayerNames = nullptr;
		CreateInfo.enabledLayerCount = 0;
	}

	VkDebugUtilsMessengerCreateInfoEXT* VulkanUtil::GetDebugMessengerCreateInfo()
	{
		static VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = []()
		{
			VkDebugUtilsMessengerCreateInfoEXT CreateInfo;
			CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			CreateInfo.pfnUserCallback = DebugCallback;
			CreateInfo.pUserData = nullptr;

			return CreateInfo;
		}();

		return &DebugCreateInfo;
	}

	void VulkanUtil::SetupDebugMessenger(VkInstance Instance)
	{
		if (_EnableValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT* DebugCreateInfo = GetDebugMessengerCreateInfo();
			CreateDebugUtilsMessengerEXT(Instance, DebugCreateInfo, nullptr, &_DebugMessenger);
		}
	}

	void VulkanUtil::DestroyDebugMessenger(VkInstance Instance)
	{
		if (_EnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(Instance, _DebugMessenger, nullptr);
		}
	}

	void VulkanUtil::GetDebugCreateInfo(VkInstanceCreateInfo& CreateInfo)
	{
		if (_EnableValidationLayers)
		{
			CreateInfo.pNext = GetDebugMessengerCreateInfo();
		}
		else
		{
			CreateInfo.pNext = nullptr;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanUtil::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		[[maybe_unused]] void* UserData)
	{
		auto&& Log = Util::Log();
		if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			Log.Error("Validation layer: {}", CallbackData->pMessage);
		}
		else
		{
			Log.Info("Validation layer: {}", CallbackData->pMessage);
		}

		return VK_FALSE;
	}

	void VulkanUtil::CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger)
	{
		auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		if (CreateMessengerFunc != nullptr)
		{
			const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, DebugMessenger);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("CreateMessengerFunc result is {}", static_cast<int>(Result));
			}
		}
		else
		{
			Util::Log().Error("CreateMessengerFunc is nullptr");
		}
	}

	void VulkanUtil::DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger,
		const VkAllocationCallbacks* Allocator)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, DebugMessenger, Allocator);
		}
		else
		{
			Util::Log().Error("DestroyMessengerFunc is nullptr");
		}
	}

	bool SwapchainDetails::Init(VkPhysicalDevice Device, VkSurfaceKHR Surface)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &_SurfaceCapabilities);

		uint32_t FormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);
		if (FormatCount == 0)
		{
			Util::Log().Error("FormatCount is 0");
			return false;
		}

		_Formats.resize(static_cast<size_t>(FormatCount));
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, _Formats.data());

		uint32_t PresentationCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentationCount, nullptr);
		if (PresentationCount == 0)
		{
			Util::Log().Error("PresentationCount is 0");
			return false;
		}

		_PresentationModes.resize(static_cast<size_t>(PresentationCount));
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentationCount, _PresentationModes.data());

		return true;
	}
}

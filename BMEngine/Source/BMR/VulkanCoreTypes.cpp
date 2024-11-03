#include "VulkanCoreTypes.h"

#include "VulkanHelper.h"

namespace BMR
{
	static bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger);

	static bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData);

	BMRMainInstance BMRMainInstance::CreateMainInstance(const char** RequiredExtensions, u32 RequiredExtensionsCount,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], u32 ValidationLayersSize)
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
		CreateInfo.enabledExtensionCount = RequiredExtensionsCount;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions;

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = MessengerDebugCallback;
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

		BMRMainInstance Instance;
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateInstance result is %d", Result);
		}

		if (IsValidationLayersEnabled)
		{
			const bool CreateDebugUtilsMessengerResult =
				CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
			assert(CreateDebugUtilsMessengerResult);
		}

		return Instance;
	}

	void BMRMainInstance::DestroyMainInstance(BMRMainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	void BMRDeviceInstance::Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
		u32 DeviceExtensionsSize)
	{
		Memory::FrameArray<VkPhysicalDevice> DeviceList = GetPhysicalDeviceList(VulkanInstance);
		
		bool IsDeviceFound = false;
		for (u32 i = 0; i < DeviceList.Count; ++i)
		{
			PhysicalDevice = DeviceList[i];

			Memory::FrameArray<VkExtensionProperties> DeviceExtensionsData = GetDeviceExtensionProperties(PhysicalDevice);
			Memory::FrameArray<VkQueueFamilyProperties> FamilyPropertiesData = GetQueueFamilyProperties(PhysicalDevice);

			Indices = GetPhysicalDeviceIndices(FamilyPropertiesData.Pointer.Data, FamilyPropertiesData.Count, PhysicalDevice, Surface);
			vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
			vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailableFeatures);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.Pointer.Data, DeviceExtensionsData.Count, Indices, AvailableFeatures);

			if (IsDeviceFound)
			{
				IsDeviceFound = true;
				break;
			}
		}

		assert(IsDeviceFound);
	}

	Memory::FrameArray<VkPhysicalDevice> BMRDeviceInstance::GetPhysicalDeviceList(VkInstance VulkanInstance)
	{
		u32 Count;
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, nullptr);

		auto Data = Memory::FrameArray<VkPhysicalDevice>::Create(Count);
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkExtensionProperties> BMRDeviceInstance::GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		const VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkEnumerateDeviceExtensionProperties result is %d", Result);
		}

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkQueueFamilyProperties> BMRDeviceInstance::GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, nullptr);

		auto Data = Memory::FrameArray<VkQueueFamilyProperties>::Create(Count);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, Data.Pointer.Data);

		return Data;
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	BMRPhysicalDeviceIndices BMRDeviceInstance::GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		BMRPhysicalDeviceIndices Indices;

		for (u32 i = 0; i < PropertiesCount; ++i)
		{
			// check if Queue is graphics type
			if (Properties[i].queueCount > 0 && Properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
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
			if (Properties[i].queueCount > 0 && PresentationSupport)
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

	bool BMRDeviceInstance::CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BMRPhysicalDeviceIndices Indices,
		VkPhysicalDeviceFeatures AvailableFeatures)
	{
		if (!CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			HandleLog(BMRLogType::LogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			HandleLog(BMRLogType::LogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			HandleLog(BMRLogType::LogType_Warning, "Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			HandleLog(BMRLogType::LogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		return true;
	}

	bool BMRDeviceInstance::CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
		const char** ExtensionsToCheck, u32 ExtensionsToCheckSize)
	{
		for (u32 i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (u32 j = 0; j < ExtensionPropertiesCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], ExtensionProperties[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				HandleLog(BMRLogType::LogType_Error, "extension %s unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	BMRSwapchainInstance BMRSwapchainInstance::CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		BMRPhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent)
	{
		BMRSwapchainInstance Instance;

		Instance.SwapExtent = Extent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Warning, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is %d", Result);
			assert(false);
		}

		VkPresentModeKHR PresentationMode = GetBestPresentationMode(PhysicalDevice, Surface);

		Instance.VulkanSwapchain = CreateSwapchain(LogicalDevice, SurfaceCapabilities, Surface, SurfaceFormat, Instance.SwapExtent,
			PresentationMode, Indices);

		Memory::FrameArray<VkImage> Images = GetSwapchainImages(LogicalDevice, Instance.VulkanSwapchain);

		Instance.ImagesCount = Images.Count;

		for (u32 i = 0; i < Instance.ImagesCount; ++i)
		{
			VkImageView ImageView = CreateImageView(LogicalDevice, Images[i],
				SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
			if (ImageView == nullptr)
			{
				assert(false);
			}

			Instance.ImageViews[i] = ImageView;
		}

		return Instance;
	}

	void BMRSwapchainInstance::DestroySwapchainInstance(VkDevice LogicalDevice, BMRSwapchainInstance& Instance)
	{
		for (u32 i = 0; i < Instance.ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, Instance.ImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(LogicalDevice, Instance.VulkanSwapchain, nullptr);
	}

	VkSwapchainKHR BMRSwapchainInstance::CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		BMRPhysicalDeviceIndices DeviceIndices)
	{
		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		u32 ImageCount = SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (SurfaceCapabilities.maxImageCount > 0
			&& SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = SurfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = { };
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = Surface;
		SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		u32 Indices[] = {
			static_cast<u32>(DeviceIndices.GraphicsFamily),
			static_cast<u32>(DeviceIndices.PresentationFamily)
		};

		if (DeviceIndices.GraphicsFamily != DeviceIndices.PresentationFamily)
		{
			// Less efficient mode
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapchainCreateInfo.queueFamilyIndexCount = 2;
			SwapchainCreateInfo.pQueueFamilyIndices = Indices;
		}
		else
		{
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapchainCreateInfo.queueFamilyIndexCount = 0;
			SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		// Used if old cwap chain been destroyed and this one replaces it
		SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VkSwapchainKHR Swapchain;
		VkResult Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &Swapchain);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateSwapchainKHR result is %d", Result);
			assert(false);
		}

		return Swapchain;
	}

	VkPresentModeKHR BMRSwapchainInstance::GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		Memory::FrameArray<VkPresentModeKHR> PresentModes = GetAvailablePresentModes(PhysicalDevice, Surface);

		VkPresentModeKHR Mode = VK_PRESENT_MODE_FIFO_KHR;
		for (u32 i = 0; i < PresentModes.Count; ++i)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				Mode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		// Has to be present by spec
		if (Mode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			HandleLog(BMRLogType::LogType_Warning, "Using default VK_PRESENT_MODE_FIFO_KHR");
		}

		return Mode;
	}

	Memory::FrameArray<VkPresentModeKHR> BMRSwapchainInstance::GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkGetPhysicalDeviceSurfacePresentModesKHR result is %d", Result);
			assert(false);
		}

		auto Data = Memory::FrameArray<VkPresentModeKHR>::Create(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkImage> BMRSwapchainInstance::GetSwapchainImages(VkDevice LogicalDevice,
		VkSwapchainKHR VulkanSwapchain)
	{
		u32 Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		auto Data = Memory::FrameArray<VkImage>::Create(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Data.Pointer.Data);

		return Data;
	}

	bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger)
	{
		auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		if (CreateMessengerFunc != nullptr)
		{
			const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, InDebugMessenger);
			if (Result != VK_SUCCESS)
			{
				HandleLog(BMRLogType::LogType_Error, "CreateMessengerFunc result is %d", Result);
				return false;
			}
		}
		else
		{
			HandleLog(BMRLogType::LogType_Error, "CreateMessengerFunc is nullptr");
			return false;
		}

		return true;
	}

	bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, InDebugMessenger, Allocator);
			return true;
		}
		else
		{
			HandleLog(BMRLogType::LogType_Error, "DestroyMessengerFunc is nullptr");
			return false;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		[[maybe_unused]] void* UserData)
	{
		if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			HandleLog(BMRLogType::LogType_Error, CallbackData->pMessage);
		}
		else
		{
			HandleLog(BMRLogType::LogType_Info, CallbackData->pMessage);
		}

		return VK_FALSE;
	}
}
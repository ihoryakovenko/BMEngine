#include "VulkanCoreContext.h"

#include "Util/Util.h"

#include <GLFW/glfw3.h>

namespace VulkanCoreContext
{
	static VkDevice CreateLogicalDevice(VkPhysicalDevice PhDevice, VulkanHelper::PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize)
	{
		const f32 Priority = 1.0f;

		// One family can support graphics and presentation
		// In that case create multiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = { };
		u32 FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<u32>(Indices.GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (Indices.GraphicsFamily != Indices.PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<u32>(Indices.PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		// TODO: Check if supported
		VkPhysicalDeviceFeatures2 DeviceFeatures2 = { };
		DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		DeviceFeatures2.features.fillModeNonSolid = VK_TRUE; // Todo: get from configs
		DeviceFeatures2.features.samplerAnisotropy = VK_TRUE; // Todo: get from configs
		DeviceFeatures2.features.multiViewport = VK_TRUE; // Todo: get from configs

		// TODO: Check if supported
		VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures = { };
		DynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		DynamicRenderingFeatures.dynamicRendering = VK_TRUE;

		// TODO: Check if supported
		VkPhysicalDeviceSynchronization2Features Sync2Features = { };
		Sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		Sync2Features.synchronization2 = VK_TRUE;

		// TODO: Check if supported
		VkPhysicalDeviceDescriptorIndexingFeatures IndexingFeatures = { };
		IndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		IndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		IndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		IndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		IndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

		VkPhysicalDeviceTimelineSemaphoreFeatures TimelineSemaphoreFeatures = { };
		TimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		TimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

		VkPhysicalDeviceFeatures DeviceFeatures = { };
		DeviceFeatures.samplerAnisotropy = VK_TRUE;

		DeviceFeatures2.pNext = &TimelineSemaphoreFeatures;
		TimelineSemaphoreFeatures.pNext = &IndexingFeatures;
		IndexingFeatures.pNext = &Sync2Features;
		Sync2Features.pNext = &DynamicRenderingFeatures;


		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = FamilyIndicesSize;
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = nullptr;
		DeviceCreateInfo.pNext = &DeviceFeatures2;

		VkDevice LogicalDevice;
		// Queues are created at the same time as the Device
		VULKAN_CHECK_RESULT(vkCreateDevice(PhDevice, &DeviceCreateInfo, nullptr, &LogicalDevice));
		return LogicalDevice;
	}

	static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, VulkanHelper::PhysicalDeviceIndices Indices,
		VkPhysicalDevice Device, VkPhysicalDeviceProperties* DeviceProperties)
	{
		VkPhysicalDeviceFeatures AvailableFeatures;
		vkGetPhysicalDeviceFeatures(Device, &AvailableFeatures);
		
		VulkanHelper::PrintDeviceData(DeviceProperties, &AvailableFeatures);

		if (!VulkanHelper::CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		return true;
	}

	void CreateCoreContext(VulkanCoreContext* Context, GLFWwindow* Window)
	{
		Context->WindowHandler = Window;

		const u32 RequiredExtensionsCount = 2;
		const char* RequiredInstanceExtensions[RequiredExtensionsCount] =
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};

		const char* ValidationExtensions[] =
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		const u32 ValidationExtensionsSize = sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]);

		Memory::FrameArray<VkExtensionProperties> AvailableExtensions = VulkanHelper::GetAvailableExtensionProperties();
		Memory::FrameArray<const char*> RequiredExtensions = VulkanHelper::GetRequiredInstanceExtensions(RequiredInstanceExtensions, RequiredExtensionsCount,
			ValidationExtensions, ValidationExtensionsSize);

		if (!VulkanHelper::CheckRequiredInstanceExtensionsSupport(AvailableExtensions.Pointer.Data, AvailableExtensions.Count,
			RequiredExtensions.Pointer.Data, RequiredExtensions.Count))
		{
			assert(false);
		}

		VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };
		MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		MessengerCreateInfo.pfnUserCallback = VulkanHelper::MessengerDebugCallback;

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
		CreateInfo.enabledExtensionCount = RequiredExtensions.Count;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Pointer.Data;

		VULKAN_CHECK_RESULT(vkCreateInstance(&CreateInfo, nullptr, &Context->VulkanInstance));

		if (!VulkanHelper::CreateDebugUtilsMessengerEXT(Context->VulkanInstance, &MessengerCreateInfo, nullptr, &Context->DebugMessenger))
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "Cannot create debug messenger");
		}

		VULKAN_CHECK_RESULT(glfwCreateWindowSurface(Context->VulkanInstance, Context->WindowHandler, nullptr, &Context->Surface));

		const char* DeviceExtensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		};

		const u32 DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

		Memory::FrameArray<VkPhysicalDevice> DeviceList = VulkanHelper::GetPhysicalDeviceList(Context->VulkanInstance);

		bool IsDeviceFound = false;
		for (u32 i = 0; i < DeviceList.Count; ++i)
		{
			Context->PhysicalDevice = DeviceList[i];

			Memory::FrameArray<VkExtensionProperties> DeviceExtensionsData = VulkanHelper::GetDeviceExtensionProperties(Context->PhysicalDevice);
			Memory::FrameArray<VkQueueFamilyProperties> FamilyPropertiesData = VulkanHelper::GetQueueFamilyProperties(Context->PhysicalDevice);

			Context->Indices = VulkanHelper::GetPhysicalDeviceIndices(FamilyPropertiesData.Pointer.Data, FamilyPropertiesData.Count, Context->PhysicalDevice, Context->Surface);

			VkPhysicalDeviceProperties DeviceProperties;
			vkGetPhysicalDeviceProperties(Context->PhysicalDevice, &DeviceProperties);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.Pointer.Data, DeviceExtensionsData.Count, Context->Indices, Context->PhysicalDevice, &DeviceProperties);

			if (IsDeviceFound)
			{
				IsDeviceFound = true;
				break;
			}
		}

		if (!IsDeviceFound)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "Cannot find suitable device");
		}

		Context->LogicalDevice = CreateLogicalDevice(Context->PhysicalDevice, Context->Indices, DeviceExtensions, DeviceExtensionsSize);

		Memory::FrameArray<VkSurfaceFormatKHR> AvailableFormats = VulkanHelper::GetSurfaceFormats(Context->PhysicalDevice, Context->Surface);
		Context->SurfaceFormat = VulkanHelper::GetBestSurfaceFormat(Context->Surface, AvailableFormats.Pointer.Data, AvailableFormats.Count);

		VulkanHelper::CheckFormats(Context->PhysicalDevice);
		Context->SwapExtent = VulkanHelper::GetBestSwapExtent(Context->PhysicalDevice, Context->WindowHandler, Context->Surface);

		vkGetDeviceQueue(Context->LogicalDevice, static_cast<u32>(Context->Indices.GraphicsFamily), 0, &Context->GraphicsQueue);
		//vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.PresentationFamily), 0, &PresentationQueue);
		//vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.TransferFamily), 0, &TransferQueue);

		//if (GraphicsQueue == nullptr || PresentationQueue == nullptr || TransferQueue == nullptr)
		{
			//return false;
		}

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context->PhysicalDevice, Context->Surface, &SurfaceCapabilities));

		VkPresentModeKHR PresentationMode = VulkanHelper::GetBestPresentationMode(Context->PhysicalDevice, Context->Surface);

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
		SwapchainCreateInfo.surface = Context->Surface;
		SwapchainCreateInfo.imageFormat = Context->SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = Context->SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = Context->SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		u32 Indices[] = {
			static_cast<u32>(Context->Indices.GraphicsFamily),
			static_cast<u32>(Context->Indices.PresentationFamily)
		};

		if (Context->Indices.GraphicsFamily != Context->Indices.PresentationFamily)
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

		// Used if old swap chain been destroyed and this one replaces it
		SwapchainCreateInfo.oldSwapchain = nullptr;

		VULKAN_CHECK_RESULT(vkCreateSwapchainKHR(Context->LogicalDevice, &SwapchainCreateInfo, nullptr, &Context->VulkanSwapchain));

		Memory::FrameArray<VkImage> Images = VulkanHelper::GetSwapchainImages(Context->LogicalDevice, Context->VulkanSwapchain);

		Context->ImagesCount = Images.Count;

		for (u32 i = 0; i < Context->ImagesCount; ++i)
		{
			Context->Images[i] = Images[i];

			VkImageViewCreateInfo ViewCreateInfo = { };
			ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo.image = Images[i];
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo.format = Context->SurfaceFormat.format;
			ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo.subresourceRange.levelCount = 1;
			ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo.subresourceRange.layerCount = 1;

			VkImageView ImageView;
			VULKAN_CHECK_RESULT(vkCreateImageView(Context->LogicalDevice, &ViewCreateInfo, nullptr, &Context->ImageViews[i]));
		}
	}

	void DestroyCoreContext(VulkanCoreContext* Context)
	{
		for (u32 i = 0; i < Context->ImagesCount; ++i)
		{
			vkDestroyImageView(Context->LogicalDevice, Context->ImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(Context->LogicalDevice, Context->VulkanSwapchain, nullptr);
		vkDestroySurfaceKHR(Context->VulkanInstance, Context->Surface, nullptr);

		vkDestroyDevice(Context->LogicalDevice, nullptr);

		if (Context->DebugMessenger != nullptr)
		{
			VulkanHelper::DestroyDebugMessenger(Context->VulkanInstance, Context->DebugMessenger, nullptr);
		}

		vkDestroyInstance(Context->VulkanInstance, nullptr);
	}
}
#include "VulkanCoreContext.h"

#include <cassert>

#include <SharedLib.h>

#include <GLFW/glfw3.h>

extern Memory_LinearAllocator FrameMemory;

static VkDevice CreateLogicalDevice(VkPhysicalDevice PhDevice, PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
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

	// Query buffer device address feature support
	VkPhysicalDeviceBufferDeviceAddressFeatures QueryBufferDeviceAddressFeatures = { };
	QueryBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		
	VkPhysicalDeviceFeatures2 QueryFeatures2 = { };
	QueryFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	QueryFeatures2.pNext = &QueryBufferDeviceAddressFeatures;
	vkGetPhysicalDeviceFeatures2(PhDevice, &QueryFeatures2);
		
	// Enable buffer device address feature if supported
	VkPhysicalDeviceBufferDeviceAddressFeatures BufferDeviceAddressFeatures = { };
	BufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	BufferDeviceAddressFeatures.bufferDeviceAddress = QueryBufferDeviceAddressFeatures.bufferDeviceAddress ? VK_TRUE : VK_FALSE;
	BufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
	BufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;

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
	IndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures TimelineSemaphoreFeatures = { };
	TimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	TimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	// Query indirect drawing feature support
	VkPhysicalDeviceFeatures QueryDeviceFeatures = { };
	vkGetPhysicalDeviceFeatures(PhDevice, &QueryDeviceFeatures);

	// Enable indirect drawing features if supported
	VkPhysicalDeviceFeatures DeviceFeatures = { };
	DeviceFeatures.samplerAnisotropy = VK_TRUE;
	DeviceFeatures.multiDrawIndirect = QueryDeviceFeatures.multiDrawIndirect ? VK_TRUE : VK_FALSE;
	DeviceFeatures.drawIndirectFirstInstance = QueryDeviceFeatures.drawIndirectFirstInstance ? VK_TRUE : VK_FALSE;

	VkPhysicalDeviceVulkan11Features features11 = {};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.shaderDrawParameters = VK_TRUE;

	DeviceFeatures2.pNext = &TimelineSemaphoreFeatures;
	TimelineSemaphoreFeatures.pNext = &IndexingFeatures;
	IndexingFeatures.pNext = &Sync2Features;
	Sync2Features.pNext = &DynamicRenderingFeatures;
	DynamicRenderingFeatures.pNext = &BufferDeviceAddressFeatures;
	BufferDeviceAddressFeatures.pNext = &features11;


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
	VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, PhysicalDeviceIndices Indices,
	VkPhysicalDevice Device, VkPhysicalDeviceProperties* DeviceProperties)
{
	VkPhysicalDeviceFeatures AvailableFeatures;
	vkGetPhysicalDeviceFeatures(Device, &AvailableFeatures);
		
	PrintDeviceData(DeviceProperties, &AvailableFeatures);

	if (!CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
	{
		RenderLog(LogType::Warning, "PhysicalDeviceIndices are not initialized");
		return false;
	}

	if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
	{
		RenderLog(LogType::Warning, "PhysicalDeviceIndices are not initialized");
		return false;
	}

	if (!AvailableFeatures.samplerAnisotropy)
	{
		RenderLog(LogType::Warning, "Feature samplerAnisotropy is not supported");
		return false;
	}

	if (!AvailableFeatures.multiViewport)
	{
		RenderLog(LogType::Warning, "Feature multiViewport is not supported");
		return false;
	}

	// Query buffer device address feature support
	VkPhysicalDeviceBufferDeviceAddressFeatures QueryBufferDeviceAddressFeatures = { };
	QueryBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		
	VkPhysicalDeviceFeatures2 QueryFeatures2 = { };
	QueryFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	QueryFeatures2.pNext = &QueryBufferDeviceAddressFeatures;
	vkGetPhysicalDeviceFeatures2(Device, &QueryFeatures2);

	// Check if buffer device address feature is supported
	if (QueryBufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE)
	{
		RenderLog(LogType::Info, "Buffer device address feature is supported");
	}
	else
	{
		RenderLog(LogType::Warning, "Buffer device address feature is not supported");
	}

	// Query indirect drawing feature support
	VkPhysicalDeviceFeatures QueryDeviceFeatures = { };
	vkGetPhysicalDeviceFeatures(Device, &QueryDeviceFeatures);

	// Check if indirect drawing features are supported
	if (QueryDeviceFeatures.multiDrawIndirect == VK_TRUE || QueryDeviceFeatures.drawIndirectFirstInstance == VK_TRUE)
	{
		RenderLog(LogType::Info, "Indirect drawing (vkCmdDrawIndirect) features are supported");
	}
	else
	{
		RenderLog(LogType::Warning, "Indirect drawing (vkCmdDrawIndirect) features are not supported");
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

	const u32 ValidationExtensionsCount = sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]);

	u32 ExtensionCount;
	VULKAN_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

	auto AvailableExtensions = (VkExtensionProperties*)Memory_LinearAllocator_Alloc(&FrameMemory, ExtensionCount * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, AvailableExtensions);

	const u32 ExtensionsCount = RequiredExtensionsCount + ValidationExtensionsCount;
	auto RequiredExtensions = (const char**)Memory_LinearAllocator_Alloc(&FrameMemory, RequiredExtensionsCount * sizeof(const char**));
	GetRequiredInstanceExtensions(RequiredInstanceExtensions, RequiredExtensionsCount,
		ValidationExtensions, ValidationExtensionsCount, RequiredExtensions);

	if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions, ExtensionCount,
		RequiredExtensions, ExtensionsCount))
	{
		assert(false);
	}

	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };
	MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	MessengerCreateInfo.pfnUserCallback = MessengerDebugCallback;

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
	CreateInfo.enabledExtensionCount = ExtensionsCount;
	CreateInfo.ppEnabledExtensionNames = RequiredExtensions;

	VULKAN_CHECK_RESULT(vkCreateInstance(&CreateInfo, nullptr, &Context->VulkanInstance));

	if (!CreateDebugUtilsMessengerEXT(Context->VulkanInstance, &MessengerCreateInfo, nullptr, &Context->DebugMessenger))
	{
		RenderLog(LogType::Error, "Cannot create debug messenger");
	}

	VULKAN_CHECK_RESULT(glfwCreateWindowSurface(Context->VulkanInstance, Context->WindowHandler, nullptr, &Context->Surface));

	const char* DeviceExtensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	};

	const u32 DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

	u32 DeviceCount;
	vkEnumeratePhysicalDevices(Context->VulkanInstance, &DeviceCount, nullptr);

	auto DeviceList = (VkPhysicalDevice*)Memory_LinearAllocator_Alloc(&FrameMemory, DeviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(Context->VulkanInstance, &DeviceCount, DeviceList);

	bool IsDeviceFound = false;
	for (u32 i = 0; i < DeviceCount; ++i)
	{
		Context->PhysicalDevice = DeviceList[i];

		u32 DeviceExtensionCount;
		VULKAN_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(Context->PhysicalDevice, nullptr, &DeviceExtensionCount, nullptr));

		auto DeviceExtensionsData = (VkExtensionProperties*)Memory_LinearAllocator_Alloc(&FrameMemory, DeviceExtensionCount * sizeof(VkExtensionProperties));
		VULKAN_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(Context->PhysicalDevice, nullptr, &DeviceExtensionCount, DeviceExtensionsData));

		u32 QueueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(Context->PhysicalDevice, &QueueFamilyCount, nullptr);

		auto FamilyPropertiesData = (VkQueueFamilyProperties*)Memory_LinearAllocator_Alloc(&FrameMemory, QueueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(Context->PhysicalDevice, &QueueFamilyCount, FamilyPropertiesData);

		Context->Indices = GetPhysicalDeviceIndices(FamilyPropertiesData, QueueFamilyCount, Context->PhysicalDevice, Context->Surface);

		VkPhysicalDeviceProperties DeviceProperties;
		vkGetPhysicalDeviceProperties(Context->PhysicalDevice, &DeviceProperties);

		IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
			DeviceExtensionsData, DeviceExtensionCount, Context->Indices, Context->PhysicalDevice, &DeviceProperties);

		if (IsDeviceFound)
		{
			IsDeviceFound = true;
			break;
		}
	}

	if (!IsDeviceFound)
	{
		RenderLog(LogType::Error, "Cannot find suitable device");
	}

	Context->LogicalDevice = CreateLogicalDevice(Context->PhysicalDevice, Context->Indices, DeviceExtensions, DeviceExtensionsSize);

	u32 SurfaceFormatCount;
	VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(Context->PhysicalDevice, Context->Surface, &SurfaceFormatCount, nullptr));

	auto AvailableFormats = (VkSurfaceFormatKHR*)Memory_LinearAllocator_Alloc(&FrameMemory, SurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(Context->PhysicalDevice, Context->Surface, &SurfaceFormatCount, AvailableFormats);

	Context->SurfaceFormat = GetBestSurfaceFormat(Context->Surface, AvailableFormats, SurfaceFormatCount);

	CheckFormats(Context->PhysicalDevice);
	Context->SwapExtent = GetBestSwapExtent(Context->PhysicalDevice, Context->WindowHandler, Context->Surface);

	VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
	VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context->PhysicalDevice, Context->Surface, &SurfaceCapabilities));

	VkPresentModeKHR PresentationMode = GetBestPresentationMode(Context->PhysicalDevice, Context->Surface);

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
	//SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_STORAGE_BIT;
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

	u32 SwapchainImageCount;
	vkGetSwapchainImagesKHR(Context->LogicalDevice, Context->VulkanSwapchain, &SwapchainImageCount, nullptr);

	auto Images = (VkImage*)Memory_LinearAllocator_Alloc(&FrameMemory, SwapchainImageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(Context->LogicalDevice, Context->VulkanSwapchain, &SwapchainImageCount, Images);

	Context->ImagesCount = SwapchainImageCount;

	for (u32 i = 0; i < Context->ImagesCount; ++i)
	{
		BmRender_Image ImageResourceData = { };
		ImageResourceData.Type = BmRender_ImageType::TransferSampled;
		ImageResourceData.Memory = nullptr;
		ImageResourceData.Format = VkFormatToBmRender(Context->SurfaceFormat.format);
		ImageResourceData.Width = Context->SwapExtent.width;
		ImageResourceData.Height = Context->SwapExtent.height;
		ImageResourceData.Size = 0;
		ImageResourceData.InternalImage = Images[i];

		Context->Images[i] = ImageResourceData;
		Context->ImageViews[i] = BmRender_CreateImageView2D(Context->Images + i);
	}
}

void DestroyCoreContext(VulkanCoreContext* Context)
{
	for (u32 i = 0; i < Context->ImagesCount; i++)
	{
		BmRender_DestroyImageView(Context->ImageViews[i]);
	}

	vkDestroySwapchainKHR(Context->LogicalDevice, Context->VulkanSwapchain, nullptr);
	vkDestroySurfaceKHR(Context->VulkanInstance, Context->Surface, nullptr);

	vkDestroyDevice(Context->LogicalDevice, nullptr);

	if (Context->DebugMessenger != nullptr)
	{
		DestroyDebugMessenger(Context->VulkanInstance, Context->DebugMessenger, nullptr);
	}

	vkDestroyInstance(Context->VulkanInstance, nullptr);
}

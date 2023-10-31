//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <vector>
#include <cassert>
#include <algorithm>

#include "Core/VulkanCoreTypes.h"

static const char* ValidationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
static constexpr uint32_t ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

static inline VkDebugUtilsMessengerEXT _DebugMessenger;

int Start()
{
	if (glfwInit() == GL_FALSE)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(800, 600, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	// Function: GetRequiredInstanceExtensions
	uint32_t RequiredExtensionsCount = 0;
	const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&RequiredExtensionsCount);
	if (RequiredInstanceExtensions == nullptr)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	uint32_t AvalibleExtensionsCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &AvalibleExtensionsCount, nullptr);

	VkExtensionProperties* AvalibleExtensions = Util::Memory::Allocate<VkExtensionProperties>(AvalibleExtensionsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &AvalibleExtensionsCount, AvalibleExtensions);

	for (uint32_t i = 0; i < RequiredExtensionsCount; ++i)
	{
		bool IsExtentionSupported = false;
		for (uint32_t j = 0; j < AvalibleExtensionsCount; ++j)
		{
			if (std::strcmp(RequiredInstanceExtensions[i], AvalibleExtensions[j].extensionName) == 0)
			{
				IsExtentionSupported = true;
				break;
			}
		}

		if (!IsExtentionSupported)
		{
			Util::Memory::Deallocate(AvalibleExtensions);
			Util::Log().Error("Extension {} unsupported", RequiredInstanceExtensions[i]);
			return -1;
		}
	}

	Util::Memory::Deallocate(AvalibleExtensions);

	uint32_t TotalExtensionsCount = RequiredExtensionsCount;
	const char** TotalExtensions = nullptr;

	if (Util::EnableValidationLayers)
	{
		++TotalExtensionsCount;
		TotalExtensions = Util::Memory::Allocate<const char*>(TotalExtensionsCount);

		uint32_t i = 0;
		for (; i < RequiredExtensionsCount; i++)
		{
			TotalExtensions[i] = RequiredInstanceExtensions[i];
		}

		TotalExtensions[i] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}
	else
	{
		TotalExtensions = RequiredInstanceExtensions;
	}
	// Function end: GetRequiredInstanceExtensions

	// Function: CreateInstance
	VkApplicationInfo ApplicationInfo = { };
	ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.pApplicationName = "Blank App";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
	ApplicationInfo.pEngineName = "BMEngine";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
	ApplicationInfo.apiVersion = VK_API_VERSION_1_3;


	// CreateInfo info initialization
	VkInstanceCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &ApplicationInfo;
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(TotalExtensionsCount);
	CreateInfo.ppEnabledExtensionNames = TotalExtensions;

	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };

	if (Util::EnableValidationLayers)
	{
		MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
		MessengerCreateInfo.pUserData = nullptr;

		CreateInfo.pNext = &MessengerCreateInfo;
	}
	else
	{
		CreateInfo.pNext = nullptr;
	}

	// Function: GetEnabledValidationLayers 
	if (Util::EnableValidationLayers)
	{
		uint32_t LayerCount = 0;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		VkLayerProperties* AvalibleLayers = Util::Memory::Allocate<VkLayerProperties>(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, AvalibleLayers);

		for (uint32_t i = 0; i < ValidationLayersSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < LayerCount; ++j)
			{
				if (std::strcmp(ValidationLayers[i], AvalibleLayers[j].layerName) == 0)
				{
					IsLayerAvalible = true;
					break;
				}
			}

			if (!IsLayerAvalible)
			{
				Util::Memory::Deallocate(AvalibleLayers);
				Util::Log().Error("Extension {} unsupported", RequiredInstanceExtensions[i]);
				return -1;
			}
		}
		Util::Memory::Deallocate(AvalibleLayers);

		CreateInfo.enabledLayerCount = ValidationLayersSize;
		CreateInfo.ppEnabledLayerNames = ValidationLayers;
	}
	else
	{
		CreateInfo.ppEnabledLayerNames = nullptr;
		CreateInfo.enabledLayerCount = 0;
	}
	// Function end: GetEnabledValidationLayers 

	VkInstance VulkanInstance = nullptr;
	VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &VulkanInstance);
	if (Result != VK_SUCCESS)
	{
		if (TotalExtensionsCount > RequiredExtensionsCount)
		{
			Util::Memory::Deallocate(TotalExtensions);
		}

		Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
		return -1;
	}

	if (TotalExtensionsCount > RequiredExtensionsCount)
	{
		Util::Memory::Deallocate(TotalExtensions);
	}
	// Function end: CreateInstance

	// Function: SetupDebugMessenger
	if (Util::EnableValidationLayers)
	{
		Util::CreateDebugUtilsMessengerEXT(VulkanInstance, &MessengerCreateInfo, nullptr, &_DebugMessenger);
	}
	// Function end: SetupDebugMessenger

	VkSurfaceKHR Surface = nullptr;
	if (glfwCreateWindowSurface(VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	// Function: SetupPhysicalDevice
	int GraphicsFamily = -1;
	int PresentationFamily = -1;

	uint32_t DevicesCount = 0;
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, nullptr);

	VkPhysicalDevice* DeviceList = Util::Memory::Allocate<VkPhysicalDevice>(DevicesCount);
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, DeviceList);

	std::vector<VkSurfaceFormatKHR> Formats;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities{};
	VkPhysicalDevice PhysicalDevice = nullptr;
	std::vector<VkPresentModeKHR> PresentationModes;
	static std::vector<const char*> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	for (uint32_t i = 0; i < DevicesCount; ++i)
	{
		PhysicalDevice = DeviceList[i];

		/*
		// ID, name, type, vendor, etc
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);

		// geo shader, tess shader, wide lines, etc
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device, &Features);
		*/

		uint32_t ExtensionsCount = 0;
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionsCount, nullptr);

		VkExtensionProperties* AvalibleDeviceExtensions = Util::Memory::Allocate<VkExtensionProperties>(ExtensionsCount);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionsCount, AvalibleDeviceExtensions);

		for (const char* Extension : DeviceExtensions)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < ExtensionsCount; ++j)
			{
				if (std::strcmp(Extension, AvalibleDeviceExtensions[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				Util::Log().Error("Device extension {} unsupported", Extension);

				Util::Memory::Deallocate(AvalibleDeviceExtensions);
				Util::Memory::Deallocate(DeviceList);
				return -1;
			}
		}

		Util::Memory::Deallocate(AvalibleDeviceExtensions);

		// TODO check function
		// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
		// GraphicsFamily could be overridden on next iteration even when it is valid
		uint32_t FamilyCount = 1;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &FamilyCount, nullptr);

		VkQueueFamilyProperties* FamilyProperties = Util::Memory::Allocate<VkQueueFamilyProperties>(FamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &FamilyCount, FamilyProperties);

		for (uint32_t j = 0; j < FamilyCount; ++j)
		{
			// check if Queue is graphics type
			if (FamilyProperties[j].queueCount > 0 && FamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilys
				GraphicsFamily = j;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, j, Surface, &PresentationSupport);
			if (FamilyProperties[j].queueCount > 0 && PresentationSupport)
			{
				PresentationFamily = j;
			}

			if (GraphicsFamily < 0 && PresentationFamily < 0)
			{
				// Todo walidate warning
				Util::Log().Warning("Device does not support required indices");
				break;
			}
		}

		if (GraphicsFamily < 0 && PresentationFamily < 0)
		{
			PhysicalDevice = nullptr;
			break;
		}

		Util::Memory::Deallocate(FamilyProperties);

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);

		uint32_t FormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, nullptr);
		if (FormatCount == 0)
		{
			Util::Log().Error("FormatCount is 0");
			PhysicalDevice = nullptr;
		}

		Formats.resize(static_cast<size_t>(FormatCount));
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, Formats.data());

		uint32_t PresentationCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentationCount, nullptr);
		if (PresentationCount == 0)
		{
			Util::Log().Error("PresentationCount is 0");
			PhysicalDevice = nullptr;
		}
		else
		{
			PresentationModes.resize(static_cast<size_t>(PresentationCount));
			vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentationCount, PresentationModes.data());
		}

		if (PhysicalDevice == nullptr)
		{
			Util::Memory::Deallocate(DeviceList);
			Util::Log().Error("No physical devices found");
			return -1;
		}
	}

	Util::Memory::Deallocate(DeviceList);
	// Function end: SetupPhysicalDevice

	// Function: CreateLogicalDevice
	//if (!MainDevice.CreateLogicalDevice())
	//{
	//	return -1;
	//}
	
	const float Priority = 1.0f;

	// One family can suppurt graphics and presentation
	// In that case create mulltiple VkDeviceQueueCreateInfo
	VkDeviceQueueCreateInfo QueueCreateInfos[2] = {};
	uint32_t FamilyIndicesSize = 1;

	QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(GraphicsFamily);
	QueueCreateInfos[0].queueCount = 1;
	QueueCreateInfos[0].pQueuePriorities = &Priority;

	if (GraphicsFamily != PresentationFamily)
	{
		QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(PresentationFamily);
		QueueCreateInfos[1].queueCount = 1;
		QueueCreateInfos[1].pQueuePriorities = &Priority;

		++FamilyIndicesSize;
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};

	VkDeviceCreateInfo DeviceCreateInfo = { };
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = (FamilyIndicesSize);
	DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
	DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

	// Queues are created at the same time as the device
	VkDevice LogicalDevice = nullptr;
	Result = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
		return -1;
	}

	// Function end: CreateLogicalDevice

	// Function: SetupQueues
	VkQueue GraphicsQueue = nullptr;
	VkQueue PresentationQueue = nullptr;

	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(GraphicsFamily), 0, &GraphicsQueue);
	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(PresentationFamily), 0, &PresentationQueue);

	if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
	{
		return -1;
	}
	// Function end: SetupQueues

	// Function: GetBestSurfaceFormat
	// Return most common format
	VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
	// All formats avalible
	if (Formats.size() == 1 && Formats[0].format == VK_FORMAT_UNDEFINED)
	{
		SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		for (const VkSurfaceFormatKHR Format : Formats)
		{
			if ((Format.format == VK_FORMAT_R8G8B8_UNORM || Format.format == VK_FORMAT_B8G8R8A8_UNORM)
				&& Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				SurfaceFormat = Format;
			}
		}
	}

	if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
	{
		Util::Log().Error("SurfaceFormat is undefined");
		return -1;
	}
	// Function end: GetBestSurfaceFormat

	// Function: GetBestPresentationMode
	// Optimal presentation mode
	VkPresentModeKHR PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR Mode : PresentationModes)
	{
		if (Mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			PresentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	// Has to be present by spec
	if (PresentationMode != VK_PRESENT_MODE_MAILBOX_KHR)
	{
		Util::Log().Warning("Using default VK_PRESENT_MODE_FIFO_KHR");
	}
	// Function end: GetBestPresentationMode

	// Function: GetBestSwapExtent
	VkExtent2D SwapExtent;
	if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		SwapExtent = SurfaceCapabilities.currentExtent;
	}
	else
	{
		int Width;
		int Height;
		glfwGetFramebufferSize(Window, &Width, &Height);

		Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
		Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

		SwapExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
	}
	// Function end: GetBestSwapExtent

	// How many images are in the swap chain
	// Get 1 more then the minimum to allow triple buffering
	uint32_t ImageCount = SurfaceCapabilities.minImageCount + 1;

	// If maxImageCount > 0, then limitless
	if (SurfaceCapabilities.maxImageCount > 0
		&& SurfaceCapabilities.maxImageCount < ImageCount)
	{
		ImageCount = SurfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
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

	uint32_t Indices[] = {
		static_cast<uint32_t>(GraphicsFamily),
		static_cast<uint32_t>(PresentationFamily)
	};

	if (GraphicsFamily != PresentationFamily)
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

	VkSwapchainKHR VulkanSwapchain = nullptr;
	Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &VulkanSwapchain);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
		return -1;
	}

	// Get swap chain images
	uint32_t SwapchainImageCount = 0;
	vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &SwapchainImageCount, nullptr);

	VkImage* Images = Util::Memory::Allocate<VkImage>(SwapchainImageCount);
	vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &SwapchainImageCount, Images);

	VkImageViewCreateInfo ViewCreateInfo = {};
	ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ViewCreateInfo.format = SurfaceFormat.format;
	ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an image
	ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ViewCreateInfo.subresourceRange.levelCount = 1;
	ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ViewCreateInfo.subresourceRange.layerCount = 1;

	VkImageView* ImageViews = Util::Memory::Allocate<VkImageView>(SwapchainImageCount);

	for (uint32_t i = 0; i < SwapchainImageCount; ++i)
	{
		ViewCreateInfo.image = Images[i];

		VkImageView ImageView;
		Result = vkCreateImageView(LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateImageView result is {}", static_cast<int>(Result));
			Util::Memory::Deallocate(Images);
			Util::Memory::Deallocate(ImageViews);
			return -1;
		}

		ImageViews[i] = ImageView;
	}

	Util::Memory::Deallocate(Images);
	// Function end: CreateSwapchain

	// TODO FIX!!!
	std::vector<char> VertexShaderCode;
	Util::UtilHelper::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");

	std::vector<char> FragmentShaderCode;
	Util::UtilHelper::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");

	// TODO build shader modules

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}

	// TODO: add destroing to invalid initialization
	for (uint32_t i = 0; i < SwapchainImageCount; ++i)
	{
		vkDestroyImageView(LogicalDevice, ImageViews[i], nullptr);
	}
	Util::Memory::Deallocate(ImageViews);

	vkDestroySwapchainKHR(LogicalDevice, VulkanSwapchain, nullptr);

	vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
	vkDestroyDevice(LogicalDevice, nullptr);

	if (Util::EnableValidationLayers)
	{
		Util::DestroyDebugMessenger(VulkanInstance, _DebugMessenger, nullptr);
	}

	vkDestroyInstance(VulkanInstance, nullptr);

	glfwDestroyWindow(Window);
	glfwTerminate();

	return 0;
}

int main()
{
	const int Result = Start();

	if (Util::Memory::AlocateCounter != 0)
	{
		Util::Log().Error("AlocateCounter in not equal 0, counter is {}", Util::Memory::AlocateCounter);
		assert(false);
	}
	
	return Result;
}
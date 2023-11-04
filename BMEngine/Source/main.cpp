//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

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

	const char* ValidationExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	const uint32_t ValidationExtensionsSize = Util::EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

	Core::VkInstanceCreateInfoSetupData InstanceCreateInfoData;
	if (!Core::InitVkInstanceCreateInfoSetupData(InstanceCreateInfoData, ValidationExtensions, ValidationExtensionsSize, Util::EnableValidationLayers))
	{
		return -1;
	}

	if (!Core::CheckRequiredInstanceExtensionsSupport(InstanceCreateInfoData))
	{
		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);
		return -1;
	}

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

	if (Util::EnableValidationLayers)
	{
		CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredAndValidationExtensionsCount;
		CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredAndValidationExtensions;
	}
	else
	{
		CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredExtensionsCount;
		CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredInstanceExtensions;
	}

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
		Core::CheckValidationLayersSupport(InstanceCreateInfoData, ValidationLayers, ValidationLayersSize);

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
		Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

		return -1;
	}

	Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);
	// Function end: CreateInstance

	if (Util::EnableValidationLayers)
	{
		Util::CreateDebugUtilsMessengerEXT(VulkanInstance, &MessengerCreateInfo, nullptr, &_DebugMessenger);
	}

	GLFWwindow* Window = glfwCreateWindow(800, 600, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	VkSurfaceKHR Surface = nullptr;
	if (glfwCreateWindowSurface(VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	// Function: SetupPhysicalDevice
	uint32_t DevicesCount = 0;
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, nullptr);

	VkPhysicalDevice* DeviceList = static_cast<VkPhysicalDevice*>(Util::Memory::Allocate(DevicesCount * sizeof(VkPhysicalDevice)));
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, DeviceList);

	Core::VkPhysicalDeviceSetupData VkPhysicalDeviceSetupData;

	Core::PhysicalDeviceIndices PhysicalDeviceIndices;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

	VkPhysicalDevice PhysicalDevice = nullptr;

	const char* DeviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const uint32_t DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

	for (uint32_t i = 0; i < DevicesCount; ++i)
	{	
		/* ID, name, type, vendor, etc
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);

		// geo shader, tess shader, wide lines, etc
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device, &Features); */
		
		if (!Core::InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData, DeviceList[i], Surface))
		{
			break;
		}

		if (!Core::CheckDeviceExtensionsSupport(VkPhysicalDeviceSetupData, DeviceExtensions, DeviceExtensionsSize))
		{
			break;
		}

		PhysicalDeviceIndices = Core::GetPhysicalDeviceIndices(VkPhysicalDeviceSetupData, DeviceList[i], Surface);

		if (PhysicalDeviceIndices.GraphicsFamily < 0 || PhysicalDeviceIndices.PresentationFamily < 0)
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			break;
		}

		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceList[i], Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			break;
		}

		PhysicalDevice = DeviceList[i];
	}

	if (PhysicalDevice == nullptr)
	{
		Util::Memory::Deallocate(DeviceList);
		Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);
		Util::Log().Error("No physical devices found");
		return -1;
	}

	Util::Memory::Deallocate(DeviceList);
	// Function end: SetupPhysicalDevice

	// Function: CreateLogicalDevice
	const float Priority = 1.0f;

	// One family can suppurt graphics and presentation
	// In that case create mulltiple VkDeviceQueueCreateInfo
	VkDeviceQueueCreateInfo QueueCreateInfos[2] = {};
	uint32_t FamilyIndicesSize = 1;

	QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily);
	QueueCreateInfos[0].queueCount = 1;
	QueueCreateInfos[0].pQueuePriorities = &Priority;

	if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
	{
		QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily);
		QueueCreateInfos[1].queueCount = 1;
		QueueCreateInfos[1].pQueuePriorities = &Priority;

		++FamilyIndicesSize;
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};

	VkDeviceCreateInfo DeviceCreateInfo = { };
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = (FamilyIndicesSize);
	DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
	DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
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

	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily), 0, &GraphicsQueue);
	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily), 0, &PresentationQueue);

	if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
	{
		return -1;
	}
	// Function end: SetupQueues

	// Function: GetBestSurfaceFormat
	// Return most common format
	VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
	// All formats avalible
	if (VkPhysicalDeviceSetupData.SurfaceFormatsCount == 1 && VkPhysicalDeviceSetupData.SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.SurfaceFormatsCount; ++i)
		{
			VkSurfaceFormatKHR Format = VkPhysicalDeviceSetupData.SurfaceFormats[i];
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
	for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.PresentModesCount; ++i)
	{
		if (VkPhysicalDeviceSetupData.PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			PresentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);

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
		static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily),
		static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily)
	};

	if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
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

	VkImage* Images = static_cast<VkImage*>(Util::Memory::Allocate(SwapchainImageCount * sizeof(VkImage)));
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

	VkImageView* ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(SwapchainImageCount * sizeof(VkImageView)));

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
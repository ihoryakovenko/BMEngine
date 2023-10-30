//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <vector>
#include <cassert>

#include "Core/VulkanCoreTypes.h"

#ifdef NDEBUG
static bool EnableValidationLayers = false;
#else
static bool EnableValidationLayers = true;
#endif

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

	if (EnableValidationLayers)
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

	if (EnableValidationLayers)
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
	if (EnableValidationLayers)
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
	if (EnableValidationLayers)
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
	Core::MainDevice MainDevice;
	if (!MainDevice.SetupPhysicalDevice(VulkanInstance, Surface))
	{
		return -1;
	}
	// Function end: SetupPhysicalDevice

	if (!MainDevice.CreateLogicalDevice())
	{
		return -1;
	}
	// Function end: SetupPhysicalDevice

	// Function: SetupQueues
	VkQueue GraphicsQueue = nullptr;
	VkQueue PresentationQueue = nullptr;

	vkGetDeviceQueue(MainDevice._LogicalDevice, static_cast<uint32_t>(MainDevice._GraphicsFamily), 0, &GraphicsQueue);
	vkGetDeviceQueue(MainDevice._LogicalDevice, static_cast<uint32_t>(MainDevice._PresentationFamily), 0, &PresentationQueue);

	if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
	{
		return -1;
	}
	// Function end: SetupQueues

	// Function: CreateSwapchain
	VkSurfaceFormatKHR SurfaceFormat = MainDevice.GetBestSurfaceFormat();
	if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
	{
		return -1;
	}

	VkPresentModeKHR PresentationMode = MainDevice.GetBestPresentationMode();
	VkExtent2D SwapExtent = MainDevice.GetBestSwapExtent(Window);

	// How many images are in the swap chain
	// Get 1 more then the minimum to allow triple buffering
	uint32_t ImageCount = MainDevice._SurfaceCapabilities.minImageCount + 1;

	// If maxImageCount > 0, then limitless
	if (MainDevice._SurfaceCapabilities.maxImageCount > 0
		&& MainDevice._SurfaceCapabilities.maxImageCount < ImageCount)
	{
		ImageCount = MainDevice._SurfaceCapabilities.maxImageCount;
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
	SwapchainCreateInfo.preTransform = MainDevice._SurfaceCapabilities.currentTransform;
	SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
	SwapchainCreateInfo.clipped = VK_TRUE;

	uint32_t Indices[] = {
		static_cast<uint32_t>(MainDevice._GraphicsFamily),
		static_cast<uint32_t>(MainDevice._PresentationFamily)
	};

	if (MainDevice._GraphicsFamily != MainDevice._PresentationFamily)
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
	Result = vkCreateSwapchainKHR(MainDevice._LogicalDevice, &SwapchainCreateInfo, nullptr, &VulkanSwapchain);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
		return -1;
	}

	// Get swap chain images
	uint32_t SwapchainImageCount = 0;
	vkGetSwapchainImagesKHR(MainDevice._LogicalDevice, VulkanSwapchain, &SwapchainImageCount, nullptr);

	VkImage* Images = Util::Memory::Allocate<VkImage>(SwapchainImageCount);
	vkGetSwapchainImagesKHR(MainDevice._LogicalDevice, VulkanSwapchain, &SwapchainImageCount, Images);

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
		Result = vkCreateImageView(MainDevice._LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
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
		vkDestroyImageView(MainDevice._LogicalDevice, ImageViews[i], nullptr);
	}
	Util::Memory::Deallocate(ImageViews);

	vkDestroySwapchainKHR(MainDevice._LogicalDevice, VulkanSwapchain, nullptr);

	vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
	vkDestroyDevice(MainDevice._LogicalDevice, nullptr);

	if (EnableValidationLayers)
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
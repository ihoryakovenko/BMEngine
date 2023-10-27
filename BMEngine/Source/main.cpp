//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <iostream>
#include <vector>

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

void CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
	const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger)
{
	auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
	if (CreateMessengerFunc != nullptr)
	{
		const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, InDebugMessenger);
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

void DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
	const VkAllocationCallbacks* Allocator)
{
	if (EnableValidationLayers)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, InDebugMessenger, Allocator);
		}
		else
		{
			Util::Log().Error("DestroyMessengerFunc is nullptr");
		}
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
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

int main()
{
	const int InitResult = glfwInit();
	if (InitResult == GL_FALSE)
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

	VkExtensionProperties* AvalibleExtensions = new VkExtensionProperties[AvalibleExtensionsCount];
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
			delete[] AvalibleExtensions;
			Util::Log().Error("Extension {} unsupported", RequiredInstanceExtensions[i]);
			return -1;
		}
	}

	delete[] AvalibleExtensions;

	uint32_t TotalExtensionsCount = RequiredExtensionsCount;
	const char** TotalExtensions = nullptr;

	if (EnableValidationLayers)
	{
		++TotalExtensionsCount;
		TotalExtensions = new const char* [TotalExtensionsCount];

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

		MessengerCreateInfo.pfnUserCallback = DebugCallback;
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

		VkLayerProperties* AvalibleLayers = new VkLayerProperties[LayerCount];
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
				delete[] AvalibleLayers;
				Util::Log().Error("Extension {} unsupported", RequiredInstanceExtensions[i]);
				return -1;
			}
		}
		delete[] AvalibleLayers;

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
	const VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &VulkanInstance);
	if (Result != VK_SUCCESS)
	{
		if (TotalExtensionsCount > RequiredExtensionsCount)
		{
			delete[] TotalExtensions;
		}

		Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
		return -1;
	}

	if (TotalExtensionsCount > RequiredExtensionsCount)
	{
		delete[] TotalExtensions;
	}
	// Function end: CreateInstance

	// Function: SetupDebugMessenger
	if (EnableValidationLayers)
	{
		CreateDebugUtilsMessengerEXT(VulkanInstance, &MessengerCreateInfo, nullptr, &_DebugMessenger);
	}
	// Function end: SetupDebugMessenger

	Core::Surface Surface;
	 Surface.CreateSurface(VulkanInstance, Window, nullptr);
	if (Surface._Surface == nullptr)
	{
		return -1;
	}

	Core::MainDevice _MainDevice;
	if (!_MainDevice.SetupPhysicalDevice(VulkanInstance, Surface._Surface))
	{
		return -1;
	}

	if (!_MainDevice.CreateLogicalDevice())
	{
		return -1;
	}

	Core::Queues _Queues;
	_Queues.SetupQueues(_MainDevice._LogicalDevice, _MainDevice._PhysicalDeviceIndices);

	Core::Swapchain _Swapchain;
	if (!_Swapchain.CreateSwapchain(_MainDevice, Window, Surface._Surface))
	{
		return -1;
	}

	//
	Core::GraphicsPipeline _GraphicsPipeline;
	if (!_GraphicsPipeline.CreateGraphicsPipeline())
	{
		return -1;
	}

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}

	_Swapchain.CleanupSwapchain(_MainDevice._LogicalDevice);
	vkDestroySurfaceKHR(VulkanInstance, Surface._Surface, nullptr);
	vkDestroyDevice(_MainDevice._LogicalDevice, nullptr);
	DestroyDebugMessenger(VulkanInstance, _DebugMessenger, nullptr);
	vkDestroyInstance(VulkanInstance, nullptr);

	glfwDestroyWindow(Window);
	glfwTerminate();

	return 0;
}
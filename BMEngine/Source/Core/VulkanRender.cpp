#include <vector>
#include <algorithm>
#include <cassert>
#include <unordered_set>

#include "VulkanRender.h"
#include "Util/Log.h"
#include "Window.h"

namespace Core
{
	bool VulkanRender::Init(Window* NewWindow)
	{
		assert(NewWindow != nullptr);
		_Window = NewWindow;
		if (!CreateInstance())
		{
			return false;
		}

		VulkanUtil::SetupDebugMessenger(_Instance);

		_Surface = _Window->CreateSurface(_Instance, nullptr);
		if (_Surface == nullptr)
		{
			return false;
		}

		if (!SetupPhysicalDevice())
		{
			return false;
		}

		if (!CreateLogicalDevice())
		{
			return false;
		}

		if (!CreateSwapchain())
		{
			return false;
		}

		return true;
	}

	void VulkanRender::Cleanup()
	{
		for (auto Image : _SwapchainImages)
		{
			vkDestroyImageView(_LogicalDevice, Image._ImageView, nullptr);
		}

		vkDestroySwapchainKHR(_LogicalDevice, _Swapchain, nullptr);
		vkDestroySurfaceKHR(_Instance, _Surface, nullptr); // todo check
		vkDestroyDevice(_LogicalDevice, nullptr);
		VulkanUtil::DestroyDebugMessenger(_Instance);
		vkDestroyInstance(_Instance, nullptr);
	}

	bool VulkanRender::CreateInstance()
	{
		// ApplicationInfo initialization
		VkApplicationInfo ApplicationInfo = { };
		ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ApplicationInfo.pApplicationName = "Blank App";
		ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.pEngineName = "BMEngine";
		ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

		// Extensions info initialization
		std::vector<const char*> InstanceExtensions;
		if (!VulkanUtil::GetRequiredInstanceExtensions(InstanceExtensions))
		{
			return false;
		}

		// CreateInfo info initialization
		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VulkanUtil::GetEnabledValidationLayers(CreateInfo);

		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size());
		CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

		VulkanUtil::GetDebugCreateInfo(CreateInfo);

		// Create instance
		const VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &_Instance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}

	bool VulkanRender::CreateLogicalDevice()
	{
		const float Priority = 1.0f;

		// One family can suppurt graphics and presentation
		// In that case create mulltiple VkDeviceQueueCreateInfo
		std::unordered_set<int> FamilyIndices = { _PhysicalDeviceIndices._GraphicsFamily,
			_PhysicalDeviceIndices._PresentationFamily };

		const size_t FamilyIndicesSize = FamilyIndices.size();
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos(FamilyIndicesSize);

		int i = 0;
		for (int QueueFamilyIndex : FamilyIndices)
		{
			QueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[i].queueFamilyIndex = static_cast<uint32_t>(QueueFamilyIndex);
			QueueCreateInfos[i].queueCount = 1;
			QueueCreateInfos[i].pQueuePriorities = &Priority;
			++i;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(FamilyIndicesSize);
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanUtil::_DeviceExtensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = VulkanUtil::_DeviceExtensions.data();
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		// Queues are created at the same time as the device
		const VkResult Result = vkCreateDevice(_PhysicalDevice, &DeviceCreateInfo, nullptr, &_LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}

		vkGetDeviceQueue(_LogicalDevice, static_cast<uint32_t>(_PhysicalDeviceIndices._GraphicsFamily), 0, &_GraphicsQueue);
		vkGetDeviceQueue(_LogicalDevice, static_cast<uint32_t>(_PhysicalDeviceIndices._PresentationFamily), 0, &_PresentationQueue);

		return true;
	}

	bool VulkanRender::CreateSwapchain()
	{
		VkSurfaceFormatKHR SurfaceFormat = _SwapchainDetails.GetBestSurfaceFormat();
		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			return false;
		}

		VkPresentModeKHR PresentationMode = _SwapchainDetails.GetBestPresentationMode();
		VkExtent2D SwapExtent = _SwapchainDetails.GetBestSwapExtent(*_Window);

		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		uint32_t ImageCount = _SwapchainDetails._SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (_SwapchainDetails._SurfaceCapabilities.maxImageCount > 0
			&& _SwapchainDetails._SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = _SwapchainDetails._SurfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = _Surface;
		SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = _SwapchainDetails._SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		uint32_t Indices[] = {
			static_cast<uint32_t>(_PhysicalDeviceIndices._GraphicsFamily),
			static_cast<uint32_t>(_PhysicalDeviceIndices._PresentationFamily)
		};

		if (_PhysicalDeviceIndices._GraphicsFamily != _PhysicalDeviceIndices._PresentationFamily)
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

		const VkResult Result = vkCreateSwapchainKHR(_LogicalDevice, &SwapchainCreateInfo, nullptr, &_Swapchain);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
			return false;
		}

		_SwapchainImageFormat = SurfaceFormat.format;
		_SwapchainExtent = SwapExtent;

		// Get swap chain images
		uint32_t SwapchainImageCount = 0;
		vkGetSwapchainImagesKHR(_LogicalDevice, _Swapchain, &SwapchainImageCount, nullptr);

		std::vector<VkImage> Images(SwapchainImageCount);
		vkGetSwapchainImagesKHR(_LogicalDevice, _Swapchain, &SwapchainImageCount, Images.data());

		for (VkImage Image : Images)
		{
			SwapchainImage SwapImage;
			SwapImage.CreateSwapchainImage(_LogicalDevice, Image, _SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			_SwapchainImages.push_back(SwapImage);
		}

		return true;
	}

	bool VulkanRender::SetupPhysicalDevice()
	{
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, nullptr);
		if (DevicesCount <= 0)
		{
			return false;
		}

		std::vector<VkPhysicalDevice> DeviceList(DevicesCount);
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, DeviceList.data());

		for (const auto& Device : DeviceList)
		{
			// todo validate
			if (VulkanUtil::IsDeviceSuitable(Device) && _PhysicalDeviceIndices.CreateQueueFamilyIndices(Device, _Surface)
				&& _SwapchainDetails.CreateSwapchainDetails(Device, _Surface))
			{
				_PhysicalDevice = Device;
				return true;
			}
		}

		Util::Log().Error("No physical devices found");
		return false;
	}
}

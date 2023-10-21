#include "VulkanCoreTypes.h"
#include "VulkanUtil.h"

#include <unordered_set>
#include <algorithm>

#include "Util/Log.h"
#include "Util/File.h"
#include "Util/Config.h"

namespace Core
{
	// TODO check function
	bool QueueFamilyIndices::CreateQueueFamilyIndices(VkPhysicalDevice Device, VkSurfaceKHR Surface)
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

		// Todo walidate warning
		Util::Log().Warning("Device does not support required indices");

		_GraphicsFamily = -1;
		_PresentationFamily = -i;
		return false;
	}

	bool SwapchainDetails::CreateSwapchainDetails(VkPhysicalDevice Device, VkSurfaceKHR Surface)
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

	// Return most common format
	VkSurfaceFormatKHR SwapchainDetails::GetBestSurfaceFormat() const
	{
		// All formats avalible
		if (_Formats.size() == 1 && _Formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const VkSurfaceFormatKHR Format : _Formats)
		{
			if ((Format.format == VK_FORMAT_R8G8B8_UNORM || Format.format == VK_FORMAT_B8G8R8A8_UNORM)
				&& Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return Format;
			}
		}

		Util::Log().Error("SurfaceFormat is undefined");
		return { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
	}

	// Optimal presentation mode
	VkPresentModeKHR SwapchainDetails::GetBestPresentationMode() const
	{
		for (const VkPresentModeKHR PresentationMode : _PresentationModes)
		{
			if (PresentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		Util::Log().Warning("Using default VK_PRESENT_MODE_FIFO_KHR");
		// Has to be present by spec
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapchainDetails::GetBestSwapExtent(const Window& Window) const
	{
		if (_SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return _SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			Window.GetWindowSize(Width, Height);

			Width = std::clamp(static_cast<uint32_t>(Width), _SurfaceCapabilities.minImageExtent.width, _SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), _SurfaceCapabilities.minImageExtent.height, _SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
	}

	bool SwapchainImage::CreateSwapchainImage(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags)
	{
		VkImageViewCreateInfo ViewCreateInfo = {};
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// Subresources allow the view to view only a part of an image
		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;

		const VkResult Result = vkCreateImageView(LogicalDevice, &ViewCreateInfo, nullptr, &_ImageView);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateImageView result is {}", static_cast<int>(Result));
			return false;
		}

		_Image = Image;

		return true;
	}

	bool MainDevice::SetupPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface)
	{
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(Instance, &DevicesCount, nullptr);
		if (DevicesCount <= 0)
		{
			return false;
		}

		std::vector<VkPhysicalDevice> DeviceList(DevicesCount);
		vkEnumeratePhysicalDevices(Instance, &DevicesCount, DeviceList.data());

		for (const auto& Device : DeviceList)
		{
			_PhysicalDevice = Device;
			if (IsDeviceSuitable(Surface))
			{
				return true;
			}
		}

		Util::Log().Error("No physical devices found");
		return false;
	}

	bool MainDevice::CreateLogicalDevice()
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
		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_DeviceExtensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = _DeviceExtensions.data();
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		// Queues are created at the same time as the device
		const VkResult Result = vkCreateDevice(_PhysicalDevice, &DeviceCreateInfo, nullptr, &_LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}

	bool MainDevice::IsDeviceExtensionSupported(const char* Extension) const
	{
		uint32_t ExtensionsCount = 0;
		vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &ExtensionsCount, nullptr);

		std::vector<VkExtensionProperties> AvalibleDeviceExtensions(ExtensionsCount);
		vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &ExtensionsCount, AvalibleDeviceExtensions.data());

		for (const auto& AvalibleExtension : AvalibleDeviceExtensions)
		{
			if (std::strcmp(Extension, AvalibleExtension.extensionName) == 0)
			{
				return true;
			}
		}

		Util::Log().Error("Device extension {} unsupported", Extension);
		return false;
	}

	bool MainDevice::IsDeviceSuitable(VkSurfaceKHR Surface)
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
			if (!IsDeviceExtensionSupported(Extension))
			{
				return false;
			}
		}

		if (!_PhysicalDeviceIndices.CreateQueueFamilyIndices(_PhysicalDevice, Surface))
		{
			return false;
		}

		if (!_SwapchainDetails.CreateSwapchainDetails(_PhysicalDevice, Surface))
		{
			return false;
		}

		return true;
	}

	void Queues::SetupQueues(VkDevice LogicalDevice, QueueFamilyIndices Indices)
	{
		vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(Indices._GraphicsFamily), 0, &_GraphicsQueue);
		vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(Indices._PresentationFamily), 0, &_PresentationQueue);
	}

	bool Swapchain::CreateSwapchain(MainDevice& Device, Window ActiveWindow, VkSurfaceKHR Surface)
	{
		VkSurfaceFormatKHR SurfaceFormat = Device._SwapchainDetails.GetBestSurfaceFormat();
		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			return false;
		}

		VkPresentModeKHR PresentationMode = Device._SwapchainDetails.GetBestPresentationMode();
		VkExtent2D SwapExtent = Device._SwapchainDetails.GetBestSwapExtent(ActiveWindow);

		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		uint32_t ImageCount = Device._SwapchainDetails._SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (Device._SwapchainDetails._SurfaceCapabilities.maxImageCount > 0
			&& Device._SwapchainDetails._SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = Device._SwapchainDetails._SurfaceCapabilities.maxImageCount;
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
		SwapchainCreateInfo.preTransform = Device._SwapchainDetails._SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		uint32_t Indices[] = {
			static_cast<uint32_t>(Device._PhysicalDeviceIndices._GraphicsFamily),
			static_cast<uint32_t>(Device._PhysicalDeviceIndices._PresentationFamily)
		};

		if (Device._PhysicalDeviceIndices._GraphicsFamily != Device._PhysicalDeviceIndices._PresentationFamily)
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

		const VkResult Result = vkCreateSwapchainKHR(Device._LogicalDevice, &SwapchainCreateInfo, nullptr, &_VulkanSwapchain);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
			return false;
		}

		_SwapchainImageFormat = SurfaceFormat.format;
		_SwapchainExtent = SwapExtent;

		// Get swap chain images
		uint32_t SwapchainImageCount = 0;
		vkGetSwapchainImagesKHR(Device._LogicalDevice, _VulkanSwapchain, &SwapchainImageCount, nullptr);

		std::vector<VkImage> Images(SwapchainImageCount);
		vkGetSwapchainImagesKHR(Device._LogicalDevice, _VulkanSwapchain, &SwapchainImageCount, Images.data());

		for (VkImage Image : Images)
		{
			SwapchainImage SwapImage;
			SwapImage.CreateSwapchainImage(Device._LogicalDevice, Image, _SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			_SwapchainImages.push_back(SwapImage);
		}

		return true;
	}

	void Swapchain::CleanupSwapchain(VkDevice _LogicalDevice)
	{
		for (auto Image : _SwapchainImages)
		{
			vkDestroyImageView(_LogicalDevice, Image._ImageView, nullptr);
		}

		vkDestroySwapchainKHR(_LogicalDevice, _VulkanSwapchain, nullptr);
	}

	bool Instance::CreateInstance()
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
		if (!GetRequiredInstanceExtensions(InstanceExtensions))
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
		const VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &_VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}

	bool Instance::IsInstanceExtensionSupported(const char* Extension) const
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

	bool Instance::GetRequiredInstanceExtensions(std::vector<const char*>& InstanceExtensions) const
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

		if (VulkanUtil::_EnableValidationLayers)
		{
			InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return true;
	}

	bool GraphicsPipeline::CreateGraphicsPipeline()
	{
		// TODO FIX!!!
		auto VertexShaderCode = Util::File::ReadFileFull(Util::Config::GetVertexShaderPath());
		auto FragmentShaderCode = Util::File::ReadFileFull(Util::Config::GetFragmentShaderPath());

		return true;
	}
}
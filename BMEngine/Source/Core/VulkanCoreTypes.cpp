#include "VulkanCoreTypes.h"

#include <unordered_set>
#include <algorithm>

#include "Util/Util.h"

namespace Core
{
	// Return most common format
	VkSurfaceFormatKHR MainDevice::GetBestSurfaceFormat() const
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
	VkPresentModeKHR MainDevice::GetBestPresentationMode() const
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

	VkExtent2D MainDevice::GetBestSwapExtent(GLFWwindow* Window) const
	{
		if (_SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return _SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(Window, &Width, &Height);

			Width = std::clamp(static_cast<uint32_t>(Width), _SurfaceCapabilities.minImageExtent.width, _SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), _SurfaceCapabilities.minImageExtent.height, _SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
	}

	bool MainDevice::SetupPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface)
	{
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(Instance, &DevicesCount, nullptr);

		VkPhysicalDevice* DeviceList = Util::Memory::Allocate<VkPhysicalDevice>(DevicesCount);
		vkEnumeratePhysicalDevices(Instance, &DevicesCount, DeviceList);

		for (uint32_t i = 0; i < DevicesCount; ++i)
		{
			_PhysicalDevice = DeviceList[i];

			/*
			// ID, name, type, vendor, etc
			VkPhysicalDeviceProperties Properties;
			vkGetPhysicalDeviceProperties(Device, &Properties);

			// geo shader, tess shader, wide lines, etc
			VkPhysicalDeviceFeatures Features;
			vkGetPhysicalDeviceFeatures(Device, &Features);
			*/

			uint32_t ExtensionsCount = 0;
			vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &ExtensionsCount, nullptr);

			VkExtensionProperties* AvalibleDeviceExtensions = Util::Memory::Allocate<VkExtensionProperties>(ExtensionsCount);
			vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &ExtensionsCount, AvalibleDeviceExtensions);

			for (const char* Extension : _DeviceExtensions)
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
					return false;
				}
			}

			Util::Memory::Deallocate(AvalibleDeviceExtensions);


			// TODO check function
			// In current realization if _GraphicsFamily is valid but if _PresentationFamily is not valid
			// _GraphicsFamily could be overridden on next iteration even when it is valid
			uint32_t FamilyCount = 1;
			vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &FamilyCount, nullptr);

			VkQueueFamilyProperties* FamilyProperties = Util::Memory::Allocate<VkQueueFamilyProperties>(FamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &FamilyCount, FamilyProperties);

			for (uint32_t j = 0; j < FamilyCount; ++j)
			{
				// check if Queue is graphics type
				if (FamilyProperties[j].queueCount > 0 && FamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					// if we QueueFamily[i] graphics type but not presentation type
					// and QueueFamily[i + 1] graphics type and presentation type
					// then we rewrite _GraphicsFamily
					// toto check what is better rewrite or have different QueueFamilys
					_GraphicsFamily = j;
				}

				// check if Queue is presentation type (can be graphics and presentation)
				VkBool32 PresentationSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(_PhysicalDevice, j, Surface, &PresentationSupport);
				if (FamilyProperties[j].queueCount > 0 && PresentationSupport)
				{
					_PresentationFamily = j;
				}

				if (_GraphicsFamily < 0 && _PresentationFamily < 0)
				{
					// Todo walidate warning
					Util::Log().Warning("Device does not support required indices");
					break;
				}
			}

			if (_GraphicsFamily < 0 && _PresentationFamily < 0)
			{
				_PhysicalDevice = nullptr;
				break;
			}

			Util::Memory::Deallocate(FamilyProperties);

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_PhysicalDevice, Surface, &_SurfaceCapabilities);

			uint32_t FormatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(_PhysicalDevice, Surface, &FormatCount, nullptr);
			if (FormatCount == 0)
			{
				Util::Log().Error("FormatCount is 0");
				_PhysicalDevice = nullptr;
			}

			_Formats.resize(static_cast<size_t>(FormatCount));
			vkGetPhysicalDeviceSurfaceFormatsKHR(_PhysicalDevice, Surface, &FormatCount, _Formats.data());

			uint32_t PresentationCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(_PhysicalDevice, Surface, &PresentationCount, nullptr);
			if (PresentationCount == 0)
			{
				Util::Log().Error("PresentationCount is 0");
				_PhysicalDevice = nullptr;
			}
			else
			{
				_PresentationModes.resize(static_cast<size_t>(PresentationCount));
				vkGetPhysicalDeviceSurfacePresentModesKHR(_PhysicalDevice, Surface, &PresentationCount, _PresentationModes.data());
			}

			if (_PhysicalDevice != nullptr)
			{
				Util::Memory::Deallocate(DeviceList);
				return true;
			}
		}

		Util::Log().Error("No physical devices found");

		Util::Memory::Deallocate(DeviceList);
		return false;
	}

	bool MainDevice::CreateLogicalDevice()
	{
		const float Priority = 1.0f;

		// One family can suppurt graphics and presentation
		// In that case create mulltiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = {};
		uint32_t FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(_GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (_GraphicsFamily != _PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(_PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = (FamilyIndicesSize);
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
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
}
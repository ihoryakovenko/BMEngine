#include "VulkanHelpers.h"

#include "Util/Log.h"

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
}
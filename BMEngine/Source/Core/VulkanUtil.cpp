#include <vector>
#include <algorithm>

#include "VulkanUtil.h"

#include "Util/Log.h"

namespace Core
{
	bool VulkanUtil::IsValidationLayerSupported(const char* Layer)
	{
		static const std::vector<VkLayerProperties>& AvailableLayers = []()
		{
			uint32_t LayerCount = 0;
			vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

			std::vector<VkLayerProperties> Layers(LayerCount);
			vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());

			return Layers;
		}();

		for (const auto& AvalibleLayer : AvailableLayers)
		{
			if (std::strcmp(Layer, AvalibleLayer.layerName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void VulkanUtil::GetEnabledValidationLayers(VkInstanceCreateInfo& CreateInfo)
	{
		if (_EnableValidationLayers)
		{
			std::erase_if(_ValidationLayers, [](const char* Layer)
			{
				if (!VulkanUtil::IsValidationLayerSupported(Layer))
				{
					Util::Log().Error("Validation layer {} unsupported", Layer);
					return true;
				}
				
				return false;
			});


			CreateInfo.enabledLayerCount = static_cast<uint32_t>(_ValidationLayers.size());
			CreateInfo.ppEnabledLayerNames = VulkanUtil::_ValidationLayers.data();
			return;
		}

		CreateInfo.ppEnabledLayerNames = nullptr;
		CreateInfo.enabledLayerCount = 0;
	}

	VkDebugUtilsMessengerCreateInfoEXT* VulkanUtil::GetDebugMessengerCreateInfo()
	{
		static VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = []()
		{
			VkDebugUtilsMessengerCreateInfoEXT CreateInfo;
			CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			CreateInfo.pfnUserCallback = DebugCallback;
			CreateInfo.pUserData = nullptr;

			return CreateInfo;
		}();

		return &DebugCreateInfo;
	}

	void VulkanUtil::SetupDebugMessenger(VkInstance Instance)
	{
		if (_EnableValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT* DebugCreateInfo = GetDebugMessengerCreateInfo();
			CreateDebugUtilsMessengerEXT(Instance, DebugCreateInfo, nullptr, &_DebugMessenger);
		}
	}

	void VulkanUtil::DestroyDebugMessenger(VkInstance Instance)
	{
		if (_EnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(Instance, _DebugMessenger, nullptr);
		}
	}

	void VulkanUtil::GetDebugCreateInfo(VkInstanceCreateInfo& CreateInfo)
	{
		if (_EnableValidationLayers)
		{
			CreateInfo.pNext = GetDebugMessengerCreateInfo();
		}
		else
		{
			CreateInfo.pNext = nullptr;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanUtil::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
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

	void VulkanUtil::CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger)
	{
		auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		if (CreateMessengerFunc != nullptr)
		{
			const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, DebugMessenger);
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

	void VulkanUtil::DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger,
		const VkAllocationCallbacks* Allocator)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, DebugMessenger, Allocator);
		}
		else
		{
			Util::Log().Error("DestroyMessengerFunc is nullptr");
		}
	}
}

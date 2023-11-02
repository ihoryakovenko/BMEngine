#include "VulkanCoreTypes.h"

#include "Util/Util.h"

namespace Core
{
	bool InitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data, const char** ValidationExtensions, uint32_t ValidationExtensionsSize,
		bool EnumerateInstanceLayerProperties)
	{
		// Get count part
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&Data.RequiredExtensionsCount);
		if (Data.RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		if (EnumerateInstanceLayerProperties)
		{
			Result = vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, nullptr);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
				return false;
			}
		}

		// Allocation part
		const size_t AdditionalExtensionSize = ValidationExtensionsSize * sizeof(const char**);
		const size_t RequiredExtensionsSize = Data.RequiredExtensionsCount * sizeof(const char**);
		const size_t AvalibleExtensionsSize = Data.AvalibleExtensionsCount * sizeof(VkExtensionProperties);
		const size_t AvalibleLayerSize = Data.AvalibleValidationLayersCount * sizeof(VkLayerProperties);

		Data.Buffer = Util::Memory::Allocate(RequiredExtensionsSize + AvalibleExtensionsSize + AdditionalExtensionSize + AvalibleLayerSize);
		
		// Data structuring part
		Data.RequiredAndValidationExtensions = reinterpret_cast<const char**>(Data.Buffer);
		Data.RequiredAndValidationExtensionsCount = ValidationExtensionsSize + Data.RequiredExtensionsCount;

		Data.RequiredInstanceExtensions = static_cast<const char**>(Data.Buffer) + ValidationExtensionsSize;
		Data.AvalibleExtensions = reinterpret_cast<VkExtensionProperties*>(Data.RequiredInstanceExtensions + Data.RequiredExtensionsCount);
		Data.AvalibleValidationLayers = reinterpret_cast<VkLayerProperties*>(Data.AvalibleExtensions + Data.AvalibleExtensionsCount);

		// Data assignment part
		for (uint32_t i = 0; i < ValidationExtensionsSize; ++i)
		{
			Data.RequiredAndValidationExtensions[i] = ValidationExtensions[i];
		}

		for (uint32_t i = 0; i < Data.RequiredExtensionsCount; ++i)
		{
			Data.RequiredInstanceExtensions[i] = RequiredInstanceExtensions[i];
		}

		vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, Data.AvalibleExtensions);

		vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, Data.AvalibleValidationLayers);

		return true;
	}

	void DeinitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data)
	{
		Util::Memory::Deallocate(Data.Buffer);
	}

	bool CheckRequiredInstanceExtensionsSupport(VkInstanceCreateInfoData& Data)
	{
		for (uint32_t i = 0; i < Data.RequiredAndValidationExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleExtensionsCount; ++j)
			{
				if (std::strcmp(Data.RequiredAndValidationExtensions[i], Data.AvalibleExtensions[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", Data.RequiredAndValidationExtensions[i]);
				return false;
			}
		}

		return true;
	}

	bool CheckValidationLayersSupport(VkInstanceCreateInfoData& Data, const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < Data.AvalibleValidationLayersCount; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Data.AvalibleValidationLayers[j].layerName) == 0)
				{
					IsLayerAvalible = true;
					break;
				}
			}

			if (!IsLayerAvalible)
			{
				Util::Log().Error("Validation layer {} unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}
}

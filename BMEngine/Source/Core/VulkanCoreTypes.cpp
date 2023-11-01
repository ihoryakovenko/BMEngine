#include "VulkanCoreTypes.h"

#include "Util/Util.h"

#include <iostream>

namespace Core
{
	bool InitVkInstanceCreateInfoData(VkInstanceCreateInfoData& Data)
	{
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

		const size_t AdditionalExtensionCount = 1;

		const size_t AdditionalExtensionSize = AdditionalExtensionCount * sizeof(const char*);
		const size_t RequiredExtensionsSize = Data.RequiredExtensionsCount * sizeof(const char**);
		const size_t AvalibleExtensionsSize = Data.AvalibleExtensionsCount * sizeof(VkExtensionProperties);

		Data.Buffer = Util::Memory::Allocate(RequiredExtensionsSize + AvalibleExtensionsSize + AdditionalExtensionSize);
		
		Data.RequiredAndValidationExtensions = reinterpret_cast<const char**>(Data.Buffer);
		Data.RequiredAndValidationExtensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		Data.RequiredAndValidationExtensionsCount = AdditionalExtensionCount + Data.RequiredExtensionsCount;

		Data.RequiredInstanceExtensions = static_cast<const char**>(Data.Buffer) + AdditionalExtensionCount;
		Data.AvalibleExtensions = reinterpret_cast<VkExtensionProperties*>(Data.RequiredInstanceExtensions + Data.RequiredExtensionsCount);

		for (uint32_t i = 0; i < Data.RequiredExtensionsCount; ++i)
		{
			Data.RequiredInstanceExtensions[i] = RequiredInstanceExtensions[i];
		}

		vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, Data.AvalibleExtensions);

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
}
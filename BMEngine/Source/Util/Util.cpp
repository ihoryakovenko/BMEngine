#include "Util.h"

namespace Util
{
	bool UtilHelper::ReadFileFull(FILE* File, std::vector<char>& OutFileData)
	{
		_fseeki64(File, 0LL, static_cast<int>(SEEK_END));
		const long long FileSize = _ftelli64(File);
		if (FileSize != -1LL)
		{
			rewind(File); // Put file pointer to 0 index

			OutFileData.resize(static_cast<size_t>(FileSize));
			// Need to check fread result if we know the size?
			const size_t ReadResult = fread(OutFileData.data(), 1, static_cast<size_t>(FileSize), File);
			if (ReadResult == static_cast<size_t>(FileSize))
			{
				return true;
			}

			return false;
		}

		return false;
	}

	bool UtilHelper::OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode)
	{
		FILE* File = nullptr;
		const int Result = fopen_s(&File, FileName, Mode);
		if (Result == 0)
		{
			if (ReadFileFull(File, OutFileData))
			{
				fclose(File);
				return true;
			}

			Log().Error("Failed to read file {}: ", FileName);
			fclose(File);
			return false;
		}

		Log().Error("Cannot open file {}: Result = {}", FileName, Result);
		return false;
	}

	bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger)
	{
		auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		if (CreateMessengerFunc != nullptr)
		{
			const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, InDebugMessenger);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("CreateMessengerFunc result is {}", static_cast<int>(Result));
				return false;
			}
		}
		else
		{
			Util::Log().Error("CreateMessengerFunc is nullptr");
			return false;
		}

		return true;
	}

	bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, InDebugMessenger, Allocator);
			return true;
		}
		else
		{
			Util::Log().Error("DestroyMessengerFunc is nullptr");
			return false;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
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
}

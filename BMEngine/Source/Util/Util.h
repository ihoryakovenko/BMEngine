#pragma once

#include <string>
#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Util
{
	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData);
	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode);

	struct Log
	{
		template <typename... TArgs>
		static void Error(std::string_view Message, TArgs&&... Args)
		{
			std::cout << "\033[31;5m"; // Set red color
			Print("Error", Message, Args...);
			std::cout << "\033[m"; // Reset red color
		}

		template <typename... TArgs>
		static void Warning(std::string_view Message, TArgs&&... Args)
		{
			std::cout << "\033[33;5m";
			Print("Warning", Message, Args...);
			std::cout << "\033[m";
		}

		template <typename... TArgs>
		static void Info(std::string_view Message, TArgs&&... Args)
		{
			Print("Info", Message, Args...);
		}

		static void GlfwLogError()
		{
			const char* ErrorDescription;
			const int LastErrorCode = glfwGetError(&ErrorDescription);

			const std::string_view ErrorLog = LastErrorCode != GLFW_NO_ERROR ? ErrorDescription : "No GLFW error";
			Error(ErrorLog);
		}

		template <typename... TArgs>
		static void Print(std::string_view Type, std::string_view Message, TArgs&&... Args)
		{
			//std::cout << std::format("{}: {}\n", Type, std::vformat(Message, std::make_format_args(Args...)));
		}
	};

#ifdef NDEBUG
	static bool EnableValidationLayers = false;
#else
	static bool EnableValidationLayers = true;
#endif
}
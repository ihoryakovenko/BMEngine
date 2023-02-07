#pragma once

#include <string>
#include <iostream>
#include <format>
#include <source_location>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Util
{
	class Log
	{
	public:
		Log(std::source_location&& Location = std::source_location::current()) :
			_Location{ std::move(Location) }
		{
		}

		template <typename... TArgs>
		void Error(std::string_view Message, TArgs&&... Args) const
		{
			std::cout << "\033[31;5m"; // Set red color
			Print("Error", Message, Args...);
			std::cout << "\033[m"; // Reset red color
		}

		template <typename... TArgs>
		void Info(std::string_view Message, TArgs&&... Args) const
		{
			Print("Info", Message, Args...);
		}

		void GlfwLogError() const
		{
			const char* ErrorDescription;
			const int LastErrorCode = glfwGetError(&ErrorDescription);

			const std::string_view ErrorLog = LastErrorCode != GLFW_NO_ERROR ? ErrorDescription : "No GLFW error";
			Error(ErrorLog);
		}

	private:
		template <typename... TArgs>
		void Print(std::string_view Type, std::string_view Message, TArgs&&... Args) const
		{
			std::cout << std::format("{}, line: {}, function: {}, {}: {}\n", _Location.file_name(),
				_Location.line(), _Location.function_name(), Type, std::vformat(Message, std::make_format_args(Args...)));
		}

	private:
		std::source_location _Location;
	};
}

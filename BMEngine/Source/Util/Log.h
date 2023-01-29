#pragma once

#include <string>
#include <iostream>
#include <format>

namespace Util
{
	void GlfwLogError();

	template <typename... TArgs>
	void ErrorLog(const std::string& Message, TArgs&&... Args)
	{
		const auto& test = std::vformat(Message, std::make_format_args(Args...));
		std::cout << test << std::endl;
	}
}

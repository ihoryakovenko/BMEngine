#pragma once

#include <vector>
#include <string>
#include <memory>

namespace Util
{
	struct File
	{
		static std::vector<char> ReadFileFull(std::string_view FileName);
	};
}

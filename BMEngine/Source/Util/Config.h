#pragma once

#include <string>

namespace Util
{
	// TODO FIX!!!
	struct Config
	{
		static std::string_view GetVertexShaderPath()
		{
			return "./Resources/Shaders/Shader.vert";
		}

		static std::string_view GetFragmentShaderPath()
		{
			return "./Resources/Shaders/Shader.frag";
		}
	};
}

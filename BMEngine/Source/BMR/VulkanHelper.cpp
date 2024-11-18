#include "VulkanHelper.h"

#include <cassert>
#include <stdarg.h>

namespace BMR
{
	static BMRLogHandler LogHandler = nullptr;

	void SetLogHandler(BMRLogHandler Handler)
	{
		LogHandler = Handler;
	}

	void HandleLog(BMRLogType LogType, const char* Format, ...)
	{
		if (LogHandler != nullptr)
		{
			va_list Args;
			va_start(Args, Format);
			LogHandler(LogType, Format, Args);
			va_end(Args);
		}
	}

	bool CreateShader(VkDevice LogicalDevice, const u32* Code, u32 CodeSize,
		VkShaderModule &VertexShaderModule)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = CodeSize;
		ShaderModuleCreateInfo.pCode = Code;

		VkResult Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateShaderModule result is %d", Result);
			return false;
		}

		return true;
	}
}
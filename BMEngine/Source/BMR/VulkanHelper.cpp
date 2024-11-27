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
}
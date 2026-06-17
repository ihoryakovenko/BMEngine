#include "RenderInterface.h"

#include <SharedLib.h>

#include <cstdarg>
#include <stdio.h>
#include <cassert>

#include "RenderInternal.h"

Memory_LinearAllocator FrameMemory;

void BmRender_Init(const BmRender_InitData* InitData)
{
	Memory_LinearAllocator_Init(&FrameMemory, 1024 * 1024);
	InitBackend(InitData->WindowHandler);
}

void BmRender_DeInit()
{
	DeInitBackend();
	Memory_LinearAllocator_Free(&FrameMemory);
}

void BmRender_FrameFree()
{
	Memory_LinearAllocator_FreeMemory(&FrameMemory);
}

void RenderLog(LogType LogType, const char* Format, va_list Args);

void RenderLog(LogType logType, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	RenderLog(logType, format, args);
	va_end(args);
}

void RenderLog(LogType LogType, const char* Format, va_list Args)
{
	switch (LogType)
	{
	case LogType::Error:
	{
		vprintf("\033[31;5mError: ", Args);
		va_list ArgsCopy;
		va_copy(ArgsCopy, Args);
		vprintf(Format, ArgsCopy);
		va_end(ArgsCopy);
		vprintf("\n\033[m", Args);
		assert(false);
		break;
	}
	case LogType::Warning:
	{
		vprintf("\033[33;5mWarning: ", Args);
		va_list ArgsCopy;
		va_copy(ArgsCopy, Args);
		vprintf(Format, ArgsCopy);
		va_end(ArgsCopy);
		vprintf("\n\033[m", Args);
		break;
	}
	case LogType::Info:
	{
		vprintf("Info: ", Args);
		va_list ArgsCopy;
		va_copy(ArgsCopy, Args);
		vprintf(Format, ArgsCopy);
		va_end(ArgsCopy);
		vprintf("\n", Args);
		break;
	}
	}
}

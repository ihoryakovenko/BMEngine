#pragma once

enum class LogType
{
	Error,
	Warning,
	Info
};

void RenderLog(LogType logType, const char* format, ...);

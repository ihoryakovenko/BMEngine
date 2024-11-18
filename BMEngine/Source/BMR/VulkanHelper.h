#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"
#include "BMR/BMRInterfaceTypes.h"

namespace BMR
{
	void SetLogHandler(BMRLogHandler Handler);

	void HandleLog(BMRLogType LogType, const char* Format, ...);



	

	
}

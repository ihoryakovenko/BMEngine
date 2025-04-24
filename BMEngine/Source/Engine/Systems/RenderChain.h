#pragma once

#include "Util/EngineTypes.h"

namespace RenderChain
{
	typedef void (*DrawFunction)(void);

	void RegisterDraw(DrawFunction Function);
	void StartFrame();
}
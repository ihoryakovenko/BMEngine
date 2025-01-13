#pragma once

#include <cassert>

#include <glm/glm.hpp>

#include "EngineTypes.h"

namespace Math
{
	static const u32 MaxZoom = 16;
	static const u32 MinZoom = 0;

	static u32 GetTilesPerAxis(u32 Zoom)
	{
		assert(Zoom <= 16);
		return 2 << glm::clamp(Zoom - 1u, MinZoom, MaxZoom);
	}
}
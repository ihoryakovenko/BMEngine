#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Util/EngineTypes.h"
#include "BMR/BMRInterface.h"

namespace DynamicMapSystem
{
	void Init();
	void DeInit();

	void Update(const glm::vec3& CameraPosition, f32 FovHorizontal, f32 AspectRatio);
	void SetVertexZoom(s32 Zoom, s32 InLatMin, s32 InLatMax, s32 InLonMin, s32 InLonMax);
	void SetTileZoom(s32 Zoom);
}
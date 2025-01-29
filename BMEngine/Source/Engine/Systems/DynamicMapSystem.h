#pragma once

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

namespace DynamicMapSystem
{
	struct MapCamera
	{
		f32 Fov;
		f32 AspectRatio;
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
	};

	void Init();
	void DeInit();

	void Update(f64 DeltaTime, const MapCamera& Camera, s32 Zoom);

	glm::vec3 SphericalToMercator(const glm::vec3& Position);
	f64 CalculateCameraAltitude(s32 ZoomLevel);

	void TestSetDownload(bool Download);
}
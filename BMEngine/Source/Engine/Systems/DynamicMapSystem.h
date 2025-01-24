#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Util/EngineTypes.h"
#include "Render/Render.h"

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
	void OnDraw();

	glm::vec3 SphericalToMercator(const glm::vec3& Position);
	f64 CalculateCameraAltitude(s32 ZoomLevel);

	void TestSetDownload(bool Download);
}
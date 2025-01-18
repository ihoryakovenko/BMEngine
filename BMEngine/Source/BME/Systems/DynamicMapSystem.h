#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Util/EngineTypes.h"
#include "BMR/BMRInterface.h"

namespace DynamicMapSystem
{
	struct MapCamera
	{
		float Fov;            // Vertical field of view in degrees
		float AspectRatio;    // Width / Height of the viewport
		glm::vec3 Position;   // Camera position (assumes sphere center at (0, 0, 0))
		glm::vec3 front;      // Camera front vector
		glm::vec3 up;         // Camera up vector
	};

	void Init();
	void DeInit();

	void Update(const MapCamera& Camera, s32 Zoom);

	glm::vec3 SphericalToMercator(const glm::vec3& Position);
	f64 CalculateCameraAltitude(s32 ZoomLevel);
}
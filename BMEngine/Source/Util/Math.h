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

	static glm::vec3 SphericalToCartesian(const glm::vec3& Position)
	{
		//const f32 Latitude = glm::half_pi<f32>() * Position.x; // Map [-1, 1] -> [-p/2, p/2]

		// Map[-1, 1] -> Mercator latitude in radians
		const f32 Latitude = glm::atan(glm::sinh(glm::pi<f32>() * Position.x)); 
		const f32 Longitude = glm::pi<f32>() * Position.y; // Map [-1, 1] -> [-p, p]
		const f32 Altitude = Position.z;

		const f32 CosLatitude = glm::cos(Latitude);
		const f32 SinLatitude = glm::sin(Latitude);

		glm::vec3 CartesianPos;
		CartesianPos.x = Altitude * CosLatitude * glm::sin(Longitude);
		CartesianPos.y = Altitude * SinLatitude;
		CartesianPos.z = Altitude * CosLatitude * glm::cos(Longitude);

		return CartesianPos;
	}
}
#pragma once

#include <cassert>

#include <glm/glm.hpp>

#include "EngineTypes.h"

namespace Math
{
	static const u32 MaxZoom = 16;
	static const u32 MinZoom = 0;

	static const f64 Circumference = 2 * glm::pi<f32>();

	static u32 GetTilesPerAxis(u32 Zoom)
	{
		assert(Zoom <= 16);
		return 2 << glm::clamp(Zoom - 1u, MinZoom, MaxZoom);
	}

	static glm::vec3 SphericalToMercator(const glm::vec3& Position)
	{
		//const f32 Latitude = glm::half_pi<f32>() * Position.x; // Map [-1, 1] -> [-p/2, p/2]

		// Map[-1, 1] -> Mercator latitude in radians [-p/2, p/2]
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

	static bool RaySphereIntersection(const glm::vec3& Origin, const glm::vec3& Direction, f32 Radius, glm::vec3& Intersection)
	{
		f32 b = 2.0f * glm::dot(Direction, Origin);
		f32 c = glm::dot(Origin, Origin) - Radius * Radius;
		f32 Discriminant = b * b - 4.0f * c;

		if (Discriminant < 0) return false;

		f32 t = (-b - std::sqrt(Discriminant)) / 2.0f;
		if (t < 0) t = (-b + std::sqrt(Discriminant)) / 2.0f;
		if (t < 0) return false;

		Intersection = Origin + t * Direction;
		return true;
	}

	static f32 CalculateCameraAltitude(s32 ZoomLevel)
	{
		return Circumference / glm::pow(2, ZoomLevel - 1);
	}
}
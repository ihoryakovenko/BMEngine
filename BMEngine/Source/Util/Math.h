#pragma once

#include <cassert>

#include <glm/glm.hpp>

#include "EngineTypes.h"

namespace Math
{
	static const f64 Circumference = 2 * glm::pi<f64>();

	static bool RaySphereIntersection(const glm::vec3& Origin, const glm::vec3& Direction, f32 Radius, glm::vec3& Intersection)
	{
		const f32 b = 2.0f * glm::dot(Direction, Origin);
		const f32 c = glm::dot(Origin, Origin) - Radius * Radius;
		const f32 Discriminant = b * b - 4.0f * c;

		if (Discriminant < 0) return false;

		f32 t = (-b - std::sqrt(Discriminant)) / 2.0f;
		if (t < 0) t = (-b + std::sqrt(Discriminant)) / 2.0f;
		if (t < 0) return false;

		Intersection = Origin + t * Direction;
		return true;
	}
}
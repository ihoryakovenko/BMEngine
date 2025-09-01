#pragma once

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "EngineTypes.h"

namespace Math
{
	template <typename T>
	constexpr T Circumference = static_cast<T>(2) * glm::pi<T>();

	template <typename T>
	bool RaySphereIntersection(const glm::vec<3, T>& Origin, const glm::vec<3, T>& Direction, T Radius, glm::vec<3, T>& Intersection)
	{
		const T b = static_cast<T>(2) * glm::dot(Direction, Origin);
		const T c = glm::dot(Origin, Origin) - Radius * Radius;
		const T Discriminant = b * b - static_cast<T>(4) * c;

		if (Discriminant < static_cast<T>(0)) return false;

		T t = (-b - std::sqrt(Discriminant)) / static_cast<T>(2);
		if (t < static_cast<T>(0)) t = (-b + std::sqrt(Discriminant)) / static_cast<T>(2);
		if (t < static_cast<T>(0)) return false;

		Intersection = Origin + t * Direction;
		return true;
	}

	template<typename T>
	static T AlignNumber(T Number, T Alignment)
	{
		return Number + (Alignment - 1) & ~(Alignment - 1);
	}

	template <typename T>
	static T WrapIncrement(T Number, T Max)
	{
		return (Number + 1) % Max;
	}
}
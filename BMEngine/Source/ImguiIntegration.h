#pragma once

#include <glm/glm.hpp>

namespace ImguiIntegration
{
	typedef void (*ZoomChanged)(int Zoom, int InLatMin, int InLatMax, int InLonMin, int InLonMax);

	struct GuiData
	{
		glm::vec3* Eye = nullptr;
		glm::vec3* DirectionLightDirection = nullptr;
		ZoomChanged OnZoomChanged = nullptr;
	};

	void DrawLoop(const bool& IsDrawing, GuiData Data);
}

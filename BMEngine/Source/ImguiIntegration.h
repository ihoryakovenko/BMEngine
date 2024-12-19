#pragma once

#include <glm/glm.hpp>

namespace ImguiIntegration
{
	struct GuiData
	{
		glm::vec3* Eye = nullptr;
		glm::vec3* DirectionLightDirection = nullptr;
	};

	void DrawLoop(const bool& IsDrawing, GuiData Data);
}

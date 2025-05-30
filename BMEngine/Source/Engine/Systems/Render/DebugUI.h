#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

struct GLFWwindow;

namespace DebugUi
{
	typedef void (*TestSetDownload)(bool Download);

	struct GuiData
	{
		glm::vec3* Eye = nullptr;
		glm::vec3* DirectionLightDirection = nullptr;

		TestSetDownload OnTestSetDownload = nullptr;
		glm::vec3* CameraMercatorPosition = nullptr;
		int* Zoom = nullptr;
	};

	void Init(GLFWwindow* Wnd, GuiData* Data);
	void DeInit();

	void Draw();
}
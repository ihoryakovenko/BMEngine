#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

namespace UI
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

	void Init(GuiData* Data);
	void DeInit();
	void Update();
}
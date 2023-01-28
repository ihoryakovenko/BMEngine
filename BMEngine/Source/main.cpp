//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#include <iostream>

#include "Core/Engine.h"

int main()
{
	Core::Engine Engine;
	if (Engine.Init())
	{
		Engine.GameLoop();
		Engine.Terminate();
	}

	return 0;
}
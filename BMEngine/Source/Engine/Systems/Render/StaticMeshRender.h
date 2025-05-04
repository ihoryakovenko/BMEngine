#pragma once

#include "Util/EngineTypes.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>
#include "Render/Render.h"

namespace StaticMeshRender
{
	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	void Init();
	void DeInit();

	void Draw();
}
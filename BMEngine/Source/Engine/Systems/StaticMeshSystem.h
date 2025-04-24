#pragma once

#include "Util/EngineTypes.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>
#include "Render/Render.h"

namespace StaticMeshSystem
{
	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	void Init();
	void DeInit();

	struct Material
	{
		f32 Shininess;
	};

	void UpdateMaterialBuffer(const Material* Buffer);
	void AttachTextureToStaticMesh(VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach);
}
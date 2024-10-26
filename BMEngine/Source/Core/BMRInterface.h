#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // TODO use something instead of GLFWwindow

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	bool Init(GLFWwindow* Window, const BMRConfig& InConfig);
	void DeInit();

	u32 LoadTexture(BMRTextureArrayInfo Info);
	u32 LoadMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, VkDeviceSize VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void UpdateLightBuffer(const BMRLightBuffer& Buffer);
	void UpdateMaterialBuffer(const BMRMaterial& Buffer);

	void Draw(const BMRDrawScene& Scene);
}
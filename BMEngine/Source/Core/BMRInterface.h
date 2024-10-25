#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // TODO use something instead of GLFWwindow

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

extern "C"
{
	bool BMRInit(GLFWwindow* Window, const BMRConfig& InConfig);
	void BMRDeInit();

	u32 BMRLoadTexture(BMRTextureArrayInfo Info);
	u32 BMRLoadMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);
	u64 BMRLoadVertices(const void* Vertices, u32 VertexSize, VkDeviceSize VerticesCount);
	u64 BMRLoadIndices(const u32* Indices, u32 IndicesCount);

	void BMRUpdateLightBuffer(const BMRLightBuffer& Buffer);
	void BMRUpdateMaterialBuffer(const BMRMaterial& Buffer);

	void BMRDraw(const BMRDrawScene& Scene);
}
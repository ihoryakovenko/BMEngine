#pragma once

#include <vulkan/vulkan.h>
#include <Windows.h>

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	bool Init(HWND WindowHandler, const BMRConfig& InConfig);
	void DeInit();

	u32 LoadTexture(BMRTextureArrayInfo Info, BMRTextureType TextureType);
	u32 LoadEntityMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);
	u32 LoadTerrainMaterial(u32 DiffuseTextureIndex);
	u32 LoadSkyBoxMaterial(u32 CubeTextureIndex);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, VkDeviceSize VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void UpdateMaterialBuffer(const BMRMaterial* Buffer);

	void Draw(const BMRDrawScene& Scene);
}
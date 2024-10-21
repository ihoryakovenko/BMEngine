#pragma once

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace Core::VulkanRenderingSystemInterface
{
	bool Init(GLFWwindow* Window, const RenderConfig& InConfig);
	void DeInit();

	u32 CreateTexture(TextureArrayInfo Info);
	u32 CreateMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);

	void CreateDrawEntities(Mesh* Meshes, u32 MeshesCount, DrawEntity* OutEntities);

	void CreateTerrainIndices(u32* Indices, u32 IndicesCount);
	void CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, u32 TerrainVerticesCount, DrawTerrainEntity& OutTerrain);

	void Draw(const DrawScene& Scene);
}
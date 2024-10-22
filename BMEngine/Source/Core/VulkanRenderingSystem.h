#pragma once

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace Core::VulkanRenderingSystemInterface
{
	bool Init(GLFWwindow* Window, const BrConfig& InConfig);
	void DeInit();

	u32 CreateTexture(BrTextureArrayInfo Info);
	u32 CreateMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);

	void CreateDrawEntities(BrMesh* Meshes, u32 MeshesCount, BrDrawEntity* OutEntities);

	void CreateTerrainIndices(u32* Indices, u32 IndicesCount);
	void CreateTerrainDrawEntity(BrTerrainVertex* TerrainVertices, u32 TerrainVerticesCount, BrDrawTerrainEntity& OutTerrain);

	void UpdateLightBuffer(const BrLightBuffer& Buffer);
	void UpdateMaterialBuffer(const BrMaterial& Buffer);

	void Draw(const BrDrawScene& Scene);
}
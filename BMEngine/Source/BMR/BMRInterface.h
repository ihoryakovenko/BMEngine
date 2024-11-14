#pragma once

#include <Windows.h>

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	bool Init(HWND WindowHandler, const BMRConfig& InConfig);
	void DeInit();

	BMRUniformBuffer CreateUniformBuffer(BMRUniformBufferType Type, u64 Size);
	void DestroyUniformBuffer(BMRUniformBuffer Buffer);
	void UpdateUniformBuffer(BMRUniformBuffer Buffer, u64 DataSize, u64 Offset, const void* Data);

	BMRUniformLayout CreateUniformLayout(const BMRUniformBufferType* Types, const u32* Stages, u32 Count);

	void CreateUniformSets(const BMRUniformLayout* Layouts, u32 Count, BMRUniformSet* OutSets);
	void DestroyUniformLayout(BMRUniformLayout Layout);
	void AttachBuffersToSet(BMRUniformSet Set, const BMRUniformBuffer* Buffers, const u32* BuffersSizes, u32 BufferCount);

	u32 LoadTexture(BMRTextureArrayInfo Info, BMRTextureType TextureType);
	u32 LoadEntityMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);
	u32 LoadTerrainMaterial(u32 DiffuseTextureIndex);
	u32 LoadSkyBoxMaterial(u32 CubeTextureIndex);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void UpdateMaterialBuffer(const BMRMaterial* Buffer);

	void Draw(const BMRDrawScene& Scene);
}
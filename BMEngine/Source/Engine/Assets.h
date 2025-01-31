#pragma once

#include "Util/EngineTypes.h"

namespace Assets
{
	struct Model3DFileHeader
	{
		u64 VerticesCount;
		u32 MeshCount;
		u32 IndicesCount;
		u32 DiffuseTexturesStringSize;
	};

	struct Model3D
	{
		void* VertexData;
		u64* VertexOffsets;
		u32* IndexData;
		u32* IndexOffsets;
		u32* IndicesCounts;
		char* DiffuseTextures;
		Model3DFileHeader Header;
	};

	Model3D LoadModel3D(const char* FilePath);
	void DestroyModel3D(Model3D* Model);
}
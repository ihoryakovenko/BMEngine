#include "Assets.h"


#include "Systems/StaticMeshSystem.h"
#include "Memory/MemoryManagmentSystem.h"

//#include <Windows.h>

namespace Assets
{
	Model3D LoadModel3D(const char* FilePath)
	{
		Model3D Model;

		HANDLE hFile = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ,NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			assert(false);
		}

		BOOL Result = 1;
		DWORD BytesRead;

		Result &= ReadFile(hFile, &Model.Header, sizeof(Model.Header), &BytesRead, NULL);

		Model.VertexData = Memory::BmMemoryManagementSystem::Allocate<StaticMeshSystem::StaticMeshVertex>(Model.Header.VerticesCount);
		Model.VertexOffsets = Memory::BmMemoryManagementSystem::Allocate<u64>(Model.Header.MeshCount);
		Model.IndexData = Memory::BmMemoryManagementSystem::Allocate<u32>(Model.Header.IndicesCount);
		Model.IndexOffsets = Memory::BmMemoryManagementSystem::Allocate<u32>(Model.Header.MeshCount);
		Model.IndicesCounts = Memory::BmMemoryManagementSystem::Allocate<u32>(Model.Header.MeshCount);
		Model.DiffuseTextures = Memory::BmMemoryManagementSystem::Allocate<char>(Model.Header.DiffuseTexturesStringSize);

		Result &= ReadFile(hFile, Model.VertexData, Model.Header.VerticesCount * sizeof(StaticMeshSystem::StaticMeshVertex), &BytesRead, NULL);
		Result &= ReadFile(hFile, Model.VertexOffsets, Model.Header.MeshCount * sizeof(u64), &BytesRead, NULL);
		Result &= ReadFile(hFile, Model.IndexData, Model.Header.IndicesCount * sizeof(u32), &BytesRead, NULL);
		Result &= ReadFile(hFile, Model.IndexOffsets, Model.Header.MeshCount * sizeof(u32), &BytesRead, NULL);
		Result &= ReadFile(hFile, Model.IndicesCounts, Model.Header.MeshCount * sizeof(u32), &BytesRead, NULL);
		Result &= ReadFile(hFile, Model.DiffuseTextures, Model.Header.DiffuseTexturesStringSize, &BytesRead, NULL);

		CloseHandle(hFile);
		assert(Result);

		return Model;
	}

	void DestroyModel3D(Model3D* Model)
	{
		Memory::BmMemoryManagementSystem::Free(Model->VertexData);
		Memory::BmMemoryManagementSystem::Free(Model->VertexOffsets);
		Memory::BmMemoryManagementSystem::Free(Model->IndexData);
		Memory::BmMemoryManagementSystem::Free(Model->IndexOffsets);
		Memory::BmMemoryManagementSystem::Free(Model->IndicesCounts);
		Memory::BmMemoryManagementSystem::Free(Model->DiffuseTextures);
	}
}

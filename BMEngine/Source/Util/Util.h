#pragma once

#include <string>
#include <source_location>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <gli/gli.hpp>

#include "Engine/Systems/Render/RenderResources.h"
#include <RenderInterface.h>

namespace Util
{
	

	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData);
	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode);


	
	VkFormat GliFormatToVkFormat(gli::format Format);
	u32 GetAttributeTypeSize(BmRender_AttributeType Attribute);

	typedef u8* Model3DData;

	struct Model3DMaterial
	{
		u64 DiffuseTextureHash;
		u64 SpecularTextureHash;
	};

	struct Model3DFileHeader
	{
		u64 VertexDataSize;
		u32 MeshCount;
		u32 MaterialCount;
		u32 UniqueTextureCount;
	};

	struct Model3D
	{
		Model3DFileHeader Header;

		u8* VertexData;
		u64* VerticesCounts;
		u32* IndicesCounts;
		u32* MaterialIndices;
		Model3DMaterial* Materials;
		u64* UniqueTextureHashes;
	};

	void ObjToModel3D(const char* FilePath, const char* OutputPath);

	Model3DData LoadModel3DData(const char* FilePath);
	void ClearModel3DData(Model3DData Data);

	Model3D ParseModel3D(Model3DData Data);
}
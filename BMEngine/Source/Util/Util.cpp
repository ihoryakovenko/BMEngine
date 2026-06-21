#include "Util.h"

#include <mini-yaml/yaml/Yaml.hpp>
#include <vector>
#include <algorithm>

#include "EngineTypes.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include <Engine/Systems/Render/Shaders/ShaderTypes.h>
#include <forge_memory_debugger.h>

#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Engine/Systems/EngineResources.h"
#include "gli/gli.hpp"



// Extern declarations for global resource maps
extern std::unordered_map<std::string, BmRender_Sampler> Samplers;
extern std::unordered_map<std::string, BmRender_Shader> Shaders;
extern std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
extern std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;

#include <unordered_map>

#include <filesystem>
#include <string>
#include <cstdarg>
#include <unordered_set>
#include <fstream>
#include <iostream>

namespace Util
{
	bool ReadFileFull(FILE* File, std::vector<char>& OutFileData)
	{
		if (fseek(File, 0L, SEEK_END) != 0)
		{
			return false;
		}

		long FileSize = ftell(File);
		if (FileSize < 0)
		{
			return false;
		}

		rewind(File);

		OutFileData.resize(static_cast<size_t>(FileSize));
		size_t ReadResult = fread(OutFileData.data(), 1, static_cast<size_t>(FileSize), File);

		return ReadResult == static_cast<size_t>(FileSize);
	}

	bool OpenAndReadFileFull(const char* FileName, std::vector<char>& OutFileData, const char* Mode)
	{
		FILE* File = fopen(FileName, Mode);
		if (File)
		{
			if (ReadFileFull(File, OutFileData))
			{
				fclose(File);
				return true;
			}

			fclose(File);
			return false;
		}

		return false;
	}

	Model3DData LoadModel3DData(const char* FilePath)
	{
		namespace fs = std::filesystem;

		fs::path path(FilePath);
		std::error_code ec;
		std::uintmax_t fileSize = fs::file_size(path, ec);
		if (ec)
		{
			assert(false);
		}

		std::ifstream file(FilePath, std::ios::binary);
		if (!file)
		{
			assert(false);
		}

		Model3DData Data = (Model3DData)malloc(fileSize * sizeof(u8));
		file.read(reinterpret_cast<char*>(Data), fileSize);
		if (!file)
		{
			assert(false);
		}

		return Data;
	}

	void ClearModel3DData(Model3DData Data)
	{
		free(Data);
	}

	Model3D ParseModel3D(Model3DData Data)
	{
		Model3D Model;

		memcpy(&Model.Header, Data, sizeof(Model3DFileHeader));
		Data += sizeof(Model3DFileHeader);

		Model.VertexData = Data;
		Data += Model.Header.VertexDataSize;

		Model.VerticesCounts = (u64*)Data;
		Data += Model.Header.MeshCount * sizeof(u64);

		Model.IndicesCounts = (u32*)Data;
		Data += Model.Header.MeshCount * sizeof(u32);

		Model.MaterialIndices = (u32*)Data;
		Data += Model.Header.MeshCount * sizeof(u32);

		Model.Materials = (Model3DMaterial*)Data;
		Data += Model.Header.MaterialCount * sizeof(Model3DMaterial);

		Model.UniqueTextureHashes = (u64*)Data;

		return Model;
	}

	BmRender_Format GliFormatToVkFormat(gli::format Format)
	{
		switch (Format)
		{
			// 8-bit formats
			case gli::FORMAT_R8_UNORM_PACK8: return BmRender_Format::R8_UNORM;
			case gli::FORMAT_R8_SNORM_PACK8: return BmRender_Format::R8_SNORM;
			case gli::FORMAT_R8_USCALED_PACK8: return BmRender_Format::R8_USCALED;
			case gli::FORMAT_R8_SSCALED_PACK8: return BmRender_Format::R8_SSCALED;
			case gli::FORMAT_R8_UINT_PACK8: return BmRender_Format::R8_UINT;
			case gli::FORMAT_R8_SINT_PACK8: return BmRender_Format::R8_SINT;
			case gli::FORMAT_R8_SRGB_PACK8: return BmRender_Format::R8_SRGB;

				// 16-bit formats
			case gli::FORMAT_RG8_UNORM_PACK8: return BmRender_Format::R8G8_UNORM;
			case gli::FORMAT_RG8_SNORM_PACK8: return BmRender_Format::R8G8_SNORM;
			case gli::FORMAT_RG8_USCALED_PACK8: return BmRender_Format::R8G8_USCALED;
			case gli::FORMAT_RG8_SSCALED_PACK8: return BmRender_Format::R8G8_SSCALED;
			case gli::FORMAT_RG8_UINT_PACK8: return BmRender_Format::R8G8_UINT;
			case gli::FORMAT_RG8_SINT_PACK8: return BmRender_Format::R8G8_SINT;
			case gli::FORMAT_RG8_SRGB_PACK8: return BmRender_Format::R8G8_SRGB;

				// 24-bit formats
			case gli::FORMAT_RGB8_UNORM_PACK8: return BmRender_Format::R8G8B8_UNORM;
			case gli::FORMAT_RGB8_SNORM_PACK8: return BmRender_Format::R8G8B8_SNORM;
			case gli::FORMAT_RGB8_USCALED_PACK8: return BmRender_Format::R8G8B8_USCALED;
			case gli::FORMAT_RGB8_SSCALED_PACK8: return BmRender_Format::R8G8B8_SSCALED;
			case gli::FORMAT_RGB8_UINT_PACK8: return BmRender_Format::R8G8B8_UINT;
			case gli::FORMAT_RGB8_SINT_PACK8: return BmRender_Format::R8G8B8_SINT;
			case gli::FORMAT_RGB8_SRGB_PACK8: return BmRender_Format::R8G8B8_SRGB;

			case gli::FORMAT_BGR8_UNORM_PACK8: return BmRender_Format::B8G8R8_UNORM;
			case gli::FORMAT_BGR8_SNORM_PACK8: return BmRender_Format::B8G8R8_SNORM;
			case gli::FORMAT_BGR8_USCALED_PACK8: return BmRender_Format::B8G8R8_USCALED;
			case gli::FORMAT_BGR8_SSCALED_PACK8: return BmRender_Format::B8G8R8_SSCALED;
			case gli::FORMAT_BGR8_UINT_PACK8: return BmRender_Format::B8G8R8_UINT;
			case gli::FORMAT_BGR8_SINT_PACK8: return BmRender_Format::B8G8R8_SINT;
			case gli::FORMAT_BGR8_SRGB_PACK8: return BmRender_Format::B8G8R8_SRGB;

				// 32-bit formats
			case gli::FORMAT_RGBA8_UNORM_PACK8: return BmRender_Format::R8G8B8A8_UNORM;
			case gli::FORMAT_RGBA8_SNORM_PACK8: return BmRender_Format::R8G8B8A8_SNORM;
			case gli::FORMAT_RGBA8_USCALED_PACK8: return BmRender_Format::R8G8B8A8_USCALED;
			case gli::FORMAT_RGBA8_SSCALED_PACK8: return BmRender_Format::R8G8B8A8_SSCALED;
			case gli::FORMAT_RGBA8_UINT_PACK8: return BmRender_Format::R8G8B8A8_UINT;
			case gli::FORMAT_RGBA8_SINT_PACK8: return BmRender_Format::R8G8B8A8_SINT;
			case gli::FORMAT_RGBA8_SRGB_PACK8: return BmRender_Format::R8G8B8A8_SRGB;

			case gli::FORMAT_BGRA8_UNORM_PACK8: return BmRender_Format::B8G8R8A8_UNORM;
			case gli::FORMAT_BGRA8_SNORM_PACK8: return BmRender_Format::B8G8R8A8_SNORM;
			case gli::FORMAT_BGRA8_USCALED_PACK8: return BmRender_Format::B8G8R8A8_USCALED;
			case gli::FORMAT_BGRA8_SSCALED_PACK8: return BmRender_Format::B8G8R8A8_SSCALED;
			case gli::FORMAT_BGRA8_UINT_PACK8: return BmRender_Format::B8G8R8A8_UINT;
			case gli::FORMAT_BGRA8_SINT_PACK8: return BmRender_Format::B8G8R8A8_SINT;
			case gli::FORMAT_BGRA8_SRGB_PACK8: return BmRender_Format::B8G8R8A8_SRGB;

				// 16-bit per component formats
			case gli::FORMAT_R16_UNORM_PACK16: return BmRender_Format::R16_UNORM;
			case gli::FORMAT_R16_SNORM_PACK16: return BmRender_Format::R16_SNORM;
			case gli::FORMAT_R16_USCALED_PACK16: return BmRender_Format::R16_USCALED;
			case gli::FORMAT_R16_SSCALED_PACK16: return BmRender_Format::R16_SSCALED;
			case gli::FORMAT_R16_UINT_PACK16: return BmRender_Format::R16_UINT;
			case gli::FORMAT_R16_SINT_PACK16: return BmRender_Format::R16_SINT;
			case gli::FORMAT_R16_SFLOAT_PACK16: return BmRender_Format::R16_SFLOAT;

			case gli::FORMAT_RG16_UNORM_PACK16: return BmRender_Format::R16G16_UNORM;
			case gli::FORMAT_RG16_SNORM_PACK16: return BmRender_Format::R16G16_SNORM;
			case gli::FORMAT_RG16_USCALED_PACK16: return BmRender_Format::R16G16_USCALED;
			case gli::FORMAT_RG16_SSCALED_PACK16: return BmRender_Format::R16G16_SSCALED;
			case gli::FORMAT_RG16_UINT_PACK16: return BmRender_Format::R16G16_UINT;
			case gli::FORMAT_RG16_SINT_PACK16: return BmRender_Format::R16G16_SINT;
			case gli::FORMAT_RG16_SFLOAT_PACK16: return BmRender_Format::R16G16_SFLOAT;

			case gli::FORMAT_RGB16_UNORM_PACK16: return BmRender_Format::R16G16B16_UNORM;
			case gli::FORMAT_RGB16_SNORM_PACK16: return BmRender_Format::R16G16B16_SNORM;
			case gli::FORMAT_RGB16_USCALED_PACK16: return BmRender_Format::R16G16B16_USCALED;
			case gli::FORMAT_RGB16_SSCALED_PACK16: return BmRender_Format::R16G16B16_SSCALED;
			case gli::FORMAT_RGB16_UINT_PACK16: return BmRender_Format::R16G16B16_UINT;
			case gli::FORMAT_RGB16_SINT_PACK16: return BmRender_Format::R16G16B16_SINT;
			case gli::FORMAT_RGB16_SFLOAT_PACK16: return BmRender_Format::R16G16B16_SFLOAT;

			case gli::FORMAT_RGBA16_UNORM_PACK16: return BmRender_Format::R16G16B16A16_UNORM;
			case gli::FORMAT_RGBA16_SNORM_PACK16: return BmRender_Format::R16G16B16A16_SNORM;
			case gli::FORMAT_RGBA16_USCALED_PACK16: return BmRender_Format::R16G16B16A16_USCALED;
			case gli::FORMAT_RGBA16_SSCALED_PACK16: return BmRender_Format::R16G16B16A16_SSCALED;
			case gli::FORMAT_RGBA16_UINT_PACK16: return BmRender_Format::R16G16B16A16_UINT;
			case gli::FORMAT_RGBA16_SINT_PACK16: return BmRender_Format::R16G16B16A16_SINT;
			case gli::FORMAT_RGBA16_SFLOAT_PACK16: return BmRender_Format::R16G16B16A16_SFLOAT;

				// 32-bit per component formats
			case gli::FORMAT_R32_UINT_PACK32: return BmRender_Format::R32_UINT;
			case gli::FORMAT_R32_SINT_PACK32: return BmRender_Format::R32_SINT;
			case gli::FORMAT_R32_SFLOAT_PACK32: return BmRender_Format::R32_SFLOAT;

			case gli::FORMAT_RG32_UINT_PACK32: return BmRender_Format::R32G32_UINT;
			case gli::FORMAT_RG32_SINT_PACK32: return BmRender_Format::R32G32_SINT;
			case gli::FORMAT_RG32_SFLOAT_PACK32: return BmRender_Format::R32G32_SFLOAT;

			case gli::FORMAT_RGB32_UINT_PACK32: return BmRender_Format::R32G32B32_UINT;
			case gli::FORMAT_RGB32_SINT_PACK32: return BmRender_Format::R32G32B32_SINT;
			case gli::FORMAT_RGB32_SFLOAT_PACK32: return BmRender_Format::R32G32B32_SFLOAT;

			case gli::FORMAT_RGBA32_UINT_PACK32: return BmRender_Format::R32G32B32A32_UINT;
			case gli::FORMAT_RGBA32_SINT_PACK32: return BmRender_Format::R32G32B32A32_SINT;
			case gli::FORMAT_RGBA32_SFLOAT_PACK32: return BmRender_Format::R32G32B32A32_SFLOAT;

				// Special formats
			case gli::FORMAT_RGB10A2_UNORM_PACK32: return BmRender_Format::A2R10G10B10_UNORM_PACK32;
			case gli::FORMAT_RGB10A2_SNORM_PACK32: return BmRender_Format::A2R10G10B10_SNORM_PACK32;
			case gli::FORMAT_RGB10A2_USCALED_PACK32: return BmRender_Format::A2R10G10B10_USCALED_PACK32;
			case gli::FORMAT_RGB10A2_SSCALED_PACK32: return BmRender_Format::A2R10G10B10_SSCALED_PACK32;
			case gli::FORMAT_RGB10A2_UINT_PACK32: return BmRender_Format::A2R10G10B10_UINT_PACK32;
			case gli::FORMAT_RGB10A2_SINT_PACK32: return BmRender_Format::A2R10G10B10_SINT_PACK32;

			case gli::FORMAT_BGR10A2_UNORM_PACK32: return BmRender_Format::A2B10G10R10_UNORM_PACK32;
			case gli::FORMAT_BGR10A2_SNORM_PACK32: return BmRender_Format::A2B10G10R10_SNORM_PACK32;
			case gli::FORMAT_BGR10A2_USCALED_PACK32: return BmRender_Format::A2B10G10R10_USCALED_PACK32;
			case gli::FORMAT_BGR10A2_SSCALED_PACK32: return BmRender_Format::A2B10G10R10_SSCALED_PACK32;
			case gli::FORMAT_BGR10A2_UINT_PACK32: return BmRender_Format::A2B10G10R10_UINT_PACK32;
			case gli::FORMAT_BGR10A2_SINT_PACK32: return BmRender_Format::A2B10G10R10_SINT_PACK32;

				// Depth formats
			case gli::FORMAT_D16_UNORM_PACK16: return BmRender_Format::D16_UNORM;
			case gli::FORMAT_D24_UNORM_PACK32: return BmRender_Format::D24_UNORM_S8_UINT;
			case gli::FORMAT_D32_SFLOAT_PACK32: return BmRender_Format::D32_SFLOAT;
			case gli::FORMAT_S8_UINT_PACK8: return BmRender_Format::S8_UINT;
			case gli::FORMAT_D16_UNORM_S8_UINT_PACK32: return BmRender_Format::D16_UNORM_S8_UINT;
			case gli::FORMAT_D24_UNORM_S8_UINT_PACK32: return BmRender_Format::D24_UNORM_S8_UINT;
			case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64: return BmRender_Format::D32_SFLOAT_S8_UINT;

				// Compressed formats - DXT/BC
			case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return BmRender_Format::BC1_RGB_UNORM_BLOCK;
			case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return BmRender_Format::BC1_RGB_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return BmRender_Format::BC1_RGBA_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return BmRender_Format::BC1_RGBA_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return BmRender_Format::BC2_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return BmRender_Format::BC2_SRGB_BLOCK;
			case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return BmRender_Format::BC3_UNORM_BLOCK;
			case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return BmRender_Format::BC3_SRGB_BLOCK;

				// Compressed formats - BC7 (BPTC)
			case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return BmRender_Format::BC7_UNORM_BLOCK;
			case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return BmRender_Format::BC7_SRGB_BLOCK;

				// Compressed formats - ETC2
			case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8: return BmRender_Format::ETC2_R8G8B8_UNORM_BLOCK;
			case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8: return BmRender_Format::ETC2_R8G8B8_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8: return BmRender_Format::ETC2_R8G8B8A1_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8: return BmRender_Format::ETC2_R8G8B8A1_SRGB_BLOCK;
			case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16: return BmRender_Format::ETC2_R8G8B8A8_UNORM_BLOCK;
			case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16: return BmRender_Format::ETC2_R8G8B8A8_SRGB_BLOCK;

				// Compressed formats - ASTC (not in enum, return Undefined)
			case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16:
			case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16:

				// Compressed formats - PVRTC (not in enum, return Undefined)
			case gli::FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32:
			case gli::FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32:
			case gli::FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32:
			case gli::FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32:
			case gli::FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32:
			case gli::FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32:
			case gli::FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32:
			case gli::FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32:
			case gli::FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8:
			case gli::FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8:
			case gli::FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8:
			case gli::FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8:

				// Special packed formats
			case gli::FORMAT_RG11B10_UFLOAT_PACK32: return BmRender_Format::B10G11R11_UFLOAT_PACK32;
			case gli::FORMAT_RGB9E5_UFLOAT_PACK32: return BmRender_Format::E5B9G9R9_UFLOAT_PACK32;

				// Luminance/Alpha formats (map to equivalent formats)
			case gli::FORMAT_L8_UNORM_PACK8: return BmRender_Format::R8_UNORM; // Luminance maps to R
			case gli::FORMAT_A8_UNORM_PACK8: return BmRender_Format::R8_UNORM; // Alpha maps to R
			case gli::FORMAT_LA8_UNORM_PACK8: return BmRender_Format::R8G8_UNORM; // Luminance+Alpha maps to RG
			case gli::FORMAT_L16_UNORM_PACK16: return BmRender_Format::R16_UNORM; // Luminance maps to R
			case gli::FORMAT_A16_UNORM_PACK16: return BmRender_Format::R16_UNORM; // Alpha maps to R
			case gli::FORMAT_LA16_UNORM_PACK16: return BmRender_Format::R16G16_UNORM; // Luminance+Alpha maps to RG

				// Default case for unsupported formats
			default:
				return BmRender_Format::Undefined;
		}
	}
}

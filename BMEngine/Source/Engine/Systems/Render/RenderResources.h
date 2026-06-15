#pragma once

#include <vulkan/vulkan.h>


#include <ShortTypes.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Render.h"
#include "RenderInterface.h"

namespace RenderResources
{

	void UpdateBufferRegion(BmRender_GPUBufferUpdateData Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateBuffer(BmRender_GPUBuffer BufferHandle, u64 Offset, const void* Data, u32 DataSize);
	void UpdateImageResource(const BmRender_Image* Handle, BmRender_ImageDescription* Description, void* Data);
}
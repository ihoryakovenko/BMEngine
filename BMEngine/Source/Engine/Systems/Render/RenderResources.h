#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Render.h"
#include "RenderInterface.h"
#include "RenderTypes.h"

namespace RenderResources
{

	void UpdateBufferRegion(BmRender_GPUBufferEntry Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data);

	void OnBufferResourceLoaded(BmRender_GPUBufferEntry Handle);
	void OnImageResourceLoaded(BmRender_Image Handle);


	bool IsBufferResourceReady(BmRender_GPUBufferEntry Handle);
	bool IsImageResourceReady(BmRender_Image Handle);
}
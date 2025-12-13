#pragma once

#include "RenderInterface.h"

u32 BmRender_GetFormatAlignment(BmRender_Format Format);
VkFormat BmRender_FormatToVk(BmRender_Format Format);
VkAllocationCallbacks* BmRender_GetVulkanAllocator();
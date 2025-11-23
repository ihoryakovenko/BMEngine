#pragma once

#include "RenderInterface.h"

u32 BmRender_GetFormatAlignment(BmRender_Format Format);
VkFormat FormatToVk(BmRender_Format Format);
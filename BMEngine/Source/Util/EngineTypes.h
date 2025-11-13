#pragma once

#include <ShortTypes.h>

// TODO: move to globals?
static const u32 MB1 = 1024 * 1024;
static const u32 MB2 = MB1 * 2;
static const u32 MB4 = MB2 * 4;
static const u32 MB8 = MB4 * 2;
static const u32 MB16 = MB8 * 2;
static const u32 MB32 = MB16 * 2;
static const u32 MB64 = MB32 * 2;
static const u32 MB128 = MB64 * 2;
static const u32 MB256 = MB128 * 2;
static const u32 IMAGE_ALIGNMENT = 4096;
static const u32 MAX_LIGHT_SOURCES = 2;
static const u32 MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE = 8;
static const u32 MAX_DESCRIPTOR_BINDING_PER_SET = 16;
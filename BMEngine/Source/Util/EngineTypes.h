#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t s32;

typedef float f32;
typedef double f64;

// TODO: move to globals?
static const u32 MB64 = 1024 * 1024 * 64 * 10;
static const u32 MB128 = 1024 * 1024 * 128;
static const u32 MB256 = 1024 * 1024 * 256;
static const u32 BUFFER_ALIGNMENT = 64;
static const u32 IMAGE_ALIGNMENT = 4096;
static const u32 MAX_LIGHT_SOURCES = 2;
static const u32 MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE = 8;
static const u32 MAX_DESCRIPTOR_BINDING_PER_SET = 16;
#pragma once

#include <SharedLib.h>

inline constexpr u32 MAX_DRAW_FRAMES = 3;
inline constexpr u32 MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS = 16;

enum class BmRender_DescriptorShaderStage : u64;
enum class BmRender_PipelineShaderStage : u8;
enum class BmRender_PipelineSyncStage : u64;
enum class BmRender_ImageType : u8;
enum class BmRender_FenceStatus : u8;
enum class BmRender_WaitResult : u8;
enum class BmRender_SwapchainResult : u8;
enum class BmRender_SemaphoreType : u8;
enum class BmRender_QueueType : u32;
enum class BmRender_PipelineType : u8;
enum class BmRender_DescriptorPoolType : u32;
enum class MemoryPropertyFlag : u32;
enum class BmRender_Filter : u32;
enum class BmRender_SamplerMipmapMode : u32;
enum class BmRender_SamplerAddressMode : u32;
enum class BmRender_CompareOp : u32;
enum class BmRender_BorderColor : u32;
enum class BmRender_ImageLayout : u32;
enum class BmRender_AttachmentLoadOp : u32;
enum class BmRender_AttachmentStoreOp : u32;
enum class BmRender_DescriptorType : u32;
enum class BmRender_IndexType : u32;
enum class BmRender_Format : u32;
enum class BmRender_PolygonMode : u32;
enum class BmRender_CullModeFlags : u32;
enum class BmRender_FrontFace : u32;
enum class BmRender_ColorComponentFlags : u32;
enum class BmRender_BlendFactor : u32;
enum class BmRender_BlendOp : u32;
enum class BmRender_PrimitiveTopology : u32;
enum class BmRender_SampleCount : u32;

enum class LogType
{
	Error,
	Warning,
	Info
};

void RenderLog(LogType logType, const char* format, ...);

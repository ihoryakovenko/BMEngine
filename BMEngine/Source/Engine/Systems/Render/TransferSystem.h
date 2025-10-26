#pragma once

#include "Util/EngineTypes.h"
#include "RenderResources.h"

#include <vulkan/vulkan.h>

namespace TransferSystem
{
	enum class TaskType
	{
		Image,
		Data
	};

	struct ImageTaskDescription
	{
		VkImage DstImage;
		BmRender_Image Handle;
		u32 Width;
		u32 Height;
	};

	struct DataTaskDescription
	{
		VkBuffer DstBuffer;
		u64 DstOffset;
		BmRender_GPUBufferEntry Handle;
		BmRender_PipelineSyncStage StageBarrier;
	};

	struct TransferTask
	{
		union
		{
			ImageTaskDescription TextureDescr;
			DataTaskDescription DataDescr;
		};

		void* RawData;
		u64 DataSize;
		u32 Alignment;
		TaskType Type;
	};

	typedef void* TransferMemory;

	void Init();
	void DeInit();

	u64 Transfer();

	TransferMemory RequestTransferMemory(u64 Size);

	void AddTask(TransferTask* Task);
}
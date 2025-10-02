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
		u32 Width;
		u32 Height;
	};

	struct DataTaskDescription
	{
		VkBuffer DstBuffer;
		u64 DstOffset;
		VulkanHelper::StageBarrier StageBarrier;
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
		BmRender_ResourceHandle Handle;
		u32 Alignment;
		TaskType Type;
	};

	typedef void* TransferMemory;

	struct ResourceTransferMemory
	{
		void* Memory;
		RenderResources::ResourceType Type;
	};

	void Init();
	void DeInit();

	void Transfer();

	TransferMemory RequestTransferMemory(u64 Size);

	void AddTask(TransferTask* Task);
}
#pragma once

#include "Util/EngineTypes.h"
#include "RenderResources.h"

#include <vulkan/vulkan.h>

namespace TransferSystem
{
	struct TextureTaskDescription
	{
		VkImage DstImage;
		u32 Width;
		u32 Height;
	};

	struct DataTaskDescription
	{
		VkBuffer DstBuffer;
		u64 DstOffset;
	};

	struct TransferTask
	{
		RenderResources::ResourceType Type;

		union
		{
			TextureTaskDescription TextureDescr;
			DataTaskDescription DataDescr;
		};

		void* RawData;
		u64 DataSize;

		u32 ResourceIndex;
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
	ResourceTransferMemory RequestMemoryForResource(RenderResources::ResourceType Type, u32 ResourceCount);

	void ProcessTransferTasks();
	void AddTask(TransferTask* Task);
}
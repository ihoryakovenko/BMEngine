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
		union
		{
			TextureTaskDescription TextureDescr;
			DataTaskDescription DataDescr;
		};

		void* RawData;
		u64 DataSize;
		u32 Alignment;
		RenderResources::ResourceType Type;
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

	void AddTask(TransferTask* Task);
}
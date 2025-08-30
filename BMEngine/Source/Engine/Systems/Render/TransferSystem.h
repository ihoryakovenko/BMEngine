#pragma once

#include "Util/EngineTypes.h"

#include <vulkan/vulkan.h>

namespace TransferSystem
{
	enum class TransferTaskType
	{
		TransferTaskType_Texture,
		TransferTaskType_Mesh,
		TransferTaskType_Material,
		TransferTaskType_Instance,
	};

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
		TransferTaskType Type;

		union
		{
			TextureTaskDescription TextureDescr;
			DataTaskDescription DataDescr;
		};

		void* RawData;
		u64 DataSize;

		u32 ResourceIndex;
	};

	void Init();
	void DeInit();

	void Transfer();

	void ProcessTransferTasks();
	void* RequestTransferMemory(u64 Size);
	void AddTask(TransferTask* Task);
}
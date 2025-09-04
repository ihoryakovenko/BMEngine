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

	void TransferStaticMesh(u32 Handle, TransferMemory VertexData);
	void TransferMaterial(u32 Handle);
	void TransferTexture2DSRGB(u32 Handle, TransferMemory TextureData);
	void TransferStaticMeshInstance(u32 Handle);

	TransferMemory RequestTransferMemory(u64 Size);

	void AddTask(TransferTask* Task);
}
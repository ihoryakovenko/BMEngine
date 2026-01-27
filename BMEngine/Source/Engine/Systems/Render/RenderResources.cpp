#include "RenderResources.h"

#include "Util/Util.h"

#include "TransferSystem.h"
#include "RenderInterface.h"
#include "RenderHelper.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Systems.h"

namespace RenderResources
{
	void UpdateBufferRegion(BmRender_GPUBufferBinding Handle, u64 ResourceOffset, const void* Data, u32 DataSize)
	{
		BmRender_GPUBufferData Buffer;
		BmRender_GetGPUBufferData(Handle.GPUBufferHandle, &Buffer);

		const u64 Offset = Handle.BufferOffset + ResourceOffset;

		if (Buffer.PropertyFlag == MemoryPropertyFlag::HostCompatible)
		{
			BmRender_UpdateHostCompatibleBuffer(Handle.GPUBufferHandle, Offset, DataSize, Data);
		}
		else if (Buffer.PropertyFlag == MemoryPropertyFlag::GPULocal)
		{
			// TODO: TMP solution
			void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
			memcpy(TransferMemory, Data, DataSize);

			TransferSystem::TransferTask Task = { };
			Task.DataSize = DataSize;
			Task.Alignment = 1;
			Task.RawData = TransferMemory;
			Task.DataDescr.Handle = Handle;
			Task.DataDescr.StageBarrier = BmRender_PipelineSyncStage::TopOfPipe; // TODO: FIX SHIT!
			Task.Type = TransferSystem::TaskType::Data;

			TransferSystem::AddTask(&Task);
		}
		else
		{
			assert(false);
		}
	}

	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data)
	{
		BmRender_ImageResource Image;
		BmRender_GetImageData(Handle, &Image);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(Image.Size);
		memcpy(TransferMemory, Data, Image.Size);

		TransferSystem::TransferTask Task = { };
		Task.DataSize = Image.Size;
		Task.Alignment = BmRender_GetFormatAlignment(Description->Format);
		Task.RawData = TransferMemory;
		Task.TextureDescr.Handle = Handle;
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
	}
}
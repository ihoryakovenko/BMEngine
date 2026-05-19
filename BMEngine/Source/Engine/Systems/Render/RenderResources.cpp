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
	void UpdateBufferRegion(BmRender_GPUBufferUpdateData Handle, u64 ResourceOffset, const void* Data, u32 DataSize)
	{
		const MemoryPropertyFlag Flag = BmRender_GetGpuBufferMemoryPropertyFlag(Handle.GPUBufferHandle);
		const u64 Offset = Handle.BufferOffset + ResourceOffset;

		if (Flag == MemoryPropertyFlag::HostCompatible)
		{
			BmRender_UpdateHostCompatibleBuffer(Handle.GPUBufferHandle, Offset, DataSize, Data);
		}
		else if (Flag == MemoryPropertyFlag::GPULocal)
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

	void UpdateBuffer(BmRender_GPUBuffer BufferHandle, u64 Offset, const void* Data, u32 DataSize)
	{
		const MemoryPropertyFlag Flag = BmRender_GetGpuBufferMemoryPropertyFlag(BufferHandle);

		if (Flag == MemoryPropertyFlag::HostCompatible)
		{
			BmRender_UpdateHostCompatibleBuffer(BufferHandle, Offset, DataSize, Data);
		}
	}

	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data)
	{
		const u64 Size = BmRender_GetImageSize(Handle);
		const BmRender_Dimensions Dim = BmRender_GetImageDimensions(Handle);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(Size);
		memcpy(TransferMemory, Data, Size);

		TransferSystem::TransferTask Task = { };
		Task.DataSize = Size;
		Task.Alignment = BmRender_GetFormatAlignment(Description->Format);
		Task.RawData = TransferMemory;
		Task.TextureDescr.Handle = Handle;
		Task.TextureDescr.Width = Dim.Width;
		Task.TextureDescr.Height = Dim.Height;
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
	}
}
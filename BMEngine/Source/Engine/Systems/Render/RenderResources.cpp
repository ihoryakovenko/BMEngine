#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"
#include "RenderInterface.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	void OnBufferResourceLoaded(BmRender_GPUBufferEntry Handle)
	{
		GetGPUBufferEntryData(Handle)->IsLoaded = true;
	}

	void OnImageResourceLoaded(BmRender_Image Handle)
	{
		GetImageData(Handle)->IsLoaded = true;
	}

	void UpdateBufferRegion(BmRender_GPUBufferEntry Handle, u64 ResourceOffset, const void* Data, u32 DataSize)
	{
		GPUBufferEntryData* Entry = GetGPUBufferEntryData(Handle);
		GPUBufferData* Buffer = GetGPUBufferData(Entry->GPUBufferHandle);

		const u64 Offset = Entry->BufferOffset + ResourceOffset;

		if (Buffer->PropertyFlag == MemoryPropertyFlag::HostCompatible)
		{
			VkDevice Device = GetCoreContext()->LogicalDevice;
			VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, Buffer->Memory, DataSize, Offset, Data);
			OnBufferResourceLoaded(Handle);
		}
		else if (Buffer->PropertyFlag == MemoryPropertyFlag::GPULocal)
		{
			// TODO: TMP solution
			void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
			memcpy(TransferMemory, Data, DataSize);

			TransferSystem::TransferTask Task = { };
			Task.DataSize = DataSize;
			Task.Alignment = 1;
			Task.DataDescr.DstBuffer = Buffer->Buffer;
			Task.DataDescr.DstOffset = Offset;
			Task.RawData = TransferMemory;
			Task.DataDescr.Handle = Handle;
			Task.DataDescr.StageBarrier = Buffer->BufferStage;
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
		ImageResource* Image = GetImageData(Handle);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(Image->Size);
		memcpy(TransferMemory, Data, Image->Size);

		TransferSystem::TransferTask Task = { };
		Task.DataSize = Image->Size;
		Task.Alignment = VulkanHelper::GetFormatAlignment(Description->Format);
		Task.TextureDescr.DstImage = Image->Image;
		Task.TextureDescr.Width = Description->Width;
		Task.TextureDescr.Height = Description->Height;
		Task.RawData = TransferMemory;
		Task.TextureDescr.Handle = Handle;
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
	}

	bool IsBufferResourceReady(BmRender_GPUBufferEntry Handle)
	{
		return GetGPUBufferEntryData(Handle)->IsLoaded;
	}

	bool IsImageResourceReady(BmRender_Image Handle)
	{
		return GetImageData(Handle)->IsLoaded;
	}
}
#include <Engine/Systems/HandleManager.h>
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Handles.h"
#include "RenderTypes.h"
#include "RenderInterface.h"

#include "VulkanCoreContext.h"

#include <Util/Util.h>

BmRender_FenceStatus BmRender_GetFenceStatus(BmRender_Fence Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkResult Result = vkGetFenceStatus(Device, (VkFence)Handle);

	if (Result == VK_SUCCESS)
	{
		return BmRender_FenceStatus::Signaled;
	}
	else if (Result == VK_NOT_READY)
	{
		return BmRender_FenceStatus::NotReady;
	}
	else
	{
		VULKAN_CHECK_RESULT(Result);
		return BmRender_FenceStatus::NotReady;
	}
}

BmRender_WaitResult BmRender_WaitForFences(BmRender_Fence Handle, VkBool32 WaitAll, u64 Timeout)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkFence Fence = (VkFence)Handle;
	VkResult Result = vkWaitForFences(Device, 1, &Fence, WaitAll, Timeout);

	if (Result == VK_SUCCESS)
	{
		return BmRender_WaitResult::Success;
	}
	else if (Result == VK_TIMEOUT)
	{
		return BmRender_WaitResult::Timeout;
	}
	else
	{
		VULKAN_CHECK_RESULT(Result);
		return BmRender_WaitResult::Timeout;
	}
}

void BmRender_ResetFences(BmRender_Fence Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkFence Fence = (VkFence)Handle;
	VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &Fence));
}

void BmRender_GetSemaphoreCounterValue(BmRender_Semaphore Handle, u64* pValue)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkSemaphore VulkanSemaphore = (VkSemaphore)Handle;
	VULKAN_CHECK_RESULT(vkGetSemaphoreCounterValue(Device, VulkanSemaphore, pValue));
}

void BmRender_BeginCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkCommandBufferBeginInfo BeginInfo = { };
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = 0;

	VkCommandBuffer VulkanCommandBuffer = (VkCommandBuffer)Handle;
	VULKAN_CHECK_RESULT(vkBeginCommandBuffer(VulkanCommandBuffer, &BeginInfo));
}

void BmRender_EndCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkCommandBuffer VulkanCommandBuffer = (VkCommandBuffer)Handle;
	VULKAN_CHECK_RESULT(vkEndCommandBuffer(VulkanCommandBuffer));
}

void BmRender_TransitionImageForRendering(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer, u32 LayersCount)
{
	ImageResource Data;
	GetImageData(Image, &Data);

	VkImageAspectFlags AspectFlags;
	VkPipelineStageFlags2 DstStageMask;
	VkAccessFlags2 DstAccessMask;
	VkImageLayout NewLayout;
	switch (Data.Type)
	{
		case BmRender_ImageType::ColorAttachmentSampled:
		case BmRender_ImageType::TransferSampled:
			DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			DstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			NewLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;

		case BmRender_ImageType::DepthSamplad:
			DstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			DstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			NewLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;

		default:
			assert(false);
	}

	VkImageMemoryBarrier2 Barrier = { };
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Barrier.newLayout = NewLayout;
	Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
	Barrier.srcAccessMask = 0;
	Barrier.dstStageMask = DstStageMask;
	Barrier.dstAccessMask = DstAccessMask;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = (VkImage)Image;
	Barrier.subresourceRange.aspectMask = AspectFlags;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;

	VkDependencyInfo DepInfo = { };
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

		vkCmdPipelineBarrier2((VkCommandBuffer)CommandBuffer, &DepInfo);
}

void BmRender_TransitionImageForSampling(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer, u32 LayersCount)
{
	ImageResource Data;
	GetImageData(Image, &Data);

	VkImageAspectFlags AspectFlags;
	VkPipelineStageFlags2 SrcStageMask;
	VkAccessFlags2 SrcAccessMask;
	VkImageLayout OldLayout;
	switch (Data.Type)
	{
		case BmRender_ImageType::ColorAttachmentSampled:
		case BmRender_ImageType::TransferSampled:
			OldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			// RELEASE: wait for all color-attachment writes to finish
			SrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			SrcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			break;

		case BmRender_ImageType::DepthSamplad:
			OldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			// RELEASE: all depth writes have finished
			SrcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			SrcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;

		default:
			assert(false);
	}

	VkImageMemoryBarrier2 Barrier = { };
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	Barrier.oldLayout = OldLayout;
	Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	Barrier.srcStageMask = SrcStageMask;
	Barrier.srcAccessMask = SrcAccessMask;
	// ACQUIRE: make image ready for sampling in the fragment shader
	Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = (VkImage)Image;
	Barrier.subresourceRange.aspectMask = AspectFlags;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;

	VkDependencyInfo DepInfo = { };
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

		vkCmdPipelineBarrier2((VkCommandBuffer)CommandBuffer, &DepInfo);
}

void BmRender_TransitionImageForPresentation(BmRender_CommandBuffer CommandBuffer, BmRender_Image Image, u32 BaseLayer, u32 LayersCount)
{
	VkImageMemoryBarrier2 Barrier = { };
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	Barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = (VkImage)Image;
	Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;
	Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	Barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	Barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;  // no further memory dep
	Barrier.dstAccessMask = 0;

	VkDependencyInfo DepInfo = { };
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

		vkCmdPipelineBarrier2((VkCommandBuffer)CommandBuffer, &DepInfo);
}

void BmRender_BeginRendering(BmRender_CommandBuffer CommandBuffer, const BmRender_RenderingInfo* pRenderingInfo)
{
	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;

	VkRenderingAttachmentInfo* ColorAttachments = nullptr;
	VkRenderingAttachmentInfo* DepthAttachment = nullptr;

	ColorAttachments = (VkRenderingAttachmentInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkRenderingAttachmentInfo) * pRenderingInfo->ColorAttachmentCount);
	for (u32 i = 0; i < pRenderingInfo->ColorAttachmentCount; ++i)
	{
		const BmRender_RenderingColorAttachment& Attachment = pRenderingInfo->ColorAttachments[i];

		VkRenderingAttachmentInfo* VkAttachment = ColorAttachments + i;
		*VkAttachment = { };
		VkAttachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		VkAttachment->imageView = (VkImageView)Attachment.ImageView;
		VkAttachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachment->loadOp = Attachment.LoadOp;
		VkAttachment->storeOp = Attachment.StoreOp;
		VkAttachment->clearValue.color = Attachment.ClearValue;
	}

	if (pRenderingInfo->DepthAttachment != nullptr)
	{
		const BmRender_RenderingDepthAttachment& Attachment = *pRenderingInfo->DepthAttachment;

		VkRenderingAttachmentInfo DepthAttachmentInfo = { };
		DepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachmentInfo.imageView = (VkImageView)Attachment.ImageView;
		DepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentInfo.loadOp = Attachment.LoadOp;
		DepthAttachmentInfo.storeOp = Attachment.StoreOp;
		DepthAttachmentInfo.clearValue.depthStencil = Attachment.ClearValue;

		DepthAttachment = &DepthAttachmentInfo;
	}

	VkRenderingInfo RenderingInfo = { };
	RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	RenderingInfo.renderArea.offset = pRenderingInfo->Offset;
	RenderingInfo.renderArea.extent = pRenderingInfo->Extent;
	RenderingInfo.layerCount = 1;
	RenderingInfo.colorAttachmentCount = pRenderingInfo->ColorAttachmentCount;
	RenderingInfo.pColorAttachments = ColorAttachments;
	RenderingInfo.pDepthAttachment = DepthAttachment;
	RenderingInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(VkCmdBuffer, &RenderingInfo);
}

void BmRender_BindPipeline(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline)
{
	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;
	vkCmdBindPipeline(VkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)Pipeline);
}

void BmRender_Draw(BmRender_CommandBuffer CommandBuffer, u32 VertexCount, u32 InstanceCount, u32 FirstVertex, u32 FirstInstance)
{
	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;
	vkCmdDraw(VkCmdBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void BmRender_DrawIndexed(BmRender_CommandBuffer CommandBuffer, u32 IndexCount, u32 InstanceCount, u32 FirstIndex, u32 VertexOffset, u32 FirstInstance)
{
	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;
	vkCmdDrawIndexed(VkCmdBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void BmRender_EndRendering(BmRender_CommandBuffer CommandBuffer)
{
	VkCommandBuffer VkCmdBuffer = (VkCommandBuffer)CommandBuffer;
	vkCmdEndRendering(VkCmdBuffer);
}

void BmRender_QueueSubmit(VkQueue Queue, u32 SubmitCount, const BmRender_SubmitInfo* Submits, BmRender_Fence Fence)
{
	VkFence VkFenceHandle = (VkFence)Fence;
	VkSubmitInfo* VkSubmits = (VkSubmitInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkSubmitInfo) * SubmitCount);

	for (u32 i = 0; i < SubmitCount; ++i)
	{
		const BmRender_SubmitInfo& Submit = Submits[i];
		VkSubmitInfo& VkSubmit = VkSubmits[i];

		u32 TotalWaitSemaphoreCount = Submit.WaitSemaphoreCount + Submit.WaitTimelineSemaphoreCount;
		u32 TotalSignalSemaphoreCount = Submit.SignalSemaphoreCount + Submit.SignalTimelineSemaphoreCount;

		VkTimelineSemaphoreSubmitInfo* TimelineInfo = nullptr;
		if (Submit.WaitTimelineSemaphoreCount > 0 || Submit.SignalTimelineSemaphoreCount > 0)
		{
			TimelineInfo = (VkTimelineSemaphoreSubmitInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkTimelineSemaphoreSubmitInfo));
			TimelineInfo->sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
			TimelineInfo->pNext = nullptr;
			TimelineInfo->waitSemaphoreValueCount = TotalWaitSemaphoreCount;
			TimelineInfo->signalSemaphoreValueCount = TotalSignalSemaphoreCount;

			u64* WaitValues = (u64*)Memory::FrameAlloc(GetFrameMemory(), sizeof(u64) * TotalWaitSemaphoreCount);
			TimelineInfo->pWaitSemaphoreValues = WaitValues;

			for (u32 j = 0; j < Submit.WaitSemaphoreCount; ++j)
			{
				WaitValues[j] = 0;
			}

			for (u32 j = 0; j < TotalWaitSemaphoreCount; ++j)
			{
				WaitValues[Submit.WaitSemaphoreCount + j] = Submit.WaitTimelineSemaphores[j].Value;
			}

			u64* SignalValues = (u64*)Memory::FrameAlloc(GetFrameMemory(), sizeof(u64) * TotalSignalSemaphoreCount);
			TimelineInfo->pSignalSemaphoreValues = SignalValues;

			for (u32 j = 0; j < Submit.SignalSemaphoreCount; ++j)
			{
				SignalValues[j] = 0;
			}

			for (u32 j = 0; j < Submit.SignalTimelineSemaphoreCount; ++j)
			{
				SignalValues[Submit.SignalSemaphoreCount + j] = Submit.SignalTimelineSemaphores[j].Value;
			}
		}

		VkSubmit = { };
		VkSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSubmit.pNext = TimelineInfo;
		VkSubmit.waitSemaphoreCount = TotalWaitSemaphoreCount;
		VkSubmit.pWaitDstStageMask = Submit.WaitDstStageFlags;
		VkSubmit.commandBufferCount = Submit.CommandBufferCount;
		VkSubmit.signalSemaphoreCount = TotalSignalSemaphoreCount;

		VkCommandBuffer* CommandBuffers = (VkCommandBuffer*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkCommandBuffer) * Submit.CommandBufferCount);
		VkSubmit.pCommandBuffers = CommandBuffers;

		for (u32 j = 0; j < Submit.CommandBufferCount; ++j)
		{
			CommandBuffers[j] = (VkCommandBuffer)Submit.CommandBuffers[j];
		}

		VkSemaphore* WaitSemaphores = (VkSemaphore*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkSemaphore) * TotalWaitSemaphoreCount);
		VkSubmit.pWaitSemaphores = WaitSemaphores;

		for (u32 j = 0; j < Submit.WaitSemaphoreCount; ++j)
		{
			WaitSemaphores[j] = (VkSemaphore)Submit.WaitSemaphores[j];
		}

		for (u32 j = 0; j < Submit.WaitTimelineSemaphoreCount; ++j)
		{
			WaitSemaphores[Submit.WaitSemaphoreCount + j] = (VkSemaphore)Submit.WaitTimelineSemaphores[j].Semaphore;
		}

		VkSemaphore* SignalSemaphores = (VkSemaphore*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkSemaphore) * TotalSignalSemaphoreCount);
		VkSubmit.pSignalSemaphores = SignalSemaphores;

		for (u32 j = 0; j < Submit.SignalSemaphoreCount; ++j)
		{
			SignalSemaphores[j] = (VkSemaphore)Submit.SignalSemaphores[j];
		}

		for (u32 j = 0; j < Submit.SignalTimelineSemaphoreCount; ++j)
		{
			SignalSemaphores[Submit.SignalSemaphoreCount + j] = (VkSemaphore)Submit.SignalTimelineSemaphores[j].Semaphore;
		}
	}

	VULKAN_CHECK_RESULT(vkQueueSubmit(Queue, SubmitCount, VkSubmits, VkFenceHandle));
}

BmRender_SwapchainResult BmRender_QueuePresent(VkQueue Queue, const BmRender_PresentInfo* pPresentInfo)
{
	VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();

	VkPresentInfoKHR PresentInfo = { };
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.waitSemaphoreCount = pPresentInfo->WaitSemaphoreCount;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &CoreContext->VulkanSwapchain;
	PresentInfo.pImageIndices = pPresentInfo->ImageIndices;

	VkSemaphore* WaitSemaphores = (VkSemaphore*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkSemaphore) * pPresentInfo->WaitSemaphoreCount);
	PresentInfo.pWaitSemaphores = WaitSemaphores;

	for (u32 i = 0; i < pPresentInfo->WaitSemaphoreCount; ++i)
	{
		WaitSemaphores[i] = (VkSemaphore)pPresentInfo->WaitSemaphores[i];
	}

	VkResult Result = vkQueuePresentKHR(Queue, &PresentInfo);

	if (Result == VK_SUCCESS)
	{
		return BmRender_SwapchainResult::Success;
	}
	else if (Result == VK_SUBOPTIMAL_KHR)
	{
		return BmRender_SwapchainResult::Suboptimal;
	}
	else if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return BmRender_SwapchainResult::OutOfDate;
	}
	else
	{
		VULKAN_CHECK_RESULT(Result);
		return BmRender_SwapchainResult::Success;
	}
}

BmRender_SwapchainResult BmRender_AcquireNextSwapchainImage(u64 Timeout, BmRender_Semaphore Semaphore, VkFence Fence, u32* pImageIndex)
{
	VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();
	VkDevice Device = CoreContext->LogicalDevice;

	VkSwapchainKHR Swapchain = CoreContext->VulkanSwapchain;
	VkResult Result = vkAcquireNextImageKHR(Device, Swapchain, Timeout, (VkSemaphore)Semaphore, Fence, pImageIndex);

	if (Result == VK_SUCCESS)
	{
		return BmRender_SwapchainResult::Success;
	}
	else if (Result == VK_SUBOPTIMAL_KHR)
	{
		return BmRender_SwapchainResult::Suboptimal;
	}
	else if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return BmRender_SwapchainResult::OutOfDate;
	}
	else
	{
		VULKAN_CHECK_RESULT(Result);
		return BmRender_SwapchainResult::Success;
	}
}
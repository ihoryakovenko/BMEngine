#include "MainRenderPass.h"

#include "VulkanInterface/VulkanInterface.h"
#include "Render.h"

namespace MainRenderPass
{
	static VulkanInterface::RenderPipeline DeferredPipeline;
	static VkDescriptorSet DeferredInputSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	void Init()
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
		}
	}

	void OnDraw()
	{
		//const u32 ImageIndex = VulkanInterface::TestGetImageIndex();

		//Render::BindNextSubpass();
		//Render::BindPipeline(DeferredPipeline.Pipeline);
		//Render::BindDescriptorSet(&MainPass.DeferredInputSet[ImageIndex], 1, DeferredPipeline.PipelineLayout, 0, nullptr, 0);
		//Render::Draw(3, 1); // 3 hardcoded vertices for second "post processing" subpass
		//Render::EndRenderPass();
	}
}
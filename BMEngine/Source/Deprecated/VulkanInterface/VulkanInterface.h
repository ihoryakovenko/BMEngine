#pragma once





//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////








#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <mutex>

struct GLFWwindow;

namespace VulkanInterface
{
	struct VulkanInterfaceConfig
	{
		u32 MaxTextures = 0;
	};

	struct UniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformImage
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformSetAttachmentInfo
	{
		union
		{
			VkDescriptorBufferInfo BufferInfo;
			VkDescriptorImageInfo ImageInfo;
		};

		VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	};

	struct UniformImageInterfaceCreateInfo
	{
		const void* pNext = nullptr;
		VkImageViewCreateFlags Flags = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkComponentMapping Components;
		VkImageSubresourceRange SubresourceRange;
	};

	struct RenderPipeline
	{
		VkPipeline Pipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct LayoutLayerTransitionData
	{
		u32 BaseArrayLayer;
		u32 LayerCount;
	};

	// TO refactor
	u32 GetImageCount();
	VkImageView* GetSwapchainImageViews();
	VkImage* GetSwapchainImages();
	u32 TestGetImageIndex();

	void WaitDevice();




	typedef UniformBuffer IndexBuffer;
	typedef UniformBuffer VertexBuffer;

	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkCommandBuffer GetCommandBuffer();
	VkQueue GetTransferQueue();
	VkQueue GetGraphicsQueue();
	VkFormat GetSurfaceFormat();
	u32 GetQueueGraphicsFamilyIndex();
	VkDescriptorPool GetDescriptorPool();
	VkInstance GetInstance();
	VkCommandPool GetTransferCommandPool();
	VkSwapchainKHR GetSwapchain();


}

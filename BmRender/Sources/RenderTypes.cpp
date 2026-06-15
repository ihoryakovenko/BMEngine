#include "RenderTypes.h"

#include <cassert>

#include <SharedLib.h>

#include "Handles.h"
#include "VulkanCoreContext.h"
#include "RenderHelper.h"

static void* VKAPI_CALL VulkanAllocationCallback(
	void* UserData,
	size_t Size,
	size_t Alignment,
	VkSystemAllocationScope AllocationScope)
{
	return malloc(Size);
}

static void* VKAPI_CALL VulkanReallocationCallback(
	void* pUserData,
	void* pOriginal,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope)
{
	return realloc(pOriginal, size);
}

static void VKAPI_CALL VulkanFreeCallback(
	void* pUserData,
	void* pMemory)
{
	if (pMemory)
	{
		free(pMemory);
	}
}

static void VKAPI_CALL VulkanInternalAllocationNotification(
	void* pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{

}

static void VKAPI_CALL VulkanInternalFreeNotification(
	void* pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{

}

static VulkanCoreContext CoreContext;

static Memory_LinearAllocator FrameMemory;

static VkAllocationCallbacks VulkanAllocator;



void CreateCoreContext(GLFWwindow* WindowHandler)
{
	VulkanAllocator.pUserData = nullptr;
	VulkanAllocator.pfnAllocation = VulkanAllocationCallback;
	VulkanAllocator.pfnReallocation = VulkanReallocationCallback;
	VulkanAllocator.pfnFree = VulkanFreeCallback;
	VulkanAllocator.pfnInternalAllocation = VulkanInternalAllocationNotification;
	VulkanAllocator.pfnInternalFree = VulkanInternalFreeNotification;

	CreateCoreContext(&CoreContext, WindowHandler);
}

void DestroyCoreContext()
{
	DestroyCoreContext(&CoreContext);
}

VulkanCoreContext* GetCoreContext()
{
	return &CoreContext;
}

VkAllocationCallbacks* GetVulkanAllocator()
{
	return &VulkanAllocator;
}

Memory_LinearAllocator* GetFrameMemory()
{
	return &FrameMemory;
}

void InitializeFrameMemory()
{
	Memory_LinearAllocator_Init(&FrameMemory, 1024 * 1024);
}

void DeMemory_LinearAllocator_Init()
{
	Memory_LinearAllocator_Free(&FrameMemory);
}



BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout* LayoutHandle, BmRender_DescriptorPool PoolHandle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	BmRender_DescriptorSet NewSet;
	NewSet.Layout = LayoutHandle;

	VkDescriptorPool Pool = (VkDescriptorPool)PoolHandle;

	VkDescriptorSetLayout VkLayout = LayoutHandle->InternalLayout;
	VkDescriptorSetAllocateInfo AllocInfo = { };
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = Pool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &VkLayout;

	VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, &NewSet.InternalSet));

	return NewSet;
}

BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutBinding* Bindings, u32 BindingsCount)
{
	assert(BindingsCount <= MAX_DESCRIPTOR_SET_LAYOUT_BUINDINGS);
	VkDevice Device = GetCoreContext()->LogicalDevice;

	BmRender_DescriptorSetLayout Layout = { };
	Layout.BindingsCount = BindingsCount;

	VkDescriptorSetLayoutBinding* NewLayoutBindings = (VkDescriptorSetLayoutBinding*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkDescriptorSetLayoutBinding) * BindingsCount);
	for (u32 i = 0; i < BindingsCount; ++i)
	{
		NewLayoutBindings[i].binding = i;
		NewLayoutBindings[i].descriptorCount = Bindings[i].DescriptorCount;
		NewLayoutBindings[i].descriptorType = DescriptorTypeToVk(Bindings[i].DescriptorType);
		NewLayoutBindings[i].stageFlags = DescriptorShaderStageToVkShaderStage(Bindings[i].StageFlags);
		NewLayoutBindings[i].pImmutableSamplers = nullptr;

		Layout.LayoutBindings[i] = Bindings[i].DescriptorType;
	}

	VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
	LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutCreateInfo.bindingCount = BindingsCount;
	LayoutCreateInfo.pBindings = NewLayoutBindings;
	LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	LayoutCreateInfo.pNext = nullptr;

	VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, GetVulkanAllocator(), &Layout.InternalLayout));

	return Layout;
}

void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet* DescriptorSetHandle, const BmRender_DescriptorSetUpdateData* Bindings, u32 BindingsCount)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkWriteDescriptorSet) * BindingsCount);

	for (u32 i = 0; i < BindingsCount; i++)
	{
		const BmRender_DescriptorSetUpdateData& Binding = Bindings[i];

		BmRender_DescriptorType DescriptorType = DescriptorSetHandle->Layout->LayoutBindings[Binding.DstBinding];
		VkDescriptorType VkDescriptorType = DescriptorTypeToVk(DescriptorType);

		WriteDescriptorSets[i] = { };
		WriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[i].dstSet = DescriptorSetHandle->InternalSet;
		WriteDescriptorSets[i].dstBinding = Binding.DstBinding;
		WriteDescriptorSets[i].dstArrayElement = Binding.DstArrayElement;
		WriteDescriptorSets[i].descriptorType = VkDescriptorType;
		WriteDescriptorSets[i].descriptorCount = Binding.BindingCount;

		if (VkDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || VkDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			VkDescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || VkDescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			VkDescriptorBufferInfo* BufferInfo = (VkDescriptorBufferInfo*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkDescriptorBufferInfo) * Binding.BindingCount);
			for (u32 j = 0; j < Binding.BindingCount; ++j)
			{
				const BmRender_GPUBufferUpdateData& Entry = Binding.BufferRegions[j];

				BufferInfo[j].buffer = Entry.GPUBufferHandle->InternalBuffer;
				BufferInfo[j].offset = Entry.BufferOffset;
				BufferInfo[j].range = Entry.Size;
			}

			WriteDescriptorSets[i].pBufferInfo = BufferInfo;
		}
		else if (VkDescriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || VkDescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
			VkDescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			VkDescriptorImageInfo* ImageInfo = (VkDescriptorImageInfo*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkDescriptorImageInfo));
			ImageInfo->imageLayout = ImageLayoutToVk(Binding.ImageBinding.ImageLayout);
			ImageInfo->imageView = Binding.ImageBinding.ImageView.InternalView;
			ImageInfo->sampler = (VkSampler)Binding.ImageBinding.Sampler;

			WriteDescriptorSets[i].pImageInfo = ImageInfo;
		}
		else
		{
			assert(false && "Unimplemented");
		}
	}

	vkUpdateDescriptorSets(Device, BindingsCount, WriteDescriptorSets, 0, nullptr);
}

BmRender_PushConstant BmRender_CreatePushConstant(BmRender_DescriptorShaderStage Stage, u32 Offset, u32 Size)
{
	BmRender_PushConstant Constant;
	Constant.Offset = Offset;
	Constant.Size = Size;
	Constant.StageFlags = Stage; // Now uses BmRender_DescriptorShaderStage directly

	return Constant;
}

static BmRender_GPUBuffer CreateGPUBuffer(u64 Capacity, MemoryPropertyFlag MemoryFlag, BufferUsageFlag Flag)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

	BmRender_GPUBuffer NewBuffer = { };

	if (Flag == BufferUsageFlag::UniformFlag)
	{
		VkPhysicalDeviceProperties DeviceProperties;
		vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

		if (Capacity > DeviceProperties.limits.maxUniformBufferRange)
		{
			assert(false);
		}
	}

	NewBuffer.PropertyFlag = MemoryFlag;

	NewBuffer.InternalBuffer = CreateBuffer(Device, Capacity, Flag, GetVulkanAllocator());

	VkBufferUsageFlags BufferUsageFlags = (VkBufferUsageFlags)Flag;
	DeviceMemoryAllocResult AllocResult = AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.InternalBuffer, MemoryFlag, BufferUsageFlags, GetVulkanAllocator());
	NewBuffer.Memory = CreateDeviceMemoryHandle(AllocResult.Memory);

	VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.InternalBuffer, (VkDeviceMemory)NewBuffer.Memory, 0));

	return NewBuffer;
}

static BmRender_Image CreateImageResource(BmRender_ImageDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

	BmRender_Image Resource;
	Resource.Format = Description->Format;
	Resource.Type = Description->Type;

	VkImageUsageFlags Usage;

	if ((Description->Type == BmRender_ImageType::MultiSampledDepthAttachment || Description->Type == BmRender_ImageType::MultiSampledColorAttachment) &&
		((u32)Description->SampleCount & (u32)BmRender_SampleCount::Count1))
	{
		assert(false || "The iamge type is multisampled, but sample count is 1");
	}

	switch (Description->Type)
	{
		case BmRender_ImageType::TransferSampled:
			Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;

		case BmRender_ImageType::DepthSamplad:
			Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;

		case BmRender_ImageType::ColorAttachmentSampled:
			Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;

		case BmRender_ImageType::MultiSampledDepthAttachment:
			Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			break;

		case BmRender_ImageType::MultiSampledColorAttachment:
			Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			break;

		default:
			assert(false);
			break;
	}

	VkImageCreateInfo ImageCreateInfo = { };
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.extent.width = Description->Width;
	ImageCreateInfo.extent.height = Description->Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = Description->ArrayLayers;
	ImageCreateInfo.format = BmRender_FormatToVk(Description->Format);
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.usage = Usage;
	ImageCreateInfo.samples = (VkSampleCountFlagBits)Description->SampleCount;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.flags = 0;

	VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, GetVulkanAllocator(), &Resource.InternalImage));

	DeviceMemoryAllocResult AllocResult = AllocateDeviceMemory(PhysicalDevice, Device,
		Resource.InternalImage, MemoryPropertyFlag::GPULocal, GetVulkanAllocator());

	Resource.Memory = CreateDeviceMemoryHandle(AllocResult.Memory);
	Resource.Dimensions.Width = Description->Width;
	Resource.Dimensions.Height = Description->Height;
	Resource.Size = AllocResult.Size;
	Resource.SampleCount = Description->SampleCount;

	VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Resource.InternalImage, (VkDeviceMemory)Resource.Memory, 0));
	return Resource;
}

static BmRender_ImageView CreateImageView(const BmRender_Image* Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType)
{
	VkImageView View;

	VkImageViewCreateInfo ViewCreateInfo = { };
	ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewCreateInfo.flags = 0;
	ViewCreateInfo.viewType = ViewType;
	ViewCreateInfo.format = BmRender_FormatToVk(Handle->Format);
	ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.subresourceRange.aspectMask = ImageTypeToVkImageAspectFlags(Handle->Type);
	ViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ViewCreateInfo.subresourceRange.levelCount = 1;
	ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
	ViewCreateInfo.subresourceRange.layerCount = LayerCount;
	ViewCreateInfo.image = Handle->InternalImage;

	VkDevice Device = GetCoreContext()->LogicalDevice;
	VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, GetVulkanAllocator(), &View));

	BmRender_ImageView ImageViewData;
	ImageViewData.Image = Handle;
	ImageViewData.Format = Handle->Format;
	ImageViewData.InternalView = View;

	return ImageViewData;
}

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkSamplerCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.magFilter = FilterToVkFilter(Description->MagFilter);
	CreateInfo.minFilter = FilterToVkFilter(Description->MinFilter);
	CreateInfo.mipmapMode = SamplerMipmapModeToVk(Description->MipmapMode);
	CreateInfo.addressModeU = SamplerAddressModeToVk(Description->AddressModeU);
	CreateInfo.addressModeV = SamplerAddressModeToVk(Description->AddressModeV);
	CreateInfo.addressModeW = SamplerAddressModeToVk(Description->AddressModeW);
	CreateInfo.mipLodBias = Description->MipLodBias;
	CreateInfo.anisotropyEnable = Description->AnisotropyEnable;
	CreateInfo.maxAnisotropy = Description->MaxAnisotropy;
	CreateInfo.compareEnable = Description->CompareEnable;
	CreateInfo.compareOp = CompareOpToVk(Description->CompareOp);
	CreateInfo.minLod = Description->MinLod;
	CreateInfo.maxLod = Description->MaxLod;
	CreateInfo.borderColor = BorderColorToVk(Description->BorderColor);
	CreateInfo.unnormalizedCoordinates = Description->UnnormalizedCoordinates;

	VkSampler VulkanSampler;
	VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, GetVulkanAllocator(), &VulkanSampler));

	return CreateSamplerHandle(VulkanSampler);
}

BmRender_Pipeline BmRender_CreatePipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_PipelineSettings* Settings, const BmRender_ShaderStageDescription* ShaderStageDescriptions,
	u32 ShaderStagesCount, const AttachmentData* Attachment)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkPipelineShaderStageCreateInfo* VkShaderStages = (VkPipelineShaderStageCreateInfo*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkPipelineShaderStageCreateInfo) * ShaderStagesCount);
	for (u32 i = 0; i < ShaderStagesCount; ++i)
	{
		const BmRender_ShaderStageDescription* ShaderStageDesc = ShaderStageDescriptions + i;

		VkPipelineShaderStageCreateInfo* VkStage = VkShaderStages + i;
		VkStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VkStage->pNext = nullptr;
		VkStage->flags = 0;
		VkStage->stage = PipelineShaderStageToVkShaderStage(ShaderStageDesc->Stage);
		VkStage->module = (VkShaderModule)ShaderStageDesc->Shader;
		VkStage->pName = ShaderStageDesc->EntryPointFunction;
		VkStage->pSpecializationInfo = nullptr;
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { };
	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	bool SampleCountFound = false;
	VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_1_BIT;

	VkFormat* ColorAttachmentFormats = (VkFormat*)Memory_LinearAllocator_Alloc(GetFrameMemory(), Attachment->ColorAttachmentCount * sizeof(VkFormat));
		for (u32 i = 0; i < Attachment->ColorAttachmentCount; ++i)
		{
			if (Attachment->ColorAttachments[i].Format != BmRender_Format::Undefined)
			{
				ColorAttachmentFormats[i] = BmRender_FormatToVk(Attachment->ColorAttachments[i].Format);

				if (!SampleCountFound)
				{
					SampleCount = SampleCountToVk(Attachment->ColorAttachments[i].Image->SampleCount);
				}
			}
			else
			{
				ColorAttachmentFormats[i] = VK_FORMAT_UNDEFINED;
			}
		}

	VkFormat DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	if (Attachment->DepthAttachment.Format != BmRender_Format::Undefined)
	{
		DepthAttachmentFormat = BmRender_FormatToVk(Attachment->DepthAttachment.Format);

		if (!SampleCountFound)
		{
			SampleCount = SampleCountToVk(Attachment->DepthAttachment.Image->SampleCount);
		}
	}

	VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	if (Attachment->StencilAttachment.Format != BmRender_Format::Undefined)
	{
		StencilAttachmentFormat = BmRender_FormatToVk(Attachment->DepthAttachment.Format);

		if (!SampleCountFound)
		{
			SampleCount = SampleCountToVk(Attachment->DepthAttachment.Image->SampleCount);
		}
	}

	VkPipelineRenderingCreateInfo RenderingInfo = { };
	RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	RenderingInfo.pNext = nullptr;
	RenderingInfo.colorAttachmentCount = Attachment->ColorAttachmentCount;
	RenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats;
	RenderingInfo.depthAttachmentFormat = DepthAttachmentFormat;
	RenderingInfo.stencilAttachmentFormat = StencilAttachmentFormat;

	VkPipelineColorBlendAttachmentState VkColorBlendAttachment = ColorBlendAttachmentToVk(Settings->ColorBlendAttachment);
	VkPipelineColorBlendStateCreateInfo ColorBlendState = ColorBlendStateToVk(Settings->ColorBlendState);
	ColorBlendState.pAttachments = &VkColorBlendAttachment;

	VkViewport VkViewport = ViewportToVk(Settings->Viewport);
	VkRect2D VkScissor = Rect2DToVk(Settings->Scissor);
	
	VkPipelineViewportStateCreateInfo ViewportState = ViewportStateToVk(Settings->ViewportState);
	ViewportState.pViewports = &VkViewport;
	ViewportState.pScissors = &VkScissor;

	VkPipelineRasterizationStateCreateInfo RasterizationState = RasterizationStateToVk(Settings->RasterizationState);
	VkPipelineMultisampleStateCreateInfo MultisampleState = MultisampleStateToVk(Settings->MultisampleState);
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = InputAssemblyStateToVk(Settings->InputAssemblyState);
	VkPipelineDepthStencilStateCreateInfo DepthStencilState = DepthStencilStateToVk(Settings->DepthStencilState);

	MultisampleState.rasterizationSamples = SampleCount;

	auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Memory_LinearAllocator_Alloc(GetFrameMemory(), sizeof(VkGraphicsPipelineCreateInfo));
	*PipelineCreateInfo = { };
	PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineCreateInfo->stageCount = ShaderStagesCount;
	PipelineCreateInfo->pStages = VkShaderStages;
	PipelineCreateInfo->pVertexInputState = &VertexInputState;
	PipelineCreateInfo->pInputAssemblyState = &InputAssemblyState;
	PipelineCreateInfo->pViewportState = &ViewportState;
	PipelineCreateInfo->pDynamicState = nullptr;
	PipelineCreateInfo->pRasterizationState = &RasterizationState;
	PipelineCreateInfo->pMultisampleState = &MultisampleState;
	PipelineCreateInfo->pColorBlendState = &ColorBlendState;
	PipelineCreateInfo->pDepthStencilState = &DepthStencilState;
	PipelineCreateInfo->layout = PipelineLayout.InternalLayout;
	PipelineCreateInfo->renderPass = nullptr;
	PipelineCreateInfo->subpass = 0;
	PipelineCreateInfo->pNext = &RenderingInfo;

	PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
	PipelineCreateInfo->basePipelineIndex = -1;

	VkPipeline Pipeline;
	VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, GetVulkanAllocator(), &Pipeline));

	PipelineData Data;
	Data.Layout = PipelineLayout;
	return CreatePipelineHandle(Pipeline, &Data);
}

BmRender_Pipeline BmRender_CreateComputePipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_ShaderStageDescription* ShaderStageDescription)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkPipelineShaderStageCreateInfo VkShaderStage = { };
	VkShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VkShaderStage.pNext = nullptr;
	VkShaderStage.flags = 0;
	VkShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	VkShaderStage.module = (VkShaderModule)ShaderStageDescription->Shader;
	VkShaderStage.pName = ShaderStageDescription->EntryPointFunction;
	VkShaderStage.pSpecializationInfo = nullptr;

	VkComputePipelineCreateInfo PipelineCreateInfo = { };
	PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	PipelineCreateInfo.pNext = nullptr;
	PipelineCreateInfo.flags = 0;
	PipelineCreateInfo.stage = VkShaderStage;
	PipelineCreateInfo.layout = PipelineLayout.InternalLayout;
	PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	PipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline Pipeline;
	VULKAN_CHECK_RESULT(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, GetVulkanAllocator(), &Pipeline));

	PipelineData Data;
	Data.Layout = PipelineLayout;
	return CreatePipelineHandle(Pipeline, &Data);
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkDescriptorSetLayout* VkSetLayouts = (VkDescriptorSetLayout*)Memory_LinearAllocator_Alloc(GetFrameMemory(), Description->SetLayoutCount * sizeof(VkDescriptorSetLayout));
	for (u32 i = 0; i < Description->SetLayoutCount; ++i)
	{
		VkSetLayouts[i] = Description->SetLayouts[i].InternalLayout;
	}

	VkPushConstantRange* VkPushConstantRanges = (VkPushConstantRange*)Memory_LinearAllocator_Alloc(GetFrameMemory(), Description->PushConstantRangeCount * sizeof(VkPushConstantRange));
	for (u32 i = 0; i < Description->PushConstantRangeCount; ++i)
	{
		VkPushConstantRanges[i].offset = Description->PushConstantRanges[i].Offset;
		VkPushConstantRanges[i].size = Description->PushConstantRanges[i].Size;
		VkPushConstantRanges[i].stageFlags = ShaderStageFlagsToVk(Description->PushConstantRanges[i].StageFlags);
	}

	VkPipelineLayoutCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfo.setLayoutCount = Description->SetLayoutCount;
	CreateInfo.pSetLayouts = VkSetLayouts;
	CreateInfo.pushConstantRangeCount = Description->PushConstantRangeCount;
	CreateInfo.pPushConstantRanges = VkPushConstantRanges;
	CreateInfo.pNext = nullptr;

	BmRender_PipelineLayout LayoutData = { };
	VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, GetVulkanAllocator(), &LayoutData.InternalLayout));
	LayoutData.PipelineType = Description->PipelineType;

	return LayoutData;
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolSize* PoolSizes, u32 MaxSets, u32 PoolSizeCount, BmRender_DescriptorPoolType Type)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkDescriptorPoolSize* VkPoolSizes = (VkDescriptorPoolSize*)Memory_LinearAllocator_Alloc(GetFrameMemory(), PoolSizeCount * sizeof(VkDescriptorPoolSize));
	for (u32 i = 0; i < PoolSizeCount; ++i)
	{
		VkPoolSizes[i] = DescriptorPoolSizeToVk(PoolSizes[i]);
	}

	VkDescriptorPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.maxSets = MaxSets;
	CreateInfo.poolSizeCount = PoolSizeCount;
	CreateInfo.pPoolSizes = VkPoolSizes;
	CreateInfo.flags = DescriptorPoolTypeToVkFlags(Type);
	CreateInfo.pNext = nullptr;

	VkDescriptorPool DescriptorPool;
	VULKAN_CHECK_RESULT(vkCreateDescriptorPool(Device, &CreateInfo, GetVulkanAllocator(), &DescriptorPool));

	return CreateDescriptorPoolHandle(DescriptorPool);
}

BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkShaderModuleCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.codeSize = Description->CodeSize;
	CreateInfo.pCode = Description->Code;

	VkShaderModule ShaderModule;
	VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &CreateInfo, GetVulkanAllocator(), &ShaderModule));

	return CreateShaderHandle(ShaderModule);
}


BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type, BmRender_SampleCount SampleCount)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = 1;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;
	Descr.SampleCount = SampleCount;

	return CreateImageResource(&Descr);
}

BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, BmRender_Format Format, BmRender_ImageType Type, u32 ArrayLayers, BmRender_SampleCount SampleCount)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = ArrayLayers;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;
	Descr.SampleCount = SampleCount;

	return CreateImageResource(&Descr);
}

BmRender_ImageView BmRender_CreateImageView2D(const BmRender_Image* Handle)
{
	return CreateImageView(Handle, 0, 1, VK_IMAGE_VIEW_TYPE_2D);
}

BmRender_ImageView BmRender_CreateImageView2DArray(const BmRender_Image* Handle, u32 BaseLayer, u32 LayerCount)
{
	return CreateImageView(Handle, BaseLayer, LayerCount, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
}

BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferUsageFlag::CombinedVertexIndexFlag);
}

BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferUsageFlag::InstanceFlag);
}

BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferUsageFlag::UniformFlag);
}

BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferUsageFlag::StorageFlag);
}

BmRender_GPUBuffer BmRender_CreateIndirectDrawBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferUsageFlag::IndirectDrawBufferFlag);
}

BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size)
{
	return CreateGPUBuffer(Size, MemoryPropertyFlag::HostCompatible, BufferUsageFlag::StagingFlag);
}

BmRender_Fence BmRender_CreateFence()
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkFenceCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	CreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence Fence;
	VULKAN_CHECK_RESULT(vkCreateFence(Device, &CreateInfo, GetVulkanAllocator(), &Fence));

	return CreateFenceHandle(Fence);
}

BmRender_Semaphore BmRender_CreateSemaphore()
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkSemaphoreCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;

	BmRender_Semaphore Data;
	Data.Type = BmRender_SemaphoreType::Binary;
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, GetVulkanAllocator(), &Data.InternalSemaphore));

	return Data;
}

BmRender_Semaphore BmRender_CreateTimelineSemaphore(u64 InitialValue)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkSemaphoreTypeCreateInfo TypeCreateInfo = { };
	TypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	TypeCreateInfo.pNext = nullptr;
	TypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	TypeCreateInfo.initialValue = InitialValue;

	VkSemaphoreCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	CreateInfo.pNext = &TypeCreateInfo;
	CreateInfo.flags = 0;

	BmRender_Semaphore Data;
	Data.Type = BmRender_SemaphoreType::Timeline;
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, GetVulkanAllocator(), &Data.InternalSemaphore));

	return Data;
}

bool BmRender_IsDedicatedQueuePresent(BmRender_QueueType BmRender_QueueType)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext->Indices);

	if (SelectedFamilyIndex == -1)
	{
		return false;
	}

	bool NeedsTransfer = ((u32)BmRender_QueueType & (u32)BmRender_QueueType::Transfer) != 0;
	bool NeedsGraphics = ((u32)BmRender_QueueType & (u32)BmRender_QueueType::Graphic) != 0;

	// If it's a transfer-only queue, check if it's dedicated (different from graphics)
	if (NeedsTransfer && !NeedsGraphics)
	{
		return CoreContext->Indices.TransferFamily != CoreContext->Indices.GraphicsFamily;
	}

	// Graphics queue is always present if we got a valid index
	return true;
}

BmRender_Queue BmRender_CreateQueue(BmRender_QueueType BmRender_QueueType)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	VkDevice Device = CoreContext->LogicalDevice;

	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext->Indices);

	if (SelectedFamilyIndex == -1)
	{
		assert(false);
		return {};
	}

	BmRender_Queue Data = { };
	vkGetDeviceQueue(Device, SelectedFamilyIndex, 0, &Data.InternalQueue);
	Data.QueueType = BmRender_QueueType;

	return Data;
}

BmRender_CommandPool BmRender_CreateCommandPool(BmRender_QueueType BmRender_QueueType)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	VkDevice Device = CoreContext->LogicalDevice;

	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext->Indices);

	if (SelectedFamilyIndex == -1)
	{
		assert(false);
		return {};
	}

	u32 QueueFamilyIndex = (u32)SelectedFamilyIndex;

	VkCommandPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CreateInfo.queueFamilyIndex = QueueFamilyIndex;

	BmRender_CommandPool Data;
	Data.QueueFamilyIndex = QueueFamilyIndex;
	VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &CreateInfo, GetVulkanAllocator(), &Data.InternalPool));

	return Data;
}

BmRender_CommandBuffer BmRender_AllocateCommandBuffer(const BmRender_CommandPool* CommandPool)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkCommandPool VulkanCommandPool = CommandPool->InternalPool;

	VkCommandBufferAllocateInfo AllocateInfo = { };
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = VulkanCommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	BmRender_CommandBuffer Data;
	Data.CommandPool = CommandPool;
	VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &AllocateInfo, &Data.InternalBuffer));

	return Data;
}

void BmRender_UpdateHostCompatibleBuffer(BmRender_GPUBuffer* Buffer, u64 BufferOffset, u64 DataSize, const void* Data)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	UpdateHostCompatibleBufferMemory(Device, (VkDeviceMemory)Buffer->Memory, DataSize, BufferOffset, Data);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyPipelineLayout(Device, Handle.InternalLayout, GetVulkanAllocator());
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroySampler(Device, (VkSampler)Handle, GetVulkanAllocator());
}

void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout* Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorSetLayout(Device, Handle->InternalLayout, GetVulkanAllocator());
}

void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorPool(Device, (VkDescriptorPool)Handle, GetVulkanAllocator());
}

void BmRender_DestroyPipeline(BmRender_Pipeline Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyPipeline(Device, (VkPipeline)Handle, GetVulkanAllocator());
	DestroyPipelineData(Handle);
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkShaderModule ShaderModule = (VkShaderModule)Handle;
	vkDestroyShaderModule(Device, ShaderModule, GetVulkanAllocator());
}

void BmRender_DestroyImage(BmRender_Image* Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyImage(Device, Handle->InternalImage, GetVulkanAllocator());
	vkFreeMemory(Device, (VkDeviceMemory)Handle->Memory, GetVulkanAllocator());
}

void BmRender_DestroyImageView(BmRender_ImageView Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyImageView(Device, Handle.InternalView, GetVulkanAllocator());
}

void BmRender_DestroyGPUBuffer(BmRender_GPUBuffer* Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyBuffer(Device, Handle->InternalBuffer, GetVulkanAllocator());
	vkFreeMemory(Device, (VkDeviceMemory)Handle->Memory, GetVulkanAllocator());
}

void BmRender_DestroyFence(BmRender_Fence Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyFence(Device, (VkFence)Handle, GetVulkanAllocator());
}

void BmRender_DestroySemaphore(BmRender_Semaphore Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroySemaphore(Device, Handle.InternalSemaphore, GetVulkanAllocator());
}

void BmRender_DestroyCommandPool(BmRender_CommandPool Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyCommandPool(Device, Handle.InternalPool, GetVulkanAllocator());
}

void BmRender_FreeCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkFreeCommandBuffers(Device, Handle.CommandPool->InternalPool, 1, &Handle.InternalBuffer);
}

BmRender_PipelineLayout BmRender_GetPipelineLayout(BmRender_Pipeline Handle)
{
	PipelineData Data;
	if (BmRender_GetPipelineData(Handle, &Data))
	{
		return Data.Layout;
	}
	return {};
}

#include "VulkanRender.h"

#include "VulkanHelper.h"
#include "VulkanCoreContext.h"

#include <forge_memory_debugger.h>
#include <cassert>

extern Memory_LinearAllocator FrameMemory;

static VulkanCoreContext CoreContext;
static VkAllocationCallbacks VulkanAllocator;

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

BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout* LayoutHandle, BmRender_DescriptorPool* PoolHandle)
{
	VkDevice Device = CoreContext.LogicalDevice;

	BmRender_DescriptorSet NewSet;
	NewSet.Layout = LayoutHandle;

	VkDescriptorPool Pool = *PoolHandle;

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
	assert(BindingsCount <= MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS);
	VkDevice Device = CoreContext.LogicalDevice;

	BmRender_DescriptorSetLayout Layout = { };
	Layout.BindingsCount = BindingsCount;

	VkDescriptorSetLayoutBinding* NewLayoutBindings = (VkDescriptorSetLayoutBinding*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkDescriptorSetLayoutBinding) * BindingsCount);
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

	VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, &VulkanAllocator, &Layout.InternalLayout));

	return Layout;
}

void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet* DescriptorSetHandle, const BmRender_DescriptorSetUpdateData* Bindings, u32 BindingsCount)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkWriteDescriptorSet) * BindingsCount);

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
			VkDescriptorBufferInfo* BufferInfo = (VkDescriptorBufferInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkDescriptorBufferInfo) * Binding.BindingCount);
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
			VkDescriptorImageInfo* ImageInfo = (VkDescriptorImageInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkDescriptorImageInfo));
			ImageInfo->imageLayout = ImageLayoutToVk(Binding.ImageBinding.ImageLayout);
			ImageInfo->imageView = Binding.ImageBinding.ImageView->InternalView;
			ImageInfo->sampler = Binding.ImageBinding.Sampler ? *Binding.ImageBinding.Sampler : nullptr;

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
	VkDevice Device = CoreContext.LogicalDevice;
	VkPhysicalDevice PhysicalDevice = CoreContext.PhysicalDevice;

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

	NewBuffer.InternalBuffer = CreateBuffer(Device, Capacity, Flag, &VulkanAllocator);

	VkBufferUsageFlags BufferUsageFlags = (VkBufferUsageFlags)Flag;
	DeviceMemoryAllocResult AllocResult = AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.InternalBuffer, MemoryFlag, BufferUsageFlags, &VulkanAllocator);
	NewBuffer.Memory = AllocResult.Memory;

	VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.InternalBuffer, (VkDeviceMemory)NewBuffer.Memory, 0));

	return NewBuffer;
}

static BmRender_Image CreateImageResource(BmRender_ImageDescription* Description)
{
	VkDevice Device = CoreContext.LogicalDevice;
	VkPhysicalDevice PhysicalDevice = CoreContext.PhysicalDevice;

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

	VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, &VulkanAllocator, &Resource.InternalImage));

	DeviceMemoryAllocResult AllocResult = AllocateDeviceMemory(PhysicalDevice, Device,
		Resource.InternalImage, MemoryPropertyFlag::GPULocal, &VulkanAllocator);

	Resource.Memory = AllocResult.Memory;
	Resource.Width = Description->Width;
	Resource.Height = Description->Height;
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

	VkDevice Device = CoreContext.LogicalDevice;
	VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, &VulkanAllocator, &View));

	BmRender_ImageView ImageViewData;
	ImageViewData.Image = Handle;
	ImageViewData.Format = Handle->Format;
	ImageViewData.InternalView = View;

	return ImageViewData;
}

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description)
{
	VkDevice Device = CoreContext.LogicalDevice;

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
	VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, &VulkanAllocator, &VulkanSampler));

	return VulkanSampler;
}

BmRender_Pipeline BmRender_CreatePipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_PipelineSettings* Settings, const BmRender_ShaderStageDescription* ShaderStageDescriptions,
	u32 ShaderStagesCount, const AttachmentData* Attachment)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkPipelineShaderStageCreateInfo* VkShaderStages = (VkPipelineShaderStageCreateInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkPipelineShaderStageCreateInfo) * ShaderStagesCount);
	for (u32 i = 0; i < ShaderStagesCount; ++i)
	{
		const BmRender_ShaderStageDescription* ShaderStageDesc = ShaderStageDescriptions + i;

		VkPipelineShaderStageCreateInfo* VkStage = VkShaderStages + i;
		VkStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VkStage->pNext = nullptr;
		VkStage->flags = 0;
		VkStage->stage = PipelineShaderStageToVkShaderStage(ShaderStageDesc->Stage);
		VkStage->module = *ShaderStageDesc->Shader;
		VkStage->pName = ShaderStageDesc->EntryPointFunction;
		VkStage->pSpecializationInfo = nullptr;
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { };
	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	bool SampleCountFound = false;
	VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_1_BIT;

	VkFormat* ColorAttachmentFormats = (VkFormat*)Memory_LinearAllocator_Alloc(&FrameMemory, Attachment->ColorAttachmentCount * sizeof(VkFormat));
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
	if (Attachment->DepthAttachment)
	{
		DepthAttachmentFormat = BmRender_FormatToVk(Attachment->DepthAttachment->Format);

		if (!SampleCountFound)
		{
			SampleCount = SampleCountToVk(Attachment->DepthAttachment->Image->SampleCount);
		}
	}

	VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	if (Attachment->StencilAttachment)
	{
		StencilAttachmentFormat = BmRender_FormatToVk(Attachment->StencilAttachment->Format);

		if (!SampleCountFound)
		{
			SampleCount = SampleCountToVk(Attachment->DepthAttachment->Image->SampleCount);
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

	auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkGraphicsPipelineCreateInfo));
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
	VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, &VulkanAllocator, &Pipeline));

	BmRender_Pipeline Data;
	Data.Layout = PipelineLayout;
	Data.InternalPipeline = Pipeline;
	return Data;
}

BmRender_Pipeline BmRender_CreateComputePipeline(BmRender_PipelineLayout PipelineLayout, const BmRender_ShaderStageDescription* ShaderStageDescription)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkPipelineShaderStageCreateInfo VkShaderStage = { };
	VkShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VkShaderStage.pNext = nullptr;
	VkShaderStage.flags = 0;
	VkShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	VkShaderStage.module = *ShaderStageDescription->Shader;
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
	VULKAN_CHECK_RESULT(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, &VulkanAllocator, &Pipeline));

	BmRender_Pipeline Data;
	Data.Layout = PipelineLayout;
	Data.InternalPipeline = Pipeline;
	return Data;
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkDescriptorSetLayout* VkSetLayouts = (VkDescriptorSetLayout*)Memory_LinearAllocator_Alloc(&FrameMemory, Description->SetLayoutCount * sizeof(VkDescriptorSetLayout));
	for (u32 i = 0; i < Description->SetLayoutCount; ++i)
	{
		VkSetLayouts[i] = Description->SetLayouts[i].InternalLayout;
	}

	VkPushConstantRange* VkPushConstantRanges = (VkPushConstantRange*)Memory_LinearAllocator_Alloc(&FrameMemory, Description->PushConstantRangeCount * sizeof(VkPushConstantRange));
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
	VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, &VulkanAllocator, &LayoutData.InternalLayout));
	LayoutData.PipelineType = Description->PipelineType;

	return LayoutData;
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolSize* PoolSizes, u32 MaxSets, u32 PoolSizeCount, BmRender_DescriptorPoolType Type)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkDescriptorPoolSize* VkPoolSizes = (VkDescriptorPoolSize*)Memory_LinearAllocator_Alloc(&FrameMemory, PoolSizeCount * sizeof(VkDescriptorPoolSize));
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
	VULKAN_CHECK_RESULT(vkCreateDescriptorPool(Device, &CreateInfo, &VulkanAllocator, &DescriptorPool));

	return DescriptorPool;
}

BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkShaderModuleCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.codeSize = Description->CodeSize;
	CreateInfo.pCode = Description->Code;

	VkShaderModule ShaderModule;
	VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &CreateInfo, &VulkanAllocator, &ShaderModule));

	return ShaderModule;
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
	VkDevice Device = CoreContext.LogicalDevice;

	VkFenceCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	CreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence Fence;
	VULKAN_CHECK_RESULT(vkCreateFence(Device, &CreateInfo, &VulkanAllocator, &Fence));

	return Fence;
}

BmRender_Semaphore BmRender_CreateSemaphore()
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkSemaphoreCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;

	BmRender_Semaphore Data;
	Data.Type = BmRender_SemaphoreType::Binary;
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, &VulkanAllocator, &Data.InternalSemaphore));

	return Data;
}

BmRender_Semaphore BmRender_CreateTimelineSemaphore(u64 InitialValue)
{
	VkDevice Device = CoreContext.LogicalDevice;

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
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, &VulkanAllocator, &Data.InternalSemaphore));

	return Data;
}

bool BmRender_IsDedicatedQueuePresent(BmRender_QueueType BmRender_QueueType)
{
	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext.Indices);

	if (SelectedFamilyIndex == -1)
	{
		return false;
	}

	bool NeedsTransfer = ((u32)BmRender_QueueType & (u32)BmRender_QueueType::Transfer) != 0;
	bool NeedsGraphics = ((u32)BmRender_QueueType & (u32)BmRender_QueueType::Graphic) != 0;

	// If it's a transfer-only queue, check if it's dedicated (different from graphics)
	if (NeedsTransfer && !NeedsGraphics)
	{
		return CoreContext.Indices.TransferFamily != CoreContext.Indices.GraphicsFamily;
	}

	// Graphics queue is always present if we got a valid index
	return true;
}

BmRender_Queue BmRender_CreateQueue(BmRender_QueueType BmRender_QueueType)
{
	VkDevice Device = CoreContext.LogicalDevice;

	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext.Indices);

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
	VkDevice Device = CoreContext.LogicalDevice;

	s32 SelectedFamilyIndex = GetQueueFamilyIndexFromQueueType(BmRender_QueueType, CoreContext.Indices);

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
	VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &CreateInfo, &VulkanAllocator, &Data.InternalPool));

	return Data;
}

BmRender_CommandBuffer BmRender_AllocateCommandBuffer(const BmRender_CommandPool* CommandPool)
{
	VkDevice Device = CoreContext.LogicalDevice;
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
	VkDevice Device = CoreContext.LogicalDevice;
	UpdateHostCompatibleBufferMemory(Device, (VkDeviceMemory)Buffer->Memory, DataSize, BufferOffset, Data);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyPipelineLayout(Device, Handle.InternalLayout, &VulkanAllocator);
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroySampler(Device, (VkSampler)Handle, &VulkanAllocator);
}

void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout* Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyDescriptorSetLayout(Device, Handle->InternalLayout, &VulkanAllocator);
}

void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyDescriptorPool(Device, (VkDescriptorPool)Handle, &VulkanAllocator);
}

void BmRender_DestroyPipeline(BmRender_Pipeline Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyPipeline(Device, Handle.InternalPipeline, &VulkanAllocator);
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	VkShaderModule ShaderModule = (VkShaderModule)Handle;
	vkDestroyShaderModule(Device, ShaderModule, &VulkanAllocator);
}

void BmRender_DestroyImage(BmRender_Image* Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyImage(Device, Handle->InternalImage, &VulkanAllocator);
	vkFreeMemory(Device, (VkDeviceMemory)Handle->Memory, &VulkanAllocator);
}

void BmRender_DestroyImageView(BmRender_ImageView Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyImageView(Device, Handle.InternalView, &VulkanAllocator);
}

void BmRender_DestroyGPUBuffer(BmRender_GPUBuffer* Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyBuffer(Device, Handle->InternalBuffer, &VulkanAllocator);
	vkFreeMemory(Device, (VkDeviceMemory)Handle->Memory, &VulkanAllocator);
}

void BmRender_DestroyFence(BmRender_Fence Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyFence(Device, (VkFence)Handle, &VulkanAllocator);
}

void BmRender_DestroySemaphore(BmRender_Semaphore Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroySemaphore(Device, Handle.InternalSemaphore, &VulkanAllocator);
}

void BmRender_DestroyCommandPool(BmRender_CommandPool Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDestroyCommandPool(Device, Handle.InternalPool, &VulkanAllocator);
}

void BmRender_FreeCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkFreeCommandBuffers(Device, Handle.CommandPool->InternalPool, 1, &Handle.InternalBuffer);
}

BmRender_FenceStatus BmRender_GetFenceStatus(BmRender_Fence Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
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

BmRender_WaitResult BmRender_WaitForFences(BmRender_Fence Handle, bool WaitAll, u64 Timeout)
{
	VkDevice Device = CoreContext.LogicalDevice;
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

u64 BmRender_GetBufferDeviceAddress(BmRender_GPUBuffer* Buffer)
{
	VkDevice Device = CoreContext.LogicalDevice;
	VkBuffer VkBufferHandle = Buffer->InternalBuffer;

	VkBufferDeviceAddressInfo AddressInfo = {};
	AddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	AddressInfo.buffer = VkBufferHandle;

	return vkGetBufferDeviceAddress(Device, &AddressInfo);
}

void BmRender_ResetFences(BmRender_Fence Handle)
{
	VkDevice Device = CoreContext.LogicalDevice;
	VkFence Fence = (VkFence)Handle;
	VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &Fence));
}

void BmRender_GetSemaphoreCounterValue(BmRender_Semaphore Handle, u64* pValue)
{
	VkDevice Device = CoreContext.LogicalDevice;
	VkSemaphore VulkanSemaphore = Handle.InternalSemaphore;
	VULKAN_CHECK_RESULT(vkGetSemaphoreCounterValue(Device, VulkanSemaphore, pValue));
}

void BmRender_BeginCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkCommandBufferBeginInfo BeginInfo = { };
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = 0;

	VkCommandBuffer VulkanCommandBuffer = Handle.InternalBuffer;
	VULKAN_CHECK_RESULT(vkBeginCommandBuffer(VulkanCommandBuffer, &BeginInfo));
}

void BmRender_EndCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkCommandBuffer VulkanCommandBuffer = Handle.InternalBuffer;
	VULKAN_CHECK_RESULT(vkEndCommandBuffer(VulkanCommandBuffer));
}

void BmRender_QueueWaitIdle(BmRender_Queue Queue)
{
	vkQueueWaitIdle(Queue.InternalQueue);
}

void BmRender_DeviceWaitIdle()
{
	VkDevice Device = CoreContext.LogicalDevice;
	vkDeviceWaitIdle(Device);
}

void BmRender_TransitionImageForRendering(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer, u32 LayersCount)
{
	VkImageAspectFlags AspectFlags;
	VkPipelineStageFlags2 DstStageMask;
	VkAccessFlags2 DstAccessMask;
	VkImageLayout NewLayout;
	switch (Image->Type)
	{
	case BmRender_ImageType::ColorAttachmentSampled:
	case BmRender_ImageType::TransferSampled:
	case BmRender_ImageType::MultiSampledColorAttachment:
		DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		DstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		NewLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		break;

	case BmRender_ImageType::DepthSamplad:
	case BmRender_ImageType::MultiSampledDepthAttachment:
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
	Barrier.image = Image->InternalImage;
	Barrier.subresourceRange.aspectMask = AspectFlags;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;

	VkDependencyInfo DepInfo = { };
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

	vkCmdPipelineBarrier2(CommandBuffer.InternalBuffer, &DepInfo);
}

void BmRender_TransitionImageForSampling(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer, u32 LayersCount)
{
	VkImageAspectFlags AspectFlags;
	VkPipelineStageFlags2 SrcStageMask;
	VkAccessFlags2 SrcAccessMask;
	VkImageLayout OldLayout;
	switch (Image->Type)
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
	Barrier.image = Image->InternalImage;
	Barrier.subresourceRange.aspectMask = AspectFlags;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;

	VkDependencyInfo DepInfo = { };
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

	vkCmdPipelineBarrier2(CommandBuffer.InternalBuffer, &DepInfo);
}

void BmRender_TransitionImageForPresentation(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer, u32 LayersCount)
{
	VkImageMemoryBarrier2 Barrier = { };
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image->InternalImage;
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

	vkCmdPipelineBarrier2(CommandBuffer.InternalBuffer, &DepInfo);
}

void BmRender_TransitionImageForComputeWrite(BmRender_CommandBuffer CommandBuffer, BmRender_Image* Image, u32 BaseLayer, u32 LayersCount)
{
	VkImageMemoryBarrier2 Barrier = {};
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	Barrier.srcAccessMask = VK_ACCESS_2_NONE;

	Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	Barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;

	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	Barrier.image = Image->InternalImage;

	Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = BaseLayer;
	Barrier.subresourceRange.layerCount = LayersCount;

	VkDependencyInfo DepInfo = {};
	DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &Barrier;

	vkCmdPipelineBarrier2(CommandBuffer.InternalBuffer, &DepInfo);
}

void BmRender_RecordUpdateGPULocalBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* DstBuffer, BmRender_GPUBuffer* SrcBuffer, u64 SrcOffset, u64 DstOffset, u64 DataSize)
{
	VkBufferCopy CopyRegion = { };
	CopyRegion.srcOffset = SrcOffset;
	CopyRegion.dstOffset = DstOffset;
	CopyRegion.size = DataSize;

	vkCmdCopyBuffer(CommandBuffer.InternalBuffer, SrcBuffer->InternalBuffer, DstBuffer->InternalBuffer, 1, &CopyRegion);
}

void BmRender_BeginRendering(BmRender_CommandBuffer CommandBuffer, const BmRender_RenderingInfo* pRenderingInfo)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;

	VkRenderingAttachmentInfo* ColorAttachments = nullptr;
	VkRenderingAttachmentInfo* DepthAttachment = nullptr;

	ColorAttachments = (VkRenderingAttachmentInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkRenderingAttachmentInfo) * pRenderingInfo->ColorAttachmentCount);
	for (u32 i = 0; i < pRenderingInfo->ColorAttachmentCount; ++i)
	{
		const BmRender_RenderingColorAttachment& Attachment = pRenderingInfo->ColorAttachments[i];

		VkRenderingAttachmentInfo* VkAttachment = ColorAttachments + i;
		*VkAttachment = { };
		VkAttachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		VkAttachment->imageView = Attachment.ImageView->InternalView;
		VkAttachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachment->loadOp = AttachmentLoadOpToVk(Attachment.LoadOp);
		VkAttachment->storeOp = AttachmentStoreOpToVk(Attachment.StoreOp);
		VkAttachment->clearValue.color = ClearColorValueToVk(Attachment.ClearValue);

		VkAttachment->resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachment->resolveImageView = Attachment.ResolveImageView ? Attachment.ResolveImageView->InternalView : nullptr;

		if (Attachment.ImageView->Image->SampleCount > BmRender_SampleCount::Count1)
		{
			VkAttachment->resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		}
	}

	VkRenderingAttachmentInfo DepthAttachmentInfo = { };
	if (pRenderingInfo->DepthAttachment != nullptr)
	{
		const BmRender_RenderingDepthAttachment& Attachment = *pRenderingInfo->DepthAttachment;

		DepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachmentInfo.imageView = Attachment.ImageView->InternalView;
		DepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentInfo.loadOp = AttachmentLoadOpToVk(Attachment.LoadOp);
		DepthAttachmentInfo.storeOp = AttachmentStoreOpToVk(Attachment.StoreOp);
		DepthAttachmentInfo.clearValue.depthStencil = ClearDepthStencilValueToVk(Attachment.ClearValue);

		DepthAttachment = &DepthAttachmentInfo;
	}

	VkRenderingInfo RenderingInfo = { };
	RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	RenderingInfo.renderArea.offset = Offset2DToVk(pRenderingInfo->Offset);
	RenderingInfo.renderArea.extent = Extent2DToVk(pRenderingInfo->Extent);
	RenderingInfo.layerCount = 1;
	RenderingInfo.colorAttachmentCount = pRenderingInfo->ColorAttachmentCount;
	RenderingInfo.pColorAttachments = ColorAttachments;
	RenderingInfo.pDepthAttachment = DepthAttachment;
	RenderingInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(VkCmdBuffer, &RenderingInfo);
}

void BmRender_BindPipeline(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	BmRender_PipelineLayout PipelineLayout = Pipeline.Layout;
	BindPoint = PipelineTypeToVkPipelineBindPoint(PipelineLayout.PipelineType);

	vkCmdBindPipeline(VkCmdBuffer, BindPoint, Pipeline.InternalPipeline);
}

void BmRender_RecordPushConstants(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline, BmRender_DescriptorShaderStage StageFlags, u32 Offset, u32 Size, const void* pValues)
{
	BmRender_PipelineLayout PipelineLayout = Pipeline.Layout;
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	vkCmdPushConstants(VkCmdBuffer, PipelineLayout.InternalLayout, ShaderStageFlagsToVk(StageFlags), Offset, Size, pValues);
}

void BmRender_RecordBindDescriptorSets(BmRender_CommandBuffer CommandBuffer, BmRender_Pipeline Pipeline, u32 FirstSet, u32 DescriptorSetCount, const BmRender_DescriptorSet* pDescriptorSets, u32 DynamicOffsetCount, const u32* pDynamicOffsets)
{
	BmRender_PipelineLayout PipelineLayout = Pipeline.Layout;

	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	VkPipelineLayout Layout = PipelineLayout.InternalLayout;

	VkPipelineBindPoint BindPoint = PipelineTypeToVkPipelineBindPoint(PipelineLayout.PipelineType);

	VkDescriptorSet* Sets = (VkDescriptorSet*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkDescriptorSet) * DescriptorSetCount);
	for (u32 i = 0; i < DescriptorSetCount; ++i)
	{
		Sets[i] = pDescriptorSets[i].InternalSet;
	}

	vkCmdBindDescriptorSets(VkCmdBuffer, BindPoint, Layout, FirstSet, DescriptorSetCount,
		Sets, DynamicOffsetCount, pDynamicOffsets);
}

void BmRender_RecordBindVertexBuffers(BmRender_CommandBuffer CommandBuffer, u32 FirstBinding, u32 BindingCount, const BmRender_GPUBuffer* Buffers, const u64* Offsets)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	const VkBuffer* VkBuffers = (const VkBuffer*)Buffers;

	vkCmdBindVertexBuffers(VkCmdBuffer, FirstBinding, BindingCount, VkBuffers, Offsets);
}

void BmRender_RecordBindIndexBuffer(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* Buffer, u64 Offset, BmRender_IndexType IndexType)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	VkBuffer VkBufferHandle = Buffer->InternalBuffer;
	vkCmdBindIndexBuffer(VkCmdBuffer, VkBufferHandle, Offset, IndexTypeToVk(IndexType));
}

void BmRender_Draw(BmRender_CommandBuffer CommandBuffer, u32 VertexCount, u32 InstanceCount, u32 FirstVertex, u32 FirstInstance)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	vkCmdDraw(VkCmdBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void BmRender_DrawIndexed(BmRender_CommandBuffer CommandBuffer, u32 IndexCount, u32 InstanceCount, u32 FirstIndex, u32 VertexOffset, u32 FirstInstance)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	vkCmdDrawIndexed(VkCmdBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void BmRender_RecordDrawIndexedIndirect(BmRender_CommandBuffer CommandBuffer, BmRender_GPUBuffer* IndirectBuffer, u64 Offset, u32 DrawCount, u32 Stride)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	VkBuffer VkIndirectBuffer = IndirectBuffer->InternalBuffer;
	vkCmdDrawIndexedIndirect(VkCmdBuffer, VkIndirectBuffer, Offset, DrawCount, Stride);

}

void BmRender_RecordDispatch(BmRender_CommandBuffer CommandBuffer, u32 GroupCountX, u32 GroupCountY, u32  GroupCountZ)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	vkCmdDispatch(VkCmdBuffer, GroupCountX, GroupCountY, GroupCountZ);
}

void BmRender_EndRendering(BmRender_CommandBuffer CommandBuffer)
{
	VkCommandBuffer VkCmdBuffer = CommandBuffer.InternalBuffer;
	vkCmdEndRendering(VkCmdBuffer);
}

void BmRender_QueueSubmit(BmRender_Queue Queue, u32 SubmitCount, const BmRender_SubmitInfo* Submits, BmRender_Fence Fence)
{
	VkFence VkFenceHandle = (VkFence)Fence;
	VkSubmitInfo* VkSubmits = (VkSubmitInfo*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkSubmitInfo) * SubmitCount);

	for (u32 i = 0; i < SubmitCount; ++i)
	{
		const BmRender_SubmitInfo& Submit = Submits[i];
		VkSubmitInfo& VkSubmit = VkSubmits[i];

		u32 TotalWaitSemaphoreCount = Submit.WaitSemaphoreCount + Submit.WaitTimelineSemaphoreCount;
		u32 TotalSignalSemaphoreCount = Submit.SignalSemaphoreCount + Submit.SignalTimelineSemaphoreCount;

		VkTimelineSemaphoreSubmitInfo NewTimelineInfo = { };
		NewTimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		NewTimelineInfo.pNext = nullptr;
		NewTimelineInfo.waitSemaphoreValueCount = TotalWaitSemaphoreCount;
		NewTimelineInfo.signalSemaphoreValueCount = TotalSignalSemaphoreCount;

		VkPipelineStageFlags* WaitDstStageFlags = nullptr;
		if (Submit.WaitDstStageFlags != nullptr)
		{
			WaitDstStageFlags = (VkPipelineStageFlags*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkPipelineStageFlags) * TotalWaitSemaphoreCount);
			for (u32 j = 0; j < TotalWaitSemaphoreCount; ++j)
			{
				WaitDstStageFlags[j] = PipelineStageFlagsToVk(Submit.WaitDstStageFlags[j]);
			}
		}

		VkSubmit = { };
		VkSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSubmit.pNext = &NewTimelineInfo;
		VkSubmit.waitSemaphoreCount = TotalWaitSemaphoreCount;
		VkSubmit.pWaitDstStageMask = WaitDstStageFlags;
		VkSubmit.commandBufferCount = Submit.CommandBufferCount;
		VkSubmit.signalSemaphoreCount = TotalSignalSemaphoreCount;

		u64* WaitValues = (u64*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(u64) * TotalWaitSemaphoreCount);
		u64* SignalValues = (u64*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(u64) * TotalSignalSemaphoreCount);
		VkSemaphore* WaitSemaphores = (VkSemaphore*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkSemaphore) * TotalWaitSemaphoreCount);
		VkSemaphore* SignalSemaphores = (VkSemaphore*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkSemaphore) * TotalSignalSemaphoreCount);

		NewTimelineInfo.pWaitSemaphoreValues = WaitValues;
		NewTimelineInfo.pSignalSemaphoreValues = SignalValues;
		VkSubmit.pCommandBuffers = (VkCommandBuffer*)Submit.CommandBuffers;
		VkSubmit.pWaitSemaphores = WaitSemaphores;
		VkSubmit.pSignalSemaphores = SignalSemaphores;

		for (u32 j = 0; j < Submit.WaitSemaphoreCount; ++j)
		{
			WaitValues[j] = 0;
			WaitSemaphores[j] = Submit.WaitSemaphores[j].InternalSemaphore;
		}

		for (u32 j = 0; j < Submit.WaitTimelineSemaphoreCount; ++j)
		{
			WaitValues[Submit.WaitSemaphoreCount + j] = Submit.WaitTimelineSemaphores[j].Value;
			WaitSemaphores[Submit.WaitSemaphoreCount + j] = Submit.WaitTimelineSemaphores[j].Semaphore->InternalSemaphore;
		}

		for (u32 j = 0; j < Submit.SignalSemaphoreCount; ++j)
		{
			SignalValues[j] = 0;
			SignalSemaphores[j] = Submit.SignalSemaphores[j].InternalSemaphore;
		}

		for (u32 j = 0; j < Submit.SignalTimelineSemaphoreCount; ++j)
		{
			SignalValues[Submit.SignalSemaphoreCount + j] = Submit.SignalTimelineSemaphores[j].Value;
			SignalSemaphores[Submit.SignalSemaphoreCount + j] = (VkSemaphore)Submit.SignalTimelineSemaphores[j].Semaphore->InternalSemaphore;
		}
	}

	VkQueue VkQueueHandle = Queue.InternalQueue;
	VULKAN_CHECK_RESULT(vkQueueSubmit(VkQueueHandle, SubmitCount, VkSubmits, VkFenceHandle));
}

BmRender_SwapchainResult BmRender_QueuePresent(BmRender_Queue Queue, const BmRender_PresentInfo* pPresentInfo)
{
	VkPresentInfoKHR PresentInfo = { };
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.waitSemaphoreCount = pPresentInfo->WaitSemaphoreCount;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &CoreContext.VulkanSwapchain;
	PresentInfo.pImageIndices = pPresentInfo->ImageIndices;

	VkSemaphore* WaitSemaphores = (VkSemaphore*)Memory_LinearAllocator_Alloc(&FrameMemory, sizeof(VkSemaphore) * pPresentInfo->WaitSemaphoreCount);
	PresentInfo.pWaitSemaphores = WaitSemaphores;

	for (u32 i = 0; i < pPresentInfo->WaitSemaphoreCount; ++i)
	{
		WaitSemaphores[i] = pPresentInfo->WaitSemaphores[i].InternalSemaphore;
	}

	VkQueue VkQueueHandle = Queue.InternalQueue;
	VkResult Result = vkQueuePresentKHR(VkQueueHandle, &PresentInfo);

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

BmRender_SwapchainResult BmRender_AcquireNextSwapchainImage(u64 Timeout, BmRender_Semaphore Semaphore, BmRender_Fence Fence, u32* pImageIndex)
{
	VkDevice Device = CoreContext.LogicalDevice;

	VkSwapchainKHR Swapchain = CoreContext.VulkanSwapchain;
	VkFence VkFenceHandle = Fence != nullptr ? (VkFence)Fence : VK_NULL_HANDLE;
	VkResult Result = vkAcquireNextImageKHR(Device, Swapchain, Timeout, Semaphore.InternalSemaphore, VkFenceHandle, pImageIndex);

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

u32 BmRender_GetSwapchainImageCount()
{
	return CoreContext.ImagesCount;
}

BmRender_SurfaceFormat BmRender_GetSurfaceFormat()
{
	return VkSurfaceFormatToBmRender(CoreContext.SurfaceFormat);
}

BmRender_Image* BmRender_GetSwapchainImage(u32 Index)
{
	if (Index >= CoreContext.ImagesCount)
	{
		return nullptr;
	}
	return CoreContext.Images + Index;
}

BmRender_ImageView* BmRender_GetSwapchainImageView(u32 Index)
{
	if (Index >= CoreContext.ImagesCount)
	{
		return { };
	}
	return CoreContext.ImageViews + Index;
}

BmRender_Dimensions BmRender_GetSwapchainExtent()
{
	return VkExtent2DToBmRender(CoreContext.SwapExtent);
}

BmRender_Instance BmRender_GetVulkanInstance()
{
	return (BmRender_Instance)CoreContext.VulkanInstance;
}

BmRender_PhysicalDevice BmRender_GetPhysicalDevice()
{
	return (BmRender_PhysicalDevice)CoreContext.PhysicalDevice;
}

BmRender_Device BmRender_GetLogicalDevice()
{
	return (BmRender_Device)CoreContext.LogicalDevice;
}

u32 BmRender_GetGraphicsQueueFamily()
{
	return (u32)CoreContext.Indices.GraphicsFamily;
}

u32 BmRender_GetQueueFamily(BmRender_Queue Queue)
{
	s32 FamilyIndex = GetQueueFamilyIndexFromQueueType(Queue.QueueType, CoreContext.Indices);
	if (FamilyIndex != -1)
	{
		return (u32)FamilyIndex;
	}

	return 0;
}

VkAllocationCallbacks* BmRender_GetVulkanAllocator()
{
	return &VulkanAllocator;
}

void InitBackend(GLFWwindow* WindowHandler)
{
	VulkanAllocator.pUserData = nullptr;
	VulkanAllocator.pfnAllocation = VulkanAllocationCallback;
	VulkanAllocator.pfnReallocation = VulkanReallocationCallback;
	VulkanAllocator.pfnFree = VulkanFreeCallback;
	VulkanAllocator.pfnInternalAllocation = VulkanInternalAllocationNotification;
	VulkanAllocator.pfnInternalFree = VulkanInternalFreeNotification;

	CreateCoreContext(&CoreContext, WindowHandler);
}

void DeInitBackend()
{
	DestroyCoreContext(&CoreContext);
}

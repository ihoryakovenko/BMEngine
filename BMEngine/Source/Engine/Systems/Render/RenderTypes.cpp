#include "RenderTypes.h"

#include <Engine/Systems/HandleManager.h>
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Handles.h"

#include "VulkanCoreContext.h"

#include "Util/Util.h"

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
	free(pMemory);
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

static VulkanCoreContext::VulkanCoreContext CoreContext;

static Memory::FrameMemory FrameMemory;

static VkAllocationCallbacks VulkanAllocator;



void CreateCoreContext(GLFWwindow* WindowHandler)
{
	VulkanAllocator.pUserData = nullptr;
	VulkanAllocator.pfnAllocation = VulkanAllocationCallback;
	VulkanAllocator.pfnReallocation = VulkanReallocationCallback;
	VulkanAllocator.pfnFree = VulkanFreeCallback;
	VulkanAllocator.pfnInternalAllocation = VulkanInternalAllocationNotification;
	VulkanAllocator.pfnInternalFree = VulkanInternalFreeNotification;

	VulkanCoreContext::CreateCoreContext(&CoreContext, WindowHandler);
}

void DestroyCoreContext()
{
	VulkanCoreContext::DestroyCoreContext(&CoreContext);
}

VulkanCoreContext::VulkanCoreContext* GetCoreContext()
{
	return &CoreContext;
}

VkAllocationCallbacks* GetVulkanAllocator()
{
	return &VulkanAllocator;
}

Memory::FrameMemory GetFrameMemory()
{
	return FrameMemory;
}

void InitializeFrameMemory()
{
	FrameMemory = Memory::CreateFrameMemory(1024 * 1024);
}

void DeinitFrameMemory()
{
	Memory::DestroyFrameMemory(FrameMemory);
}



BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetData NewSet;
	NewSet.Layout = LayoutHandle;

	VkDescriptorPool Pool = (VkDescriptorPool)PoolHandle;

	VkDescriptorSetLayout VkLayout = (VkDescriptorSetLayout)LayoutHandle;
	VkDescriptorSetAllocateInfo AllocInfo = { };
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = Pool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &VkLayout;

	VkDescriptorSet Set;
	VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, &Set));
	NewSet.Set = Set;

	return CreateDescriptorSetHandle(Set, &NewSet);
}

BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutBinding* Bindings, u64 BindingsCount)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetLayoutData Layout = { };
	Layout.BindingsCount = BindingsCount;
	Layout.LayoutBindings = (DescriptorSetLayoutBinding*)malloc(sizeof(DescriptorSetLayoutBinding) * BindingsCount);

	VkDescriptorSetLayoutBinding* NewLayoutBindings = (VkDescriptorSetLayoutBinding*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkDescriptorSetLayoutBinding) * BindingsCount);
	for (u32 i = 0; i < BindingsCount; ++i)
	{
		NewLayoutBindings[i].binding = i;
		NewLayoutBindings[i].descriptorCount = Bindings[i].DescriptorCount;
		NewLayoutBindings[i].descriptorType = Bindings[i].DescriptorType;
		NewLayoutBindings[i].stageFlags = VulkanHelper::DescriptorShaderStageToVkShaderStage(Bindings[i].StageFlags);
		NewLayoutBindings[i].pImmutableSamplers = nullptr;

		Layout.LayoutBindings[i].DescriptorType = NewLayoutBindings[i].descriptorType;
	}

	VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
	LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutCreateInfo.bindingCount = BindingsCount;
	LayoutCreateInfo.pBindings = NewLayoutBindings;
	LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	LayoutCreateInfo.pNext = nullptr;

	VkDescriptorSetLayout VkLayout;
	VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, GetVulkanAllocator(), &VkLayout));

	return CreateDescriptorSetLayoutHandle(VkLayout, &Layout);
}

void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetData Set;
	GetDescriptorSetData(DescriptorSetHandle, &Set);
	DescriptorSetLayoutData Layout;
	GetDescriptorSetLayoutData(Set.Layout, &Layout);

	VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkWriteDescriptorSet) * BindingsCount);

	for (u32 i = 0; i < BindingsCount; i++)
	{
		const BmRender_DescriptorSetBinding& Binding = Bindings[i];

		VkDescriptorType DescriptorType = Layout.LayoutBindings[i].DescriptorType;

		WriteDescriptorSets[i] = { };
		WriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[i].dstSet = Set.Set;
		WriteDescriptorSets[i].dstBinding = i;
		WriteDescriptorSets[i].dstArrayElement = Binding.DstArrayElement;
		WriteDescriptorSets[i].descriptorType = DescriptorType;
		WriteDescriptorSets[i].descriptorCount = Binding.BindingCount;

		if (DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			VkDescriptorBufferInfo* BufferInfo = (VkDescriptorBufferInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkDescriptorBufferInfo) * Binding.BindingCount);
			for (u32 j = 0; j < Binding.BindingCount; ++j)
			{
				const BmRender_GPUBufferBinding& Entry = Binding.BufferRegions[j];

				BufferInfo[j].buffer = (VkBuffer)Entry.GPUBufferHandle;
				BufferInfo[j].offset = Entry.BufferOffset;
				BufferInfo[j].range = Entry.Size;
			}

			WriteDescriptorSets[i].pBufferInfo = BufferInfo;
		}
		else if (VK_DESCRIPTOR_TYPE_SAMPLER || VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			VkDescriptorImageInfo* ImageInfo = (VkDescriptorImageInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkDescriptorImageInfo));
			ImageInfo->imageLayout = Binding.ImageBinding.ImageLayout;
			ImageInfo->imageView = (VkImageView)Binding.ImageBinding.ImageView;
			ImageInfo->sampler = (VkSampler)Binding.ImageBinding.Sampler;

			WriteDescriptorSets[i].pImageInfo = ImageInfo;
		}
		else
		{
			assert(false || "Unimplemented");
		}
	}

	vkUpdateDescriptorSets(Device, BindingsCount, WriteDescriptorSets, 0, nullptr);
}

BmRender_PushConstant BmRender_CreatePushConstant(BmRender_DescriptorShaderStage Stage, u32 Offset, u32 Size)
{
	BmRender_PushConstant Constant;
	Constant.offset = Offset;
	Constant.size = Size;
	Constant.stageFlags = VulkanHelper::DescriptorShaderStageToVkShaderStage(Stage);

	return Constant;
}

static BmRender_GPUBuffer CreateGPUBuffer(u64 Capacity, MemoryPropertyFlag MemoryFlag, BmRender_PipelineSyncStage BufferStage, BufferUsageFlag Flag)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

	GPUBufferData NewBuffer = { };

	if (MemoryFlag == MemoryPropertyFlag::HostCompatible)
	{
		NewBuffer.ReadyValue = 0;
	}
	else
	{
		NewBuffer.ReadyValue = ULLONG_MAX;
	}

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
	NewBuffer.BufferStage = BufferStage;

	VkBuffer Buffer = VulkanHelper::CreateBuffer(Device, Capacity, Flag, GetVulkanAllocator());

	VkBufferUsageFlags BufferUsageFlags = (VkBufferUsageFlags)Flag;
	VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Buffer, MemoryFlag, BufferUsageFlags, GetVulkanAllocator());
	NewBuffer.Memory = AllocResult.Memory;

	VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, Buffer, NewBuffer.Memory, 0));

	return CreateGPUBufferHandle(Buffer, &NewBuffer);
}

static BmRender_Image CreateImageResource(BmRender_ImageDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

	ImageResource Resource;
	Resource.ReadyValue = ULLONG_MAX;
	Resource.Format = Description->Format;
	Resource.Type = Description->Type;

	VkImageUsageFlags Usage;

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
	ImageCreateInfo.format = Description->Format;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.usage = Usage;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.flags = 0;

	VkImage Image;
	VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, GetVulkanAllocator(), &Image));

	VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
		Image, MemoryPropertyFlag::GPULocal, GetVulkanAllocator());

	Resource.Memory = AllocResult.Memory;
	Resource.Width = Description->Width;
	Resource.Height = Description->Height;
	Resource.Size = AllocResult.Size;

	VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Image, Resource.Memory, 0));
	return CreateImageHandle(Image, &Resource);
}

static BmRender_ImageView CreateImageView(BmRender_Image Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType)
{
	ImageResource Resource;
	GetImageData(Handle, &Resource);
	VkImageView View;

	VkImageViewCreateInfo ViewCreateInfo = { };
	ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewCreateInfo.flags = 0;
	ViewCreateInfo.viewType = ViewType;
	ViewCreateInfo.format = Resource.Format;
	ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.subresourceRange.aspectMask = VulkanHelper::ImageTypeToVkImageAspectFlags(Resource.Type);
	ViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ViewCreateInfo.subresourceRange.levelCount = 1;
	ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
	ViewCreateInfo.subresourceRange.layerCount = LayerCount;
	ViewCreateInfo.image = (VkImage)Handle;

	VkDevice Device = GetCoreContext()->LogicalDevice;
	VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, GetVulkanAllocator(), &View));

	return CreateImageViewHandle(View);
}

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkSamplerCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.magFilter = Description->MagFilter;
	CreateInfo.minFilter = Description->MinFilter;
	CreateInfo.mipmapMode = Description->MipmapMode;
	CreateInfo.addressModeU = Description->AddressModeU;
	CreateInfo.addressModeV = Description->AddressModeV;
	CreateInfo.addressModeW = Description->AddressModeW;
	CreateInfo.mipLodBias = Description->MipLodBias;
	CreateInfo.anisotropyEnable = Description->AnisotropyEnable;
	CreateInfo.maxAnisotropy = Description->MaxAnisotropy;
	CreateInfo.compareEnable = Description->CompareEnable;
	CreateInfo.compareOp = Description->CompareOp;
	CreateInfo.minLod = Description->MinLod;
	CreateInfo.maxLod = Description->MaxLod;
	CreateInfo.borderColor = Description->BorderColor;
	CreateInfo.unnormalizedCoordinates = Description->UnnormalizedCoordinates;

	VkSampler VulkanSampler;
	VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, GetVulkanAllocator(), &VulkanSampler));

	return CreateSamplerHandle(VulkanSampler);
}

BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkVertexInputBindingDescription* VkVertexBindings = (VkVertexInputBindingDescription*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkVertexInputBindingDescription) * Description->VertexBindingsCount);

	u32 TotalAttributes = 0;
	VkVertexInputAttributeDescription* VkVertexAttributes = (VkVertexInputAttributeDescription*)Memory::GetHead(GetFrameMemory());
	VkVertexInputAttributeDescription* VkAttribute = nullptr;

	u32 CurrentLocation = 0;
	for (u32 BindingIndex = 0; BindingIndex < Description->VertexBindingsCount; ++BindingIndex)
	{
		const BmRender_VertexBinding& VertexBinding = Description->VertexBindings[BindingIndex];

		VkVertexInputBindingDescription* VkBinding = VkVertexBindings + BindingIndex;
		VkBinding->binding = BindingIndex;
		VkBinding->stride = VertexBinding.Stride;
		VkBinding->inputRate = VertexBinding.InputRate;

		for (u32 AttrIndex = 0; AttrIndex < VertexBinding.AttributesCount; ++AttrIndex)
		{
			const VertexAttribute& attribute = VertexBinding.Attributes[AttrIndex];
			if (attribute.Type != BmRender_AttributeType::Mat4)
			{
				VkFormat Format;
				switch (attribute.Type)
				{
					case BmRender_AttributeType::Int:
						Format = VK_FORMAT_R32_SINT;
						break;
					case BmRender_AttributeType::Uint:
						Format = VK_FORMAT_R32_UINT;
						break;
					case BmRender_AttributeType::Float:
						Format = VK_FORMAT_R32_SFLOAT;
						break;
					case BmRender_AttributeType::Vec2:
						Format = VK_FORMAT_R32G32_SFLOAT;
						break;
					case BmRender_AttributeType::Vec3:
						Format = VK_FORMAT_R32G32B32_SFLOAT;
						break;
					case BmRender_AttributeType::Vec4:
						Format = VK_FORMAT_R32G32B32A32_SFLOAT;
						break;
					default:
						assert(false);
				}

				VkAttribute = (VkVertexInputAttributeDescription*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkVertexInputAttributeDescription));
				VkAttribute->binding = BindingIndex;
				VkAttribute->location = CurrentLocation;
				VkAttribute->format = Format;
				VkAttribute->offset = attribute.Offset;

				++CurrentLocation;
				++TotalAttributes;
			}
			else
			{
				u32 MatrixBindingOffset = 0;
				for (u32 i = 0; i < 4; ++i)
				{
					VkAttribute = (VkVertexInputAttributeDescription*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkVertexInputAttributeDescription));
					VkAttribute->binding = BindingIndex;
					VkAttribute->location = CurrentLocation;
					VkAttribute->format = VK_FORMAT_R32G32B32A32_SFLOAT;
					VkAttribute->offset = attribute.Offset + MatrixBindingOffset;

					MatrixBindingOffset += 16;
					++CurrentLocation;
					++TotalAttributes;
				}
			}
		}
	}

	VkPipelineShaderStageCreateInfo* VkShaderStages = (VkPipelineShaderStageCreateInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkPipelineShaderStageCreateInfo) * Description->ShaderStagesCount);
	for (u32 i = 0; i < Description->ShaderStagesCount; ++i)
	{
		const BmRender_ShaderStageDescription* ShaderStageDesc = Description->ShaderStages + i;
		ShaderData ShaderData;
		GetShaderData(ShaderStageDesc->Shader, &ShaderData);

		VkPipelineShaderStageCreateInfo* VkStage = VkShaderStages + i;
		VkStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VkStage->pNext = nullptr;
		VkStage->flags = 0;
		VkStage->stage = VulkanHelper::PipelineShaderStageToVkShaderStage(ShaderData.Stage);
		VkStage->module = (VkShaderModule)ShaderStageDesc->Shader;
		VkStage->pName = ShaderStageDesc->EntryPointFunction;
		VkStage->pSpecializationInfo = nullptr;
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { };
	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputState.vertexBindingDescriptionCount = Description->VertexBindingsCount;
	VertexInputState.pVertexBindingDescriptions = VkVertexBindings;
	VertexInputState.vertexAttributeDescriptionCount = TotalAttributes;
	VertexInputState.pVertexAttributeDescriptions = TotalAttributes == 0 ? nullptr : VkVertexAttributes;

	VkPipelineRenderingCreateInfo RenderingInfo = { };
	RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	RenderingInfo.pNext = nullptr;
	RenderingInfo.colorAttachmentCount = Description->ResourceInfo.PipelineAttachmentData.ColorAttachmentCount;
	RenderingInfo.pColorAttachmentFormats = Description->ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats;
	RenderingInfo.depthAttachmentFormat = Description->ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat;
	RenderingInfo.stencilAttachmentFormat = Description->ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat;

	VkPipelineColorBlendStateCreateInfo ColorBlendState = Description->ColorBlendState;
	ColorBlendState.pAttachments = &Description->ColorBlendAttachment;

	VkPipelineViewportStateCreateInfo ViewportState = Description->ViewportState;
	ViewportState.pViewports = &Description->Viewport;
	ViewportState.pScissors = &Description->Scissor;

	auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Memory::FrameAlloc(GetFrameMemory(), sizeof(VkGraphicsPipelineCreateInfo));
	*PipelineCreateInfo = { };
	PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineCreateInfo->stageCount = Description->ShaderStagesCount;
	PipelineCreateInfo->pStages = VkShaderStages;
	PipelineCreateInfo->pVertexInputState = &VertexInputState;
	PipelineCreateInfo->pInputAssemblyState = &Description->InputAssemblyState;
	PipelineCreateInfo->pViewportState = &ViewportState;
	PipelineCreateInfo->pDynamicState = nullptr;
	PipelineCreateInfo->pRasterizationState = &Description->RasterizationState;
	PipelineCreateInfo->pMultisampleState = &Description->MultisampleState;
	PipelineCreateInfo->pColorBlendState = &ColorBlendState;
	PipelineCreateInfo->pDepthStencilState = &Description->DepthStencilState;
	PipelineCreateInfo->layout = (VkPipelineLayout)Description->PipelineLayout;
	PipelineCreateInfo->renderPass = nullptr;
	PipelineCreateInfo->subpass = 0;
	PipelineCreateInfo->pNext = &RenderingInfo;

	PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
	PipelineCreateInfo->basePipelineIndex = -1;

	VkPipeline Pipeline;
	VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, GetVulkanAllocator(), &Pipeline));

	return CreatePipelineHandle(Pipeline);
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkDescriptorSetLayout* VkSetLayouts = (VkDescriptorSetLayout*)Memory::FrameAlloc(GetFrameMemory(), Description->SetLayoutCount * sizeof(VkDescriptorSetLayout));
	for (u32 i = 0; i < Description->SetLayoutCount; ++i)
	{
		VkSetLayouts[i] = (VkDescriptorSetLayout)Description->SetLayouts[i];
	}

	VkPushConstantRange* VkPushConstantRanges = (VkPushConstantRange*)Memory::FrameAlloc(GetFrameMemory(), Description->PushConstantRangeCount * sizeof(VkPushConstantRange));
	for (u32 i = 0; i < Description->PushConstantRangeCount; ++i)
	{
		VkPushConstantRanges[i].offset = Description->PushConstantRanges[i].offset;
		VkPushConstantRanges[i].size = Description->PushConstantRanges[i].size;
		VkPushConstantRanges[i].stageFlags = Description->PushConstantRanges[i].stageFlags;
	}

	VkPipelineLayoutCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfo.setLayoutCount = Description->SetLayoutCount;
	CreateInfo.pSetLayouts = VkSetLayouts;
	CreateInfo.pushConstantRangeCount = Description->PushConstantRangeCount;
	CreateInfo.pPushConstantRanges = VkPushConstantRanges;
	CreateInfo.pNext = nullptr;

	VkPipelineLayout PipelineLayout;
	VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, GetVulkanAllocator(), &PipelineLayout));

	return CreatePipelineLayoutHandle(PipelineLayout);
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkDescriptorPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.maxSets = Description->MaxSets;
	CreateInfo.poolSizeCount = Description->PoolSizeCount;
	CreateInfo.pPoolSizes = Description->PoolSizes;
	CreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
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

	ShaderData Data;
	Data.Stage = Description->Stage;

	return CreateShaderHandle(ShaderModule, &Data);
}


BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = 1;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return CreateImageResource(&Descr);
}

BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, BmRender_ImageType Type, u32 ArrayLayers)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = ArrayLayers;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return CreateImageResource(&Descr);
}

BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle)
{
	return CreateImageView(Handle, 0, 1, VK_IMAGE_VIEW_TYPE_2D);
}

BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount)
{
	return CreateImageView(Handle, BaseLayer, LayerCount, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
}

BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BmRender_PipelineSyncStage::VertexShader, BufferUsageFlag::CombinedVertexIndexFlag);
}

BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BmRender_PipelineSyncStage::VertexShader, BufferUsageFlag::InstanceFlag);
}

BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, MemoryPropertyFlag MemoryFlag, BmRender_PipelineSyncStage BufferStage)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferStage, BufferUsageFlag::UniformFlag);
}

BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, MemoryPropertyFlag MemoryFlag, BmRender_PipelineSyncStage BufferStage)
{
	return CreateGPUBuffer(Size, MemoryFlag, BufferStage, BufferUsageFlag::StorageFlag);
}

BmRender_GPUBuffer BmRender_CreateIndirectDrawBuffer(u64 Size, MemoryPropertyFlag MemoryFlag)
{
	return CreateGPUBuffer(Size, MemoryFlag, BmRender_PipelineSyncStage::VertexShader, BufferUsageFlag::IndirectDrawBufferFlag);
}

BmRender_GPUBuffer BmRender_CreateStagingBuffer(u64 Size)
{
	return CreateGPUBuffer(Size, MemoryPropertyFlag::HostCompatible, BmRender_PipelineSyncStage::None, BufferUsageFlag::StagingFlag);
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

	SemaphoreData Data;
	Data.Type = BmRender_SemaphoreType::Binary;
	VkSemaphore VulkanSemaphore;
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, GetVulkanAllocator(), &VulkanSemaphore));

	return CreateSemaphoreHandle(VulkanSemaphore, &Data);
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

	SemaphoreData Data;
	Data.Type = BmRender_SemaphoreType::Timeline;
	VkSemaphore VulkanSemaphore;
	VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &CreateInfo, GetVulkanAllocator(), &VulkanSemaphore));

	return CreateSemaphoreHandle(VulkanSemaphore, &Data);
}

BmRender_CommandPool BmRender_CreateCommandPool(u32 QueueFamilyIndex)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	VkCommandPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CreateInfo.queueFamilyIndex = QueueFamilyIndex;

	CommandPoolData Data;
	Data.QueueFamilyIndex = QueueFamilyIndex;
	VkCommandPool VulkanCommandPool;
	VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &CreateInfo, GetVulkanAllocator(), &VulkanCommandPool));

	return CreateCommandPoolHandle(VulkanCommandPool, &Data);
}

BmRender_CommandBuffer BmRender_AllocateCommandBuffer(BmRender_CommandPool CommandPool)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkCommandPool VulkanCommandPool = (VkCommandPool)CommandPool;

	VkCommandBufferAllocateInfo AllocateInfo = { };
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = VulkanCommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	CommandBufferData Data;
	Data.CommandPool = CommandPool;
	VkCommandBuffer VulkanCommandBuffer;
	VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &AllocateInfo, &VulkanCommandBuffer));

	return CreateCommandBufferHandle(VulkanCommandBuffer, &Data);
}

void BmRender_UpdateHostCompatibleBuffer(BmRender_GPUBuffer StagingBuffer, u64 BufferOffset, u64 DataSize, const void* Data)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	GPUBufferData BufferData;
	GetGPUBufferData(StagingBuffer, &BufferData);
	VulkanHelper::UpdateHostCompatibleBufferMemory(Device, BufferData.Memory, DataSize, BufferOffset, Data);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyPipelineLayout(Device, (VkPipelineLayout)Handle, GetVulkanAllocator());
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroySampler(Device, (VkSampler)Handle, GetVulkanAllocator());
}

void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetLayoutData Data;
	GetDescriptorSetLayoutData(Handle, &Data);
	free(Data.LayoutBindings);

	vkDestroyDescriptorSetLayout(Device, (VkDescriptorSetLayout)Handle, GetVulkanAllocator());
	DestroyDescriptorSetLayoutHandle(Handle);	
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
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkShaderModule ShaderModule = (VkShaderModule)Handle;
	vkDestroyShaderModule(Device, ShaderModule, GetVulkanAllocator());
	DestroyShaderHandle(Handle);
}

void BmRender_DestroyImage(BmRender_Image Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	ImageResource Data;
	if (GetImageData(Handle, &Data))
	{
		vkDestroyImage(Device, (VkImage)Handle, GetVulkanAllocator());
		vkFreeMemory(Device, Data.Memory, GetVulkanAllocator());
	}

	DestroyImageHandle(Handle);
}

void BmRender_DestroyImageView(BmRender_ImageView Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyImageView(Device, (VkImageView)Handle, GetVulkanAllocator());
}

void BmRender_DestroyGPUBuffer(BmRender_GPUBuffer Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	GPUBufferData Data;
	if (GetGPUBufferData(Handle, &Data))
	{
		vkDestroyBuffer(Device, (VkBuffer)Handle, GetVulkanAllocator());
		vkFreeMemory(Device, Data.Memory, GetVulkanAllocator());
	}

	DestroyGPUBufferHandle(Handle);
}

void BmRender_DestroyFence(BmRender_Fence Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyFence(Device, (VkFence)Handle, GetVulkanAllocator());
}

void BmRender_DestroySemaphore(BmRender_Semaphore Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkSemaphore Semaphore = (VkSemaphore)Handle;
	vkDestroySemaphore(Device, Semaphore, GetVulkanAllocator());
	DestroySemaphoreHandle(Handle);
}

void BmRender_DestroyCommandPool(BmRender_CommandPool Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkCommandPool CommandPool = (VkCommandPool)Handle;
	vkDestroyCommandPool(Device, CommandPool, GetVulkanAllocator());
	DestroyCommandPoolHandle(Handle);
}

void BmRender_FreeCommandBuffer(BmRender_CommandBuffer Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	CommandBufferData BufferData;
	GetCommandBufferData(Handle, &BufferData);
	VkCommandBuffer VulkanCommandBuffer = (VkCommandBuffer)Handle;
	VkCommandPool VulkanCommandPool = (VkCommandPool)BufferData.CommandPool;

	vkFreeCommandBuffers(Device, VulkanCommandPool, 1, &VulkanCommandBuffer);
	DestroyCommandBufferHandle(Handle);
}

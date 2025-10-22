#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"
#include "Render.h"
#include "RenderTypes.h"

#include "Util/Util.h"
#include <type_traits>

#include "Engine/Systems/HandleManager.h"

#include "VulkanCoreContext.h"

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

static void OnSamplerClear(SamplerData* SamplerData)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroySampler(Device, SamplerData->VulkanSampler, &VulkanAllocator);
}

static void OnPipelineClear(PipelineData* PipelineData)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyPipeline(Device, PipelineData->VulkanPipeline, &VulkanAllocator);
}

static void OnPipelineLayoutClear(PipelineLayoutData* LayoutData)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyPipelineLayout(Device, LayoutData->VulkanPipelineLayout, &VulkanAllocator);
}

static void OnDescriptorSetLayoutClear(DescriptorSetLayoutData* LayoutData)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorSetLayout(Device, LayoutData->Layout, &VulkanAllocator);
	free(LayoutData->LayoutBindings);
}

static void OnDescriptorPoolClear(DescriptorPoolData* PoolData)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorPool(Device, PoolData->VulkanDescriptorPool, &VulkanAllocator);
}

static void OnShaderClear(ShaderData* Shader)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyShaderModule(Device, Shader->VulkanShaderModule, &VulkanAllocator);
}

static void OnImageClear(ImageResource* Image)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyImage(Device, Image->Image, &VulkanAllocator);
	vkFreeMemory(Device, Image->Memory, &VulkanAllocator);
}

static void OnImageViewClear(ImageViewData* Data)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyImageView(Device, Data->View, &VulkanAllocator);
}

static void OnGPUBufferClear(GPUBufferData* Data)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	vkDestroyBuffer(Device, Data->Buffer, &VulkanAllocator);
	vkFreeMemory(Device, Data->Memory, &VulkanAllocator);
}

void BmRender_Init(GLFWwindow* WindowHandler)
{
	CreateCoreContext(WindowHandler);

	VulkanAllocator.pUserData = nullptr;
	VulkanAllocator.pfnAllocation = VulkanAllocationCallback;
	VulkanAllocator.pfnReallocation = VulkanReallocationCallback;
	VulkanAllocator.pfnFree = VulkanFreeCallback;
	VulkanAllocator.pfnInternalAllocation = VulkanInternalAllocationNotification;
	VulkanAllocator.pfnInternalFree = VulkanInternalFreeNotification;

	InitializeSamplerManager(32);
	InitializePipelineManager(4);
	InitializePipelineLayoutManager(32);
	InitializeDescriptorSetLayoutManager(32);
	InitializeDescriptorPoolManager(1);
	InitializeShaderManager(32);
	InitializeImageManager(32);
	InitializeImageViewManager(32);
	InitializeGPUBufferManager(4);
	InitializeDescriptorSetManager(32);
	InitializeGPUBufferEntryManager(32);
	InitializePushConstantManager(4);
}

void BmRender_DeInit()
{
	DeinitSamplerManager(OnSamplerClear);
	DeinitPipelineManager(OnPipelineClear);
	DeinitPipelineLayoutManager(OnPipelineLayoutClear);
	DeinitDescriptorSetLayoutManager(OnDescriptorSetLayoutClear);
	DeinitDescriptorPoolManager(OnDescriptorPoolClear);
	DeinitShaderManager(OnShaderClear);
	DeinitImageManager(OnImageClear);
	DeinitImageViewManager(OnImageViewClear);
	DeinitGPUBufferManager(OnGPUBufferClear);
	DeinitDescriptorSetManager();
	DeinitGPUBufferEntryManager();
	DeinitPushConstantManager();

	DestroyCoreContext();
}

VkAllocationCallbacks* BmRender_GetVulkanAllocator()
{
	return &VulkanAllocator;
}

BmRender_DescriptorSet BmRender_CreateDescriptorSet(BmRender_DescriptorSetLayout LayoutHandle, BmRender_DescriptorPool PoolHandle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetData NewSet;
	NewSet.Layout = LayoutHandle;

	DescriptorSetLayoutData* Layout = GetDescriptorSetLayoutData(LayoutHandle);
	VkDescriptorPool Pool = GetDescriptorPoolData(PoolHandle)->VulkanDescriptorPool;

	VkDescriptorSetAllocateInfo AllocInfo = { };
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = Pool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &Layout->Layout;

	VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, &NewSet.Set));

	return CreateDescriptorSetHandle(&NewSet);
}

BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetLayoutData Layout = { };
	Layout.BindingsCount = Description->BindingsCount;
	
	Layout.LayoutBindings = (DescriptorSetLayoutBinding*)malloc(sizeof(DescriptorSetLayoutBinding) * Description->BindingsCount);

	VkDescriptorSetLayoutBinding* NewLayoutBindings = (VkDescriptorSetLayoutBinding*)Render::FrameAlloc(sizeof(VkDescriptorSetLayoutBinding) * Description->BindingsCount);
	for (u32 i = 0; i < Description->BindingsCount; ++i)
	{
		NewLayoutBindings[i].binding = i;
		NewLayoutBindings[i].descriptorCount = Description->Bindings[i].DescriptorCount;
		NewLayoutBindings[i].descriptorType = Description->Bindings[i].DescriptorType;
		NewLayoutBindings[i].stageFlags = Description->Bindings[i].StageFlags;
		NewLayoutBindings[i].pImmutableSamplers = nullptr;

		Layout.LayoutBindings[i].DescriptorType = NewLayoutBindings[i].descriptorType;
	}

	VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
	LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutCreateInfo.bindingCount = Description->BindingsCount;
	LayoutCreateInfo.pBindings = NewLayoutBindings;
	LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	LayoutCreateInfo.pNext = nullptr;

	VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, &VulkanAllocator, &Layout.Layout));

	return CreateDescriptorSetLayoutHandle(&Layout);
}

void BmRender_UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;

	DescriptorSetData* Set = GetDescriptorSetData(DescriptorSetHandle);
	DescriptorSetLayoutData* Layout = GetDescriptorSetLayoutData(Set->Layout);

	VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Render::FrameAlloc(sizeof(VkWriteDescriptorSet) * BindingsCount);

	for (u32 i = 0; i < BindingsCount; i++)
	{
		const BmRender_DescriptorSetBinding& Binding = Bindings[i];

		VkDescriptorType DescriptorType = Layout->LayoutBindings[i].DescriptorType;

		WriteDescriptorSets[i] = { };
		WriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[i].dstSet = Set->Set;
		WriteDescriptorSets[i].dstBinding = i;
		WriteDescriptorSets[i].dstArrayElement = Binding.DstArrayElement;
		WriteDescriptorSets[i].descriptorType = DescriptorType;
		WriteDescriptorSets[i].descriptorCount = Binding.BindingCount;

		if (DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			VkDescriptorBufferInfo* BufferInfo = (VkDescriptorBufferInfo*)Render::FrameAlloc(sizeof(VkDescriptorBufferInfo) * Binding.BindingCount);
			for (u32 j = 0; j < Binding.BindingCount; ++j)
			{

				GPUBufferEntryData* Entry = GetGPUBufferEntryData(Binding.BufferRegions[j]);

				BufferInfo[j].buffer = GetGPUBufferData(Entry->GPUBufferHandle)->Buffer;
				BufferInfo[j].offset = Entry->BufferOffset;
				BufferInfo[j].range = Entry->Size;
			}

			WriteDescriptorSets[i].pBufferInfo = BufferInfo;
		}
		else if (VK_DESCRIPTOR_TYPE_SAMPLER || VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			VkDescriptorImageInfo* ImageInfo = (VkDescriptorImageInfo*)Render::FrameAlloc(sizeof(VkDescriptorImageInfo));
			ImageInfo->imageLayout = Binding.ImageBinding.ImageLayout;
			ImageInfo->imageView = GetImageViewData(Binding.ImageBinding.ImageView)->View;
			ImageInfo->sampler = GetSamplerData(Binding.ImageBinding.Sampler)->VulkanSampler;

			WriteDescriptorSets[i].pImageInfo = ImageInfo;
		}
		else
		{
			assert(false || "Unimplemented");
		}
	}

	vkUpdateDescriptorSets(Device, BindingsCount, WriteDescriptorSets, 0, nullptr);
}

BmRender_PushConstant BmRender_CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size)
{
	PushConstantData Constant;
	Constant.PushConstants.offset = Offset;
	Constant.PushConstants.size = Size;
	Constant.PushConstants.stageFlags = (VkShaderStageFlags)Stage;
	
	return CreatePushConstantHandle(&Constant);
}

static BmRender_GPUBuffer CreateGPUBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, BufferUsageFlag Flag)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;

	MemoryPropertyFlag MemoryFlag = MemoryPropertyFlag::GPULocal;

	if (UpdateFrequency == BufferUpdateFrequency::PerFrame)
	{
		MemoryFlag = MemoryPropertyFlag::HostCompatible;
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

	GPUBufferData NewBuffer = { };

	NewBuffer.Capacity = Capacity;
	NewBuffer.UpdateFrequency = UpdateFrequency;
	NewBuffer.PropertyFlag = MemoryFlag;
	NewBuffer.BufferStage = BufferStage;
	NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, Capacity, Flag, &VulkanAllocator);

	VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.Buffer, NewBuffer.PropertyFlag, &VulkanAllocator);
	NewBuffer.Memory = AllocResult.Memory;

	VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

	return CreateGPUBufferHandle(&NewBuffer);
}

static BmRender_Image CreateImageResource(BmRender_ImageDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = GetCoreContext()->PhysicalDevice;
	VkQueue TransferQueue = GetCoreContext()->GraphicsQueue;

	ImageResource Resource;
	Resource.IsLoaded = false;
	Resource.Format = Description->Format;

	VkImageUsageFlags Usage;

	switch (Description->Type)
	{
		case ImageType::TransferSampled:
			Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;

		case ImageType::DepthSamplad:
			Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;

		case ImageType::ColorAttachmentSampled:
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

	VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, &VulkanAllocator, &Resource.Image));

	VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
		Resource.Image, MemoryPropertyFlag::GPULocal, &VulkanAllocator);

	Resource.Memory = AllocResult.Memory;
	Resource.Size = AllocResult.Size;

	VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Resource.Image, Resource.Memory, 0));
	return CreateImageHandle(&Resource);
}

static BmRender_ImageView CreateImageView(BmRender_Image Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType, VkImageAspectFlags AspectFlags)
{
	ImageResource* Resource = GetImageData(Handle);
	ImageViewData View;

	VkImageViewCreateInfo ViewCreateInfo = { };
	ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ViewCreateInfo.flags = 0;
	ViewCreateInfo.viewType = ViewType;
	ViewCreateInfo.format = Resource->Format;
	ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
	ViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ViewCreateInfo.subresourceRange.levelCount = 1;
	ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
	ViewCreateInfo.subresourceRange.layerCount = LayerCount;
	ViewCreateInfo.image = Resource->Image;

	VkDevice Device = GetCoreContext()->LogicalDevice;
	VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, &VulkanAllocator, &View.View));

	return CreateImageViewHandle(&View);
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
	VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, &VulkanAllocator, &VulkanSampler));

	SamplerData Data;
	Data.VulkanSampler = VulkanSampler;

	return CreateSamplerHandle(&Data);
}

BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	
	VkPipelineVertexInputStateCreateInfo VertexInputState = { };
	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputState.vertexBindingDescriptionCount = Description->VertexBindingsCount;
	VertexInputState.pVertexBindingDescriptions = Description->VertexBindings;
	VertexInputState.vertexAttributeDescriptionCount = Description->VertexAttributesCount;
	VertexInputState.pVertexAttributeDescriptions = Description->VertexAttributes;

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

	auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Render::FrameAlloc(sizeof(VkGraphicsPipelineCreateInfo));
	*PipelineCreateInfo = { };
	PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineCreateInfo->stageCount = Description->ShaderStagesCount;
	PipelineCreateInfo->pStages = Description->ShaderStages;
	PipelineCreateInfo->pVertexInputState = &VertexInputState;
	PipelineCreateInfo->pInputAssemblyState = &Description->InputAssemblyState;
	PipelineCreateInfo->pViewportState = &ViewportState;
	PipelineCreateInfo->pDynamicState = nullptr;
	PipelineCreateInfo->pRasterizationState = &Description->RasterizationState;
	PipelineCreateInfo->pMultisampleState = &Description->MultisampleState;
	PipelineCreateInfo->pColorBlendState = &ColorBlendState;
	PipelineCreateInfo->pDepthStencilState = &Description->DepthStencilState;
	PipelineCreateInfo->layout = Description->PipelineLayout;
	PipelineCreateInfo->renderPass = nullptr;
	PipelineCreateInfo->subpass = 0;
	PipelineCreateInfo->pNext = &RenderingInfo;

	PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
	PipelineCreateInfo->basePipelineIndex = -1;

	VkPipeline Pipeline;
	VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, &VulkanAllocator, &Pipeline));

	PipelineData Data;
	Data.VulkanPipeline = Pipeline;

	return CreatePipelineHandle(&Data);
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	
	VkDescriptorSetLayout* VkSetLayouts = (VkDescriptorSetLayout*)malloc(Description->SetLayoutCount * sizeof(VkDescriptorSetLayout));
	for (u32 i = 0; i < Description->SetLayoutCount; ++i)
	{
		VkSetLayouts[i] = GetDescriptorSetLayoutData(Description->SetLayouts[i])->Layout;
	}
	
	VkPushConstantRange* VkPushConstantRanges = nullptr;
	if (Description->PushConstantRangeCount > 0)
	{
		VkPushConstantRanges = (VkPushConstantRange*)malloc(Description->PushConstantRangeCount * sizeof(VkPushConstantRange));
		for (u32 i = 0; i < Description->PushConstantRangeCount; ++i)
		{
			VkPushConstantRanges[i] = GetPushConstantData(Description->PushConstantRanges[i])->PushConstants;
		}
	}
	
	VkPipelineLayoutCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfo.setLayoutCount = Description->SetLayoutCount;
	CreateInfo.pSetLayouts = VkSetLayouts;
	CreateInfo.pushConstantRangeCount = Description->PushConstantRangeCount;
	CreateInfo.pPushConstantRanges = VkPushConstantRanges;
	CreateInfo.flags = Description->Flags;
	CreateInfo.pNext = nullptr;

	VkPipelineLayout PipelineLayout;
	VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, &VulkanAllocator, &PipelineLayout));

	free(VkSetLayouts);

	if (Description->PushConstantRangeCount > 0)
	{
		free(VkPushConstantRanges);
	}

	PipelineLayoutData Data;
	Data.VulkanPipelineLayout = PipelineLayout;

	return CreatePipelineLayoutHandle(&Data);
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	
	VkDescriptorPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.maxSets = Description->MaxSets;
	CreateInfo.poolSizeCount = Description->PoolSizeCount;
	CreateInfo.pPoolSizes = Description->PoolSizes;
	CreateInfo.flags = Description->Flags;
	CreateInfo.pNext = nullptr;
	
	VkDescriptorPool DescriptorPool;
	VULKAN_CHECK_RESULT(vkCreateDescriptorPool(Device, &CreateInfo, &VulkanAllocator, &DescriptorPool));
	
	DescriptorPoolData Data;
	Data.VulkanDescriptorPool = DescriptorPool;
	
	return CreateDescriptorPoolHandle(&Data);
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
	VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &CreateInfo, &VulkanAllocator, &ShaderModule));
	
	ShaderData Data;
	Data.VulkanShaderModule = ShaderModule;
	
	return CreateShaderHandle(&Data);
}

BmRender_GPUBufferEntry BmRender_CreateGPUBufferEntry(u64 BufferOffset, u64 RegionSize, BmRender_GPUBuffer BufferHandle)
{
	GPUBufferEntryData Entry;
	Entry.IsLoaded = false;
	Entry.BufferOffset = BufferOffset;
	Entry.Size = RegionSize;
	Entry.GPUBufferHandle = BufferHandle;

	return CreateGPUBufferEntryHandle(&Entry);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetPipelineLayoutData(Handle);
	OnPipelineLayoutClear(Data);
	DestroyPipelineLayoutHandle(Handle);
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetSamplerData(Handle);
	OnSamplerClear(Data);
	DestroySamplerHandle(Handle);
}

void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetDescriptorSetLayoutData(Handle);
	OnDescriptorSetLayoutClear(Data);
	DestroyDescriptorSetLayoutHandle(Handle);
}

void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetDescriptorPoolData(Handle);
	OnDescriptorPoolClear(Data);
	DestroyDescriptorPoolHandle(Handle);
}

void BmRender_DestroyPipeline(BmRender_Pipeline Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetPipelineData(Handle);
	OnPipelineClear(Data);
	DestroyPipelineHandle(Handle);
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetShaderData(Handle);
	OnShaderClear(Data);
	DestroyShaderHandle(Handle);
}

void BmRender_DestroyImage(BmRender_Image Handle)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	auto Data = GetImageData(Handle);
	OnImageClear(Data);
	DestroyImageHandle(Handle);
}































BmRender_Image BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, ImageType Type)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = 1;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return CreateImageResource(&Descr);
}

BmRender_Image BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, ImageType Type, u32 ArrayLayers)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = ArrayLayers;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return CreateImageResource(&Descr);
}

BmRender_ImageView BmRender_CreateImageView2D(BmRender_Image Handle, VkImageAspectFlags AspectFlags)
{
	return CreateImageView(Handle, 0, 1, VK_IMAGE_VIEW_TYPE_2D, AspectFlags);
}

BmRender_ImageView BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags)
{
	return CreateImageView(Handle, BaseLayer, LayerCount, VK_IMAGE_VIEW_TYPE_2D_ARRAY, AspectFlags);
}

BmRender_GPUBuffer BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency)
{
	return CreateGPUBuffer(Size, UpdateFrequency, PipelineStage::Vertex, BufferUsageFlag::CombinedVertexIndexFlag);
}

BmRender_GPUBuffer BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency)
{
	return CreateGPUBuffer(Size, UpdateFrequency, PipelineStage::Vertex, BufferUsageFlag::InstanceFlag);
}

BmRender_GPUBuffer BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage)
{
	return CreateGPUBuffer(Size, UpdateFrequency, BufferStage, BufferUsageFlag::UniformFlag);
}

BmRender_GPUBuffer BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage)
{
	return CreateGPUBuffer(Size, UpdateFrequency, BufferStage, BufferUsageFlag::StorageFlag);
}
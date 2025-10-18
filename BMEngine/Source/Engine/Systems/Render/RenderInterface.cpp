#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"
#include "Render.h"

#include "Util/Util.h"
#include <type_traits>

#include "Engine/Systems/HandleManager.h"

struct BmRender_BufferRegion_T { u64 Index; };
struct BmRender_BufferArrayRegion_T { u64 Index; };
struct BmRender_ImageViewResource_T { u64 Index; };
struct BmRender_PushConstant_T { u64 Index; };

Memory::Array<DescriptorSetLayoutBinding> LayoutBindings;

static void OnSamplerClear(SamplerData* SamplerData)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroySampler(Device, SamplerData->VulkanSampler, nullptr);
}

static void OnPipelineClear(PipelineData* PipelineData)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyPipeline(Device, PipelineData->VulkanPipeline, nullptr);
}

static void OnPipelineLayoutClear(PipelineLayoutData* LayoutData)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyPipelineLayout(Device, LayoutData->VulkanPipelineLayout, nullptr);
}

static void OnDescriptorSetLayoutClear(DescriptorSetLayoutData* LayoutData)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorSetLayout(Device, LayoutData->Layout, nullptr);
}

static void OnDescriptorPoolClear(DescriptorPoolData* PoolData)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyDescriptorPool(Device, PoolData->VulkanDescriptorPool, nullptr);
}

static void OnShaderClear(ShaderData* Shader)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyShaderModule(Device, Shader->VulkanShaderModule, nullptr);
}

static void OnImageClear(ImageResource* Image)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	vkDestroyImage(Device, Image->Image, nullptr);
	vkFreeMemory(Device, Image->Memory, nullptr);
}

void BmRender_Init()
{
	InitializeSamplerManager(32);
	InitializePipelineManager(32);
	InitializePipelineLayoutManager(32);
	InitializeDescriptorSetLayoutManager(32);
	InitializeDescriptorPoolManager(32);
	InitializeShaderManager(32);
	InitializeImageManager(32);

	LayoutBindings.Capacity = 20;
	LayoutBindings.Count = 0;
	LayoutBindings.Data = (DescriptorSetLayoutBinding*)malloc(LayoutBindings.Capacity * sizeof(LayoutBindings.Data[0]));
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

	free(LayoutBindings.Data);
}

void BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, PipelineStage::Vertex, BufferUsageFlag::CombinedVertexIndexFlag, Name);
}

void BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, PipelineStage::Vertex, BufferUsageFlag::InstanceFlag, Name);
}

void BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, BufferStage, BufferUsageFlag::UniformFlag, Name);
}

void BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, BufferStage, BufferUsageFlag::StorageFlag, Name);
}

BmRender_ImageViewResource BmRender_CreateImageView2D(BmRender_Image Handle, VkImageAspectFlags AspectFlags)
{
	return RenderResources::CreateImageView(Handle, 0, 1, VK_IMAGE_VIEW_TYPE_2D, AspectFlags);
}

BmRender_ImageViewResource BmRender_CreateImageView2DArray(BmRender_Image Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags)
{
	return RenderResources::CreateImageView(Handle, BaseLayer, LayerCount, VK_IMAGE_VIEW_TYPE_2D_ARRAY, AspectFlags);
}

void BmRender_CreateDescriptorSet(const std::string& Name, const std::string& LayoutName, const std::string& PoolName)
{
	RenderResources::CreateDescriptorSet(Name, LayoutName, PoolName);
}

void BmRender_UpdateDescriptorSet(const std::string& DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
{
	RenderResources::UpdateDescriptorSet(DescriptorSetName, Bindings, BindingsCount);
}

BmRender_PushConstant CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size)
{
	return BmRender_PushConstant();
}

static BmRender_Image CreateImageResource(BmRender_ImageDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;
	VkQueue TransferQueue = RenderResources::GetCoreContext()->GraphicsQueue;

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

	VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, nullptr, &Resource.Image));

	VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
		Resource.Image, MemoryPropertyFlag::GPULocal);

	Resource.Memory = AllocResult.Memory;
	Resource.Size = AllocResult.Size;

	VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Resource.Image, Resource.Memory, 0));
	return CreateImageHandle(&Resource);
}

BmRender_Sampler BmRender_CreateSampler(const BmRHI_SamplerDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

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
	VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, nullptr, &VulkanSampler));

	SamplerData Data;
	Data.VulkanSampler = VulkanSampler;

	return CreateSamplerHandle(&Data);
}

BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkPipelineVertexInputStateCreateInfo VertexInputState = { };
	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputState.vertexBindingDescriptionCount = static_cast<u32>(Description->VertexBindings.size());
	VertexInputState.pVertexBindingDescriptions = Description->VertexBindings.empty() ? nullptr : Description->VertexBindings.data();
	VertexInputState.vertexAttributeDescriptionCount = static_cast<u32>(Description->VertexAttributes.size());
	VertexInputState.pVertexAttributeDescriptions = Description->VertexAttributes.empty() ? nullptr : Description->VertexAttributes.data();

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
	PipelineCreateInfo->stageCount = static_cast<u32>(Description->ShaderStages.size());
	PipelineCreateInfo->pStages = Description->ShaderStages.data();
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
	VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

	PipelineData Data;
	Data.VulkanPipeline = Pipeline;

	return CreatePipelineHandle(&Data);
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkPipelineLayoutCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfo.setLayoutCount = Description->SetLayoutCount;
	CreateInfo.pSetLayouts = Description->SetLayouts;
	CreateInfo.pushConstantRangeCount = Description->PushConstantRangeCount;
	CreateInfo.pPushConstantRanges = Description->PushConstantRanges;
	CreateInfo.flags = Description->Flags;
	CreateInfo.pNext = Description->Next;

	VkPipelineLayout PipelineLayout;
	VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, nullptr, &PipelineLayout));

	PipelineLayoutData Data;
	Data.VulkanPipelineLayout = PipelineLayout;

	return CreatePipelineLayoutHandle(&Data);
}

BmRender_DescriptorSetLayout BmRender_CreateDescriptorSetLayout(const BmRender_DescriptorSetLayoutDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

	DescriptorSetLayoutData Layout = { };
	Layout.BindingsCount = Description->BindingsCount;
	Layout.BindingsIndex = LayoutBindings.Count;

	VkDescriptorSetLayoutBinding* NewLayoutBindings = (VkDescriptorSetLayoutBinding*)Render::FrameAlloc(sizeof(VkDescriptorSetLayoutBinding) * Description->BindingsCount);
	for (u32 i = 0; i < Description->BindingsCount; ++i)
	{
		NewLayoutBindings[i].binding = i;
		NewLayoutBindings[i].descriptorCount = Description->Bindings[i].DescriptorCount;
		NewLayoutBindings[i].descriptorType = Description->Bindings[i].DescriptorType;
		NewLayoutBindings[i].stageFlags = Description->Bindings[i].StageFlags;
		NewLayoutBindings[i].pImmutableSamplers = nullptr;

		assert(LayoutBindings.Count < LayoutBindings.Capacity);
		DescriptorSetLayoutBinding* Binding = LayoutBindings.Data + LayoutBindings.Count++;
		Binding->DescriptorType = NewLayoutBindings[i].descriptorType;
	}

	VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
	LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutCreateInfo.bindingCount = Description->BindingsCount;
	LayoutCreateInfo.pBindings = NewLayoutBindings;
	LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	LayoutCreateInfo.pNext = nullptr;

	VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &Layout.Layout));

	return CreateDescriptorSetLayoutHandle(&Layout);
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkDescriptorPoolCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.maxSets = Description->MaxSets;
	CreateInfo.poolSizeCount = Description->PoolSizeCount;
	CreateInfo.pPoolSizes = Description->PoolSizes;
	CreateInfo.flags = Description->Flags;
	CreateInfo.pNext = Description->Next;
	
	VkDescriptorPool DescriptorPool;
	VULKAN_CHECK_RESULT(vkCreateDescriptorPool(Device, &CreateInfo, nullptr, &DescriptorPool));
	
	DescriptorPoolData Data;
	Data.VulkanDescriptorPool = DescriptorPool;
	
	return CreateDescriptorPoolHandle(&Data);
}

BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkShaderModuleCreateInfo CreateInfo = { };
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.codeSize = Description->CodeSize;
	CreateInfo.pCode = Description->Code;
	
	VkShaderModule ShaderModule;
	VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));
	
	ShaderData Data;
	Data.VulkanShaderModule = ShaderModule;
	
	return CreateShaderHandle(&Data);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetPipelineLayoutData(Handle);
	OnPipelineLayoutClear(Data);
	DestroyPipelineLayoutHandle(Handle);
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetSamplerData(Handle);
	OnSamplerClear(Data);
	DestroySamplerHandle(Handle);
}

void BmRender_DestroyDescriptorSetLayout(BmRender_DescriptorSetLayout Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetDescriptorSetLayoutData(Handle);
	OnDescriptorSetLayoutClear(Data);
	DestroyDescriptorSetLayoutHandle(Handle);
}

void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetDescriptorPoolData(Handle);
	OnDescriptorPoolClear(Data);
	DestroyDescriptorPoolHandle(Handle);
}

void BmRender_DestroyPipeline(BmRender_Pipeline Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetPipelineData(Handle);
	OnPipelineClear(Data);
	DestroyPipelineHandle(Handle);
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	auto Data = GetShaderData(Handle);
	OnShaderClear(Data);
	DestroyShaderHandle(Handle);
}

void BmRender_DestroyImage(BmRender_Image Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
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
#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"
#include "Render.h"

#include "Util/Util.h"

#include "Engine/Systems/HandleManager.h"

struct BmRender_BufferRegion_T { u64 Index; };
struct BmRender_BufferArrayRegion_T { u64 Index; };
struct BmRender_ImageResource_T { u64 Index; };
struct BmRender_ImageViewResource_T { u64 Index; };
struct BmRender_PushConstant_T { u64 Index; };

enum class HandleType : u32
{
	Sampler,
	Pipeline,
	PipelineLayout,
	DescriptorSetLayout,
	DescriptorPool,
	Shader,

	MAX
};

static System_HandleManager HandleManagers[(u32)HandleType::MAX];

void BmRender_Init()
{
	const u32 SamplerID = (u16)HandleType::Sampler;
	HandleManagers[SamplerID] = System_HandleManager_InitData(32, sizeof(SamplerData), SamplerID);
	
	const u32 PipelineID = (u16)HandleType::Pipeline;
	HandleManagers[PipelineID] = System_HandleManager_InitData(32, sizeof(PipelineData), PipelineID);
	
	const u32 PipelineLayoutID = (u16)HandleType::PipelineLayout;
	HandleManagers[PipelineLayoutID] = System_HandleManager_InitData(32, sizeof(PipelineLayoutData), PipelineLayoutID);
	
	const u32 DescriptorSetLayoutID = (u16)HandleType::DescriptorSetLayout;
	HandleManagers[DescriptorSetLayoutID] = System_HandleManager_InitData(32, sizeof(DescriptorSetLayoutData), DescriptorSetLayoutID);
	
	const u32 DescriptorPoolID = (u16)HandleType::DescriptorPool;
	HandleManagers[DescriptorPoolID] = System_HandleManager_InitData(32, sizeof(DescriptorPoolData), DescriptorPoolID);
	
	const u32 ShaderID = (u16)HandleType::Shader;
	HandleManagers[ShaderID] = System_HandleManager_InitData(32, sizeof(ShaderData), ShaderID);
}

void BmRender_DeInit()
{
	for (u16 i = 0; i < (u16)HandleType::MAX; ++i)
	{
		System_HandleManager Manager = HandleManagers[i];
		if (Manager != nullptr)
		{
			System_HandleManager_ClearData(Manager);
		}
	}
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

BmRender_ImageViewResource BmRender_CreateImageView2D(BmRender_ImageResource Handle, VkImageAspectFlags AspectFlags)
{
	return RenderResources::CreateImageView(Handle, 0, 1, VK_IMAGE_VIEW_TYPE_2D, AspectFlags);
}

BmRender_ImageViewResource BmRender_CreateImageView2DArray(BmRender_ImageResource Handle, u32 BaseLayer, u32 LayerCount, VkImageAspectFlags AspectFlags)
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

BmRender_ImageResource BmRender_CreateImage2D(u32 Width, u32 Height, VkFormat Format, ImageType Type)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = 1;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return RenderResources::CreateImageResource(&Descr);
}

BmRender_ImageResource BmRender_CreateImage2DArray(u32 Width, u32 Height, VkFormat Format, ImageType Type, u32 ArrayLayers)
{
	BmRender_ImageDescription Descr;
	Descr.ArrayLayers = ArrayLayers;
	Descr.Format = Format;
	Descr.Width = Width;
	Descr.Height = Height;
	Descr.Type = Type;

	return RenderResources::CreateImageResource(&Descr);
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

	return (BmRender_Sampler)System_HandleManager_CreateHandle(HandleManagers[(u16)HandleType::Sampler], &Data);
}

void BmRender_DestroySampler(BmRender_Sampler Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

	System_HandleManager Manager = HandleManagers[(u16)HandleType::Sampler];
	auto Data = (SamplerData*)System_HandleManager_GetHandleData(Manager, Handle.Private);

	vkDestroySampler(Device, Data->VulkanSampler, nullptr);
	System_HandleManager_DestroyHandle(Manager, Handle.Private);
}

SamplerData* BmRender_GetSamplerData(BmRender_Sampler Handle)
{
	return (SamplerData*)System_HandleManager_GetHandleData(HandleManagers[(u16)HandleType::Sampler], Handle.Private);
}

BmRender_Pipeline BmRender_CreatePipeline(const BmRender_PipelineDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkPipelineVertexInputStateCreateInfo VertexInputState = {};
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

	return (BmRender_Pipeline)System_HandleManager_CreateHandle(HandleManagers[(u16)HandleType::Pipeline], &Data);
}

void BmRender_DestroyPipeline(BmRender_Pipeline Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

	System_HandleManager Manager = HandleManagers[(u16)HandleType::Pipeline];
	auto Data = (PipelineData*)System_HandleManager_GetHandleData(Manager, Handle.Private);

	vkDestroyPipeline(Device, Data->VulkanPipeline, nullptr);
	System_HandleManager_DestroyHandle(Manager, Handle.Private);
}

PipelineData* BmRender_GetPipelineData(BmRender_Pipeline Handle)
{
	return (PipelineData*)System_HandleManager_GetHandleData(HandleManagers[(u16)HandleType::Pipeline], Handle.Private);
}

BmRender_PipelineLayout BmRender_CreatePipelineLayout(const BmRender_PipelineLayoutDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkPipelineLayoutCreateInfo CreateInfo = {};
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

	return (BmRender_PipelineLayout)System_HandleManager_CreateHandle(HandleManagers[(u16)HandleType::PipelineLayout], &Data);
}

void BmRender_DestroyPipelineLayout(BmRender_PipelineLayout Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;

	System_HandleManager Manager = HandleManagers[(u16)HandleType::PipelineLayout];
	auto Data = (PipelineLayoutData*)System_HandleManager_GetHandleData(Manager, Handle.Private);

	vkDestroyPipelineLayout(Device, Data->VulkanPipelineLayout, nullptr);
	System_HandleManager_DestroyHandle(Manager, Handle.Private);
}

PipelineLayoutData* BmRender_GetPipelineLayoutData(BmRender_PipelineLayout Handle)
{
	return (PipelineLayoutData*)System_HandleManager_GetHandleData(HandleManagers[(u16)HandleType::PipelineLayout], Handle.Private);
}

BmRender_DescriptorPool BmRender_CreateDescriptorPool(const BmRender_DescriptorPoolDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkDescriptorPoolCreateInfo CreateInfo = {};
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
	
	return (BmRender_DescriptorPool)System_HandleManager_CreateHandle(HandleManagers[(u16)HandleType::DescriptorPool], &Data);
}

void BmRender_DestroyDescriptorPool(BmRender_DescriptorPool Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	System_HandleManager Manager = HandleManagers[(u16)HandleType::DescriptorPool];
	auto Data = (DescriptorPoolData*)System_HandleManager_GetHandleData(Manager, Handle.Private);
	
	vkDestroyDescriptorPool(Device, Data->VulkanDescriptorPool, nullptr);
	System_HandleManager_DestroyHandle(Manager, Handle.Private);
}

DescriptorPoolData* BmRender_GetDescriptorPoolData(BmRender_DescriptorPool Handle)
{
	return (DescriptorPoolData*)System_HandleManager_GetHandleData(HandleManagers[(u16)HandleType::DescriptorPool], Handle.Private);
}

BmRender_Shader BmRender_CreateShader(const BmRender_ShaderDescription* Description)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.flags = 0;
	CreateInfo.codeSize = Description->CodeSize;
	CreateInfo.pCode = Description->Code;
	
	VkShaderModule ShaderModule;
	VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));
	
	ShaderData Data;
	Data.VulkanShaderModule = ShaderModule;
	
	return (BmRender_Shader)System_HandleManager_CreateHandle(HandleManagers[(u16)HandleType::Shader], &Data);
}

void BmRender_DestroyShader(BmRender_Shader Handle)
{
	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	
	System_HandleManager Manager = HandleManagers[(u16)HandleType::Shader];
	auto Data = (ShaderData*)System_HandleManager_GetHandleData(Manager, Handle.Private);
	
	vkDestroyShaderModule(Device, Data->VulkanShaderModule, nullptr);
	System_HandleManager_DestroyHandle(Manager, Handle.Private);
}

ShaderData* BmRender_GetShaderData(BmRender_Shader Handle)
{
	return (ShaderData*)System_HandleManager_GetHandleData(HandleManagers[(u16)HandleType::Shader], Handle.Private);
}

#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"

#include "Util/Util.h"

//#include "Engine/Systems/HandleManager.h"

struct BmRender_BufferRegion_T { u64 Index; };
struct BmRender_BufferArrayRegion_T { u64 Index; };
struct BmRender_ImageResource_T { u64 Index; };
struct BmRender_ImageViewResource_T { u64 Index; };
struct BmRender_PushConstant_T { u64 Index; };

//static HandleManager::HandleManagerData<SamplerData> SamplerManager;

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

void BmRHI_Initialize()
{
	//SamplerManager = HandleManager::InitHandleManagerData<SamplerData>(1024);
}

void BmRHI_Shutdown()
{
	//HandleManager::DestroyHandle<SamplerData>(SamplerManager);
	//SamplerManager = nullptr;
}

BmRHI_Sampler BmRHI_CreateSampler(BmRHI_SamplerDescription* Description)
{
	//if (!SamplerManager || !Description)
	{
		//return HandleManager::CreateHandle<SamplerData>(0, 0);
	}

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

	//return HandleManager::CreateHandle<SamplerData>(SamplerManager, Data);
	return 1;
}

void BmRHI_DestroySampler(BmRHI_Sampler Handle)
{
	//if (!SamplerManager) return;

	//SamplerData* Data = HandleManager::GetHandleData(SamplerManager, Handle);
	//if (Data)
	//{
	//	VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
	//	vkDestroySampler(Device, Data->VulkanSampler, nullptr);
	//}

	//HandleManager::DestroyHandle<SamplerData>(SamplerManager, Handle);
}

VkSampler BmRHI_GetVulkanSampler(BmRHI_Sampler Handle)
{
	//SamplerData* Data = HandleManager::GetHandleData(SamplerManager, Handle);
	//return Data ? Data->VulkanSampler : VK_NULL_HANDLE;
	return VkSampler();
}

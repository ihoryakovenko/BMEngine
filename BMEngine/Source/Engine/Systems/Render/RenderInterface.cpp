#include "RenderInterface.h"

#include "RenderResources.h"

struct BmRender_BufferRegion_T { u64 Index; };
struct BmRender_BufferArrayRegion_T { u64 Index; };
struct BmRender_ImageResource_T { u64 Index; };
struct BmRender_ImageViewResource_T { u64 Index; };

void BmRender_CreateVertexStageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, StageBarier::Vertex, BufferUsageFlag::CombinedVertexIndexFlag, Name);
}

void BmRender_CreateInstanceBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, StageBarier::Vertex, BufferUsageFlag::InstanceFlag, Name);
}

void BmRender_CreateUniformBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, const std::string& Name)
{
	RenderResources::CreateBuffer(Size, UpdateFrequency, BufferStage, BufferUsageFlag::UniformFlag, Name);
}

void BmRender_CreateStorageBuffer(u64 Size, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, const std::string& Name)
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

void BmRender_BindDescriptorSet(const std::string& DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
{
	RenderResources::BindDescriptorSet(DescriptorSetName, Bindings, BindingsCount);
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


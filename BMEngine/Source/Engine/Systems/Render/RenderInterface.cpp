#include "RenderInterface.h"

#include "RenderResources.h"

struct BmRender_BufferRegion_T { u64 Index; };
struct BmRender_BufferArrayRegion_T { u64 Index; };
struct BmRender_ImageResource_T { u64 Index; };
struct BmRender_ImageViewResource_T { u64 Index; };

void BmRender_CreateVertexStageBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name)
{
	RenderResources::CreateBuffer(Capacity, UpdateFrequency, StageBarier::Vertex, BufferUsageFlag::CombinedVertexIndexFlag, Name);
}

void BmRender_CreateInstanceBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name)
{
	RenderResources::CreateBuffer(Capacity, UpdateFrequency, StageBarier::Vertex, BufferUsageFlag::InstanceFlag, Name);
}

void BmRender_CreateUniformBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, std::string& Name)
{
	VkPhysicalDevice PhDevice = RenderResources::GetCoreContext()->PhysicalDevice;

	VkPhysicalDeviceProperties DeviceProperties;
	vkGetPhysicalDeviceProperties(PhDevice, &DeviceProperties);

	if (Capacity > DeviceProperties.limits.maxUniformBufferRange)
	{
		assert(false);
	}

	RenderResources::CreateBuffer(Capacity, UpdateFrequency, BufferStage, BufferUsageFlag::UniformFlag, Name);
}

void BmRender_CreateStorageBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, std::string& Name)
{
	RenderResources::CreateBuffer(Capacity, UpdateFrequency, BufferStage, BufferUsageFlag::StorageFlag, Name);
}

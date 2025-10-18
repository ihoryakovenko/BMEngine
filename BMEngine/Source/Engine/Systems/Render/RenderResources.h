#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Render.h"
#include "RenderInterface.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

namespace RenderResources
{
	struct DescriptorSet
	{
		VkDescriptorSet Set;
		std::string Layout;
	};

	struct GPUBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		MemoryPropertyFlag PropertyFlag;
		PipelineStage BufferStage;
		BufferUpdateFrequency UpdateFrequency;
	};

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding);
	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize);
	void CreateSampler(const std::string& Name, const BmRHI_SamplerDescription& Data);
	void CreateGeometryBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name);
	void CreateDescriptorSetLayout(const std::string& Name, const BmRender_DescriptorSetLayoutDescription& Description);
	void UpdateDescriptorSet(std::string DescriptorSet, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);
	void CreateDescriptorSet(const std::string& Name, const std::string& LayoutName, const std::string& PoolName);
	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description);

	void CreateBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, BufferUsageFlag Flag, const std::string& Name);

	BmRender_ImageViewResource CreateImageView(BmRender_Image Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType, VkImageAspectFlags AspectFlags);
	BmRender_BufferRegion CreateBufferRegion(u64 BufferOffset, u64 RegionSize, const std::string& BufferName);
	BmRender_PushConstant CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size);

	void UpdateBufferRegion(BmRender_BufferRegion Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data);

	void OnBufferResourceLoaded(BmRender_BufferRegion Handle);
	void OnImageResourceLoaded(BmRender_Image Handle);

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	DescriptorSetLayoutData* GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VkDescriptorPool GetDescriptorPool(const std::string& Id);
	DescriptorSet* GetDescriptorSet(const std::string& Id);
	RenderResources::GPUBuffer* GetGPUBuffer(const std::string& Name);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);
	VkImage GetImage(BmRender_Image Handle);
	VkImageView GetImageView(BmRender_ImageViewResource Handle);
	VkPushConstantRange GetPushConstant(BmRender_PushConstant Handle);

	bool IsBufferResourceReady(BmRender_BufferRegion Handle);
	bool IsImageResourceReady(BmRender_Image Handle);
}
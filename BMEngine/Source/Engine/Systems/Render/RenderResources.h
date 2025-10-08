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
	struct DescriptorSetLayout
	{
		VkDescriptorSetLayout Layout;
		u32 BindingsIndex;
		u32 BindingsCount;
	};

	struct BmRender_PipelineLayoutDescription
	{
		u32 SetLayoutCount;
		const VkDescriptorSetLayout* SetLayouts;
		u32 PushConstantRangeCount;
		const VkPushConstantRange* PushConstantRanges;
		VkPipelineLayoutCreateFlags Flags;
		const void* Next;
	};

	struct BmRender_PipelineDescription
	{
		VkExtent2D Extent;
		VkPipelineLayout PipelineLayout;
		VulkanHelper::PipelineResourceInfo ResourceInfo;

		std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
		std::vector<VkVertexInputBindingDescription> VertexBindings;
		std::vector<VkVertexInputAttributeDescription> VertexAttributes;

		VkPipelineRasterizationStateCreateInfo RasterizationState;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo ColorBlendState;
		VkPipelineDepthStencilStateCreateInfo DepthStencilState;
		VkPipelineMultisampleStateCreateInfo MultisampleState;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
		VkPipelineViewportStateCreateInfo ViewportState;
		VkViewport Viewport;
		VkRect2D Scissor;
	};

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
		StageBarier BufferStage;
		BufferUpdateFrequency UpdateFrequency;
	};

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding);
	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize);
	void CreateSampler(const std::string& Name, const BmRender_SamplerDescription& Data);
	void CreateGeometryBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name);
	void CreateDescriptorSetLayout(const std::string& Name, const BmRender_DescriptorSetLayoutDescription& Description);
	void BindDescriptorSet(std::string DescriptorSet, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);
	void CreateDescriptorSet(const std::string& Name, const std::string& LayoutName, const std::string& PoolName);
	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description);

	void CreateBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, StageBarier BufferStage, BufferUsageFlag Flag, const std::string& Name);

	BmRender_ImageResource CreateImageResource(BmRender_ImageDescription* Description);
	BmRender_ImageViewResource CreateImageView(BmRender_ImageResource Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType, VkImageAspectFlags AspectFlags);
	BmRender_BufferRegion CreateBufferRegion(u64 BufferOffset, const std::string& BufferName);

	void UpdateBufferRegion(BmRender_BufferRegion Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_ImageResource Handle, BmRender_ImageDescription* Description, void* Data);

	void OnBufferResourceLoaded(BmRender_BufferRegion Handle);
	void OnImageResourceLoaded(BmRender_ImageResource Handle);

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	DescriptorSetLayout* GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VkDescriptorPool GetDescriptorPool(const std::string& Id);
	DescriptorSet* GetDescriptorSet(const std::string& Id);
	RenderResources::GPUBuffer* GetGPUBuffer(const std::string& Name);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);
	VkImage GetImage(BmRender_ImageResource Handle);
	VkImageView GetImageView(BmRender_ImageViewResource Handle);

	bool IsBufferResourceReady(BmRender_BufferRegion Handle);
	bool IsImageResourceReady(BmRender_ImageResource Handle);
}
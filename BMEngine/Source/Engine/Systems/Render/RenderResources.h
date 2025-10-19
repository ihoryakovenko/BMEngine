#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Render.h"
#include "RenderInterface.h"
#include "RenderTypes.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

namespace RenderResources
{
	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding);
	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize);
	void CreateSampler(const std::string& Name, const BmRHI_SamplerDescription& Data);
	void CreateDescriptorSetLayout(const std::string& Name, const BmRender_DescriptorSetLayoutDescription& Description);
	void UpdateDescriptorSet(std::string DescriptorSet, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);
	void CreateDescriptorSet(const std::string& Name, BmRender_DescriptorSet Set);
	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description);

	void CreateGPUBuffer(BmRender_GPUBuffer Buffer, const std::string& Name);

	BmRender_GPUBufferEntry BmRender_CreateGPUBufferEntry(u64 BufferOffset, u64 RegionSize, const std::string& BufferName);

	void UpdateBufferRegion(BmRender_GPUBufferEntry Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data);

	void OnBufferResourceLoaded(BmRender_GPUBufferEntry Handle);
	void OnImageResourceLoaded(BmRender_Image Handle);

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	DescriptorSetLayoutData* GetSetLayout(const std::string& Id);
	BmRender_DescriptorSetLayout GetDescriptorSetLayoutHandle(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VkDescriptorPool GetDescriptorPool(const std::string& Id);
	DescriptorSetData* GetDescriptorSet(const std::string& Id);
	GPUBufferData* GetGPUBuffer(const std::string& Name);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);
	VkImage GetImage(BmRender_Image Handle);
	VkImageView GetImageView(BmRender_ImageView Handle);
	VkPushConstantRange GetPushConstant(BmRender_PushConstant Handle);

	bool IsBufferResourceReady(BmRender_GPUBufferEntry Handle);
	bool IsImageResourceReady(BmRender_Image Handle);
}
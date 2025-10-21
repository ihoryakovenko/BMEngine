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
	void UpdateDescriptorSet(BmRender_DescriptorSet DescriptorSetHandle, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount);
	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description);

	BmRender_GPUBufferEntry BmRender_CreateGPUBufferEntry(u64 BufferOffset, u64 RegionSize, BmRender_GPUBuffer BufferHandle);

	void UpdateBufferRegion(BmRender_GPUBufferEntry Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data);

	void OnBufferResourceLoaded(BmRender_GPUBufferEntry Handle);
	void OnImageResourceLoaded(BmRender_Image Handle);

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	DescriptorSetLayoutData* GetSetLayout(const std::string& Id);
	BmRender_DescriptorSetLayout GetDescriptorSetLayoutHandle(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);

	bool IsBufferResourceReady(BmRender_GPUBufferEntry Handle);
	bool IsImageResourceReady(BmRender_Image Handle);
}
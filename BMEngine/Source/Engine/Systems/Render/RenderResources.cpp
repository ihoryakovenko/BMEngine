#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "Engine/Systems/Render/Render.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	static VulkanCoreContext::VulkanCoreContext CoreContext;

	static std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
	
	static std::unordered_map<std::string, VkSampler> Samplers;
	static std::unordered_map<std::string, VkDescriptorSetLayout> DescriptorSetLayouts;
	static std::unordered_map<std::string, VkShaderModule> Shaders;

	void Init(GLFWwindow* WindowHandler, Yaml::Node& Root)
	{
		VulkanCoreContext::CreateCoreContext(&CoreContext, WindowHandler);
		VkDevice Device = CoreContext.LogicalDevice;

		Yaml::Node& VerticesNode = Util::GetVertices(Root);
		for (auto VertexIt = VerticesNode.Begin(); VertexIt != VerticesNode.End(); VertexIt++)
		{
			Yaml::Node& VertexNode = (*VertexIt).second;
			
			VulkanHelper::VertexBinding Binding = { };
			Util::ParseVertexBindingNode(Util::GetVertexBindingNode(VertexNode), &Binding);
			
			Yaml::Node& AttributesNode = Util::GetVertexAttributesNode(VertexNode);

			u32 Offset = 0;
			u32 Stride = 0;
			for (auto AttributeIt = AttributesNode.Begin(); AttributeIt != AttributesNode.End(); AttributeIt++)
			{
				VulkanHelper::VertexAttribute Attribute = { };
				std::string AttributeName;
				Util::ParseVertexAttributeNode((*AttributeIt).second, &Attribute, &AttributeName);

				Yaml::Node& FormatNode = Util::GetVertexAttributeFormatNode((*AttributeIt).second);
				std::string FormatStr = FormatNode.As<std::string>();
				u32 Size = VulkanHelper::CalculateFormatSizeFromString(FormatStr.c_str(), (u32)FormatStr.length());

				Attribute.Offset = Offset;
				Offset += Size;
				Stride += Size;

				Binding.Attributes[AttributeName] = Attribute;
			}

			Binding.Stride = Stride;
			VBindings[(*VertexIt).first] = Binding;
		}

		Yaml::Node& ShadersNode = Util::GetShaders(Root);
		for (auto It = ShadersNode.Begin(); It != ShadersNode.End(); It++)
		{
			std::string ShaderPath;
			Util::ParseShaderNode((*It).second, &ShaderPath);
			
			std::vector<char> ShaderCode;
			if (Util::OpenAndReadFileFull(ShaderPath.c_str(), ShaderCode, "rb"))
			{
				VkShaderModuleCreateInfo shaderInfo = { };
				shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shaderInfo.pNext = nullptr;
				shaderInfo.flags = 0;
				shaderInfo.codeSize = ShaderCode.size();
				shaderInfo.pCode = reinterpret_cast<const uint32_t*>(ShaderCode.data());
				
				VkShaderModule NewShaderModule;
				VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &shaderInfo, nullptr, &NewShaderModule));
				Shaders[(*It).first] = NewShaderModule;
			}
			else
			{
				assert(false);
			}
		}

		Yaml::Node& SamplersNode = Util::GetSamplers(Root);
		for (auto It = SamplersNode.Begin(); It != SamplersNode.End(); It++)
		{
			VkSamplerCreateInfo CreateInfo = { };
			Util::ParseSamplerNode((*It).second, &CreateInfo);

			VkSampler NewSampler;
			VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, nullptr, &NewSampler));
			Samplers[(*It).first] = NewSampler;
		}

		Yaml::Node& DescriptorSetLayoutsNode = Util::GetDescriptorSetLayouts(Root);
		for (auto LayoutIt = DescriptorSetLayoutsNode.Begin(); LayoutIt != DescriptorSetLayoutsNode.End(); LayoutIt++)
		{
			Yaml::Node& BindingsNode = Util::ParseDescriptorSetLayoutNode((*LayoutIt).second);

			Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding> Bindings = Memory::AllocateArray<VkDescriptorSetLayoutBinding>(1);
			
			for (auto BindingIt = BindingsNode.Begin(); BindingIt != BindingsNode.End(); BindingIt++)
			{
				VkDescriptorSetLayoutBinding Binding = { };
				Util::ParseDescriptorSetLayoutBindingNode((*BindingIt).second, &Binding);
				Memory::PushBackToArray(&Bindings, &Binding);
			}

			VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
			LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo.bindingCount = Bindings.Count;
			LayoutCreateInfo.pBindings = Bindings.Data;
			LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutCreateInfo.pNext = nullptr;

			VkDescriptorSetLayout NewLayout;
			VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &NewLayout));
			DescriptorSetLayouts[(*LayoutIt).first] = NewLayout;

			Memory::FreeArray(&Bindings);
		}
	}

	VkPipeline CreateGraphicsPipeline(VkDevice Device,
		VkPipelineVertexInputStateCreateInfo* VertexInputState,
		VulkanHelper::PipelineSettings* Settings, const VulkanHelper::PipelineResourceInfo* ResourceInfo)
	{
		VkViewport Viewport = Settings->Viewport;
		Viewport.width = Settings->Extent.width;
		Viewport.height = Settings->Extent.height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		VkRect2D Scissor = Settings->Scissor;
		Scissor.extent.width = Settings->Extent.width;
		Scissor.extent.height = Settings->Extent.height;
		Scissor.offset = { };

		VkPipelineViewportStateCreateInfo ViewportState = Settings->ViewportState;
		ViewportState.pViewports = &Viewport;
		ViewportState.pScissors = &Scissor;

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = ResourceInfo->PipelineAttachmentData.ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = ResourceInfo->PipelineAttachmentData.ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;

		auto PipelineCreateInfo = Memory::MemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>();
		PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfo->stageCount = Settings->Shaders.Count;
		PipelineCreateInfo->pStages = Settings->Shaders.Data;
		PipelineCreateInfo->pVertexInputState = VertexInputState;
		PipelineCreateInfo->pInputAssemblyState = &Settings->InputAssemblyState;
		PipelineCreateInfo->pViewportState = &ViewportState;
		PipelineCreateInfo->pDynamicState = nullptr;
		PipelineCreateInfo->pRasterizationState = &Settings->RasterizationState;
		PipelineCreateInfo->pMultisampleState = &Settings->MultisampleState;
		PipelineCreateInfo->pColorBlendState = &Settings->ColorBlendState;
		PipelineCreateInfo->pDepthStencilState = &Settings->DepthStencilState;
		PipelineCreateInfo->layout = ResourceInfo->PipelineLayout;
		PipelineCreateInfo->renderPass = nullptr;
		PipelineCreateInfo->subpass = 0;
		PipelineCreateInfo->pNext = &RenderingInfo;

		PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
		PipelineCreateInfo->basePipelineIndex = -1;

		VkPipeline Pipeline;
		VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

		// todo: TMP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		Memory::FreeArray(&Settings->Shaders);

		return Pipeline;
	}

	void DeInit()
	{
		VkDevice Device = CoreContext.LogicalDevice;

		for (auto It = Shaders.begin(); It != Shaders.end(); ++It)
		{
			vkDestroyShaderModule(Device, It->second, nullptr);
		}

		for (auto It = Samplers.begin(); It != Samplers.end(); ++It)
		{
			vkDestroySampler(Device, It->second, nullptr);
		}

		for (auto It = DescriptorSetLayouts.begin(); It != DescriptorSetLayouts.end(); ++It)
		{
			vkDestroyDescriptorSetLayout(Device, It->second, nullptr);
		}

		Shaders.clear();
		Samplers.clear();
		DescriptorSetLayouts.clear();
		VBindings.clear();

		VulkanCoreContext::DestroyCoreContext(&CoreContext);
	}

	VulkanCoreContext::VulkanCoreContext* GetCoreContext()
	{
		return &CoreContext;
	}

	VkSampler GetSampler(const std::string& Id)
	{
		auto It = Samplers.find(Id);
		if (It != Samplers.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkDescriptorSetLayout GetSetLayout(const std::string& Id)
	{
		auto It = DescriptorSetLayouts.find(Id);
		if (It != DescriptorSetLayouts.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkShaderModule GetShader(const std::string& Id)
	{
		auto It = Shaders.find(Id);
		if (It != Shaders.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id)
	{
		auto It = VBindings.find(Id);
		if (It != VBindings.end())
		{
			return It->second;
		}

		assert(false);
		return { };
	}
}
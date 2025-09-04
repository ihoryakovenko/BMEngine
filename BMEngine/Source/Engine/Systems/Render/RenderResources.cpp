#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstring>

namespace RenderResources
{
	struct ResourceContext
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
		std::unordered_map<std::string, VkSampler> Samplers;
		std::unordered_map<std::string, VkDescriptorSetLayout> DescriptorSetLayouts;
		std::unordered_map<std::string, VkShaderModule> Shaders;

		Memory::DynamicHeapArray<RenderResources::RenderResource<Material>> Materials;
		Memory::DynamicHeapArray<RenderResources::RenderResource<MeshTexture2D>> Textures;
		Memory::DynamicHeapArray<RenderResources::RenderResource<InstanceData>> MeshInstances;
		Memory::DynamicHeapArray<RenderResources::RenderResource<VertexData>> StaticMeshes;

		Memory::DynamicHeapArray<bool> ResourcesState;

		VulkanHelper::GPUBuffer VertexStageData;
		VulkanHelper::GPUBuffer GPUInstances;
		VulkanHelper::GPUBuffer MaterialBuffer;

		VkSampler DiffuseSampler;
		VkSampler SpecularSampler;

		VkDescriptorSetLayout BindlesTexturesLayout;
		VkDescriptorSet BindlesTexturesSet;
		
		VkDescriptorSetLayout MaterialLayout;
		VkDescriptorSet MaterialSet;

		VkDescriptorPool MainPool;
	};

	static ResourceContext ResContext;

	void Init(GLFWwindow* WindowHandler)
	{
		VulkanCoreContext::CreateCoreContext(&ResContext.CoreContext, WindowHandler);

		const u32 PoolSizeCount = 11;
		auto TotalPassPoolSizes = (VkDescriptorPoolSize*)Render::FrameAlloc(PoolSizeCount * sizeof(VkDescriptorPoolSize));
		u32 TotalDescriptorLayouts = 21;
		TotalPassPoolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[2] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[3] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 };
		TotalPassPoolSizes[4] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 };
		TotalPassPoolSizes[5] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 };
		TotalPassPoolSizes[6] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[7] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[8] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
		TotalPassPoolSizes[9] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 };
		TotalPassPoolSizes[10] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * 3;
		TotalDescriptorCount += 256;

		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = TotalDescriptorCount;
		PoolCreateInfo.poolSizeCount = PoolSizeCount;
		PoolCreateInfo.pPoolSizes = TotalPassPoolSizes;
		PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VULKAN_CHECK_RESULT(vkCreateDescriptorPool(ResContext.CoreContext.LogicalDevice, &PoolCreateInfo, nullptr, &ResContext.MainPool));

		ResContext.ResourcesState = Memory::AllocateArray<bool>(512);
		ResContext.StaticMeshes = Memory::AllocateArray<RenderResources::RenderResource<VertexData>>(512);
		ResContext.MeshInstances = Memory::AllocateArray<RenderResources::RenderResource<InstanceData>>(512);

		const u64 VertexCapacity = MB4;
		const u64 InstanceCapacity = MB4;

		ResContext.VertexStageData.Buffer = VulkanHelper::CreateBuffer(ResContext.CoreContext.LogicalDevice, VertexCapacity, VulkanHelper::BufferUsageFlag::CombinedVertexIndexFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(ResContext.CoreContext.PhysicalDevice, ResContext.CoreContext.LogicalDevice, ResContext.VertexStageData.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		ResContext.VertexStageData.Memory = AllocResult.Memory;
		ResContext.VertexStageData.Alignment = AllocResult.Alignment;
		ResContext.VertexStageData.Capacity = AllocResult.Size;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(ResContext.CoreContext.LogicalDevice, ResContext.VertexStageData.Buffer, ResContext.VertexStageData.Memory, 0));

		ResContext.GPUInstances.Buffer = VulkanHelper::CreateBuffer(ResContext.CoreContext.LogicalDevice, InstanceCapacity, VulkanHelper::BufferUsageFlag::InstanceFlag);
		AllocResult = VulkanHelper::AllocateDeviceMemory(ResContext.CoreContext.PhysicalDevice, ResContext.CoreContext.LogicalDevice, ResContext.GPUInstances.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		ResContext.GPUInstances.Memory = AllocResult.Memory;
		ResContext.GPUInstances.Alignment = AllocResult.Alignment;
		ResContext.GPUInstances.Capacity = AllocResult.Size;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(ResContext.CoreContext.LogicalDevice, ResContext.GPUInstances.Buffer, ResContext.GPUInstances.Memory, 0));

		ResContext.Materials = Memory::AllocateArray<RenderResources::RenderResource<Material>>(512);
	}

	VkPipeline CreateGraphicsPipeline(VkDevice Device, Yaml::Node& Root,
		VkExtent2D Extent, VkPipelineLayout PipelineLayout, const VulkanHelper::PipelineResourceInfo* ResourceInfo)
	{
		Yaml::Node& PipelineNode = Util::GetPipelineNode(Root);

		Yaml::Node& ShadersNode = Util::GetPipelineShadersNode(PipelineNode);
		auto Shaders = Memory::AllocateArray<VkPipelineShaderStageCreateInfo>(1);

		for (auto it = ShadersNode.Begin(); it != ShadersNode.End(); it++)
		{
			VkPipelineShaderStageCreateInfo* NewShaderStage = Memory::ArrayGetNew(&Shaders);
			*NewShaderStage = { };
			NewShaderStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			NewShaderStage->stage = Util::ParseShaderStage((*it).first.c_str(), (*it).first.length());
			NewShaderStage->pName = "main";
			NewShaderStage->module = RenderResources::GetShader((*it).second.As<std::string>());
		}

		auto VertexBindings = Memory::AllocateArray<VkVertexInputBindingDescription>(1);
		auto VertexAttributes = Memory::AllocateArray<VkVertexInputAttributeDescription>(1);

		VkPipelineVertexInputStateCreateInfo VertexInputState = {};
		VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		Yaml::Node& VertexAttributeLayoutNode = Util::GetVertexAttributeLayoutNode(PipelineNode);
		if (!VertexAttributeLayoutNode.IsNone())
		{
			u32 currentLocation = 0;
			u32 bindingIndex = 0;

			for (auto VertexTypeIt = VertexAttributeLayoutNode.Begin(); VertexTypeIt != VertexAttributeLayoutNode.End(); VertexTypeIt++)
			{
				Yaml::Node& VertexTypeNode = (*VertexTypeIt).second;
				std::string VertexTypeName = Util::ParseNameNode(VertexTypeNode);

				VulkanHelper::VertexBinding VertexBinding = RenderResources::GetVertexBinding(VertexTypeName);
				VkVertexInputBindingDescription* NewBinding = Memory::ArrayGetNew(&VertexBindings);
				*NewBinding = {};
				NewBinding->binding = bindingIndex;
				NewBinding->stride = VertexBinding.Stride;
				NewBinding->inputRate = VertexBinding.InputRate;

				Yaml::Node& AttributesNode = Util::GetVertexAttributesNode(VertexTypeNode);
				for (auto AttrIt = AttributesNode.Begin(); AttrIt != AttributesNode.End(); AttrIt++)
				{
					Yaml::Node& AttributeNode = (*AttrIt).second;
					std::string AttributeName = Util::ParseNameNode(AttributeNode);

					auto bindingAttrIt = VertexBinding.Attributes.find(AttributeName);
					if (bindingAttrIt != VertexBinding.Attributes.end())
					{
						VkVertexInputAttributeDescription* NewAttribute = Memory::ArrayGetNew(&VertexAttributes);
						*NewAttribute = {};
						NewAttribute->binding = bindingIndex;
						NewAttribute->location = currentLocation;
						NewAttribute->format = bindingAttrIt->second.Format;
						NewAttribute->offset = bindingAttrIt->second.Offset;
						currentLocation++;
					}
				}

				bindingIndex++;
			}

			VertexInputState.vertexBindingDescriptionCount = VertexBindings.Count;
			VertexInputState.pVertexBindingDescriptions = VertexBindings.Data;
			VertexInputState.vertexAttributeDescriptionCount = VertexAttributes.Count;
			VertexInputState.pVertexAttributeDescriptions = VertexAttributes.Data;
		}

		Yaml::Node& RasterizationNode = Util::GetPipelineRasterizationNode(PipelineNode);
		Yaml::Node& ColorBlendStateNode = Util::GetPipelineColorBlendStateNode(PipelineNode);
		Yaml::Node& ColorBlendAttachmentNode = Util::GetPipelineColorBlendAttachmentNode(PipelineNode);
		Yaml::Node& DepthStencilNode = Util::GetPipelineDepthStencilNode(PipelineNode);
		Yaml::Node& MultisampleNode = Util::GetPipelineMultisampleNode(PipelineNode);
		Yaml::Node& InputAssemblyNode = Util::GetPipelineInputAssemblyNode(PipelineNode);
		Yaml::Node& ViewportStateNode = Util::GetPipelineViewportStateNode(PipelineNode);
		Yaml::Node& ViewportNode = Util::GetViewportNode(PipelineNode);
		Yaml::Node& ScissorNode = Util::GetScissorNode(PipelineNode);

		VkPipelineRasterizationStateCreateInfo RasterizationState = Util::ParsePipelineRasterizationNode(RasterizationNode);
		VkPipelineColorBlendAttachmentState ColorBlendAttachment = Util::ParsePipelineColorBlendAttachmentNode(ColorBlendAttachmentNode);
		VkPipelineColorBlendStateCreateInfo ColorBlendState = Util::ParsePipelineColorBlendStateNode(ColorBlendStateNode);
		ColorBlendState.pAttachments = &ColorBlendAttachment;
		VkPipelineDepthStencilStateCreateInfo DepthStencilState = Util::ParsePipelineDepthStencilNode(DepthStencilNode);
		VkPipelineMultisampleStateCreateInfo MultisampleState = Util::ParsePipelineMultisampleNode(MultisampleNode);
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = Util::ParsePipelineInputAssemblyNode(InputAssemblyNode);
		VkPipelineViewportStateCreateInfo ViewportState = Util::ParsePipelineViewportStateNode(ViewportStateNode);

		VkViewport Viewport = Util::ParseViewportNode(ViewportNode);
		Viewport.width = Extent.width;
		Viewport.height = Extent.height;

		VkRect2D Scissor = Util::ParseScissorNode(ScissorNode);
		Scissor.extent.width = Extent.width;
		Scissor.extent.height = Extent.height;

		ViewportState.pViewports = &Viewport;
		ViewportState.pScissors = &Scissor;

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = ResourceInfo->PipelineAttachmentData.ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = ResourceInfo->PipelineAttachmentData.ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;

		auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Render::FrameAlloc(sizeof(VkGraphicsPipelineCreateInfo));
		PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfo->stageCount = Shaders.Count;
		PipelineCreateInfo->pStages = Shaders.Data;
		PipelineCreateInfo->pVertexInputState = &VertexInputState;
		PipelineCreateInfo->pInputAssemblyState = &InputAssemblyState;
		PipelineCreateInfo->pViewportState = &ViewportState;
		PipelineCreateInfo->pDynamicState = nullptr;
		PipelineCreateInfo->pRasterizationState = &RasterizationState;
		PipelineCreateInfo->pMultisampleState = &MultisampleState;
		PipelineCreateInfo->pColorBlendState = &ColorBlendState;
		PipelineCreateInfo->pDepthStencilState = &DepthStencilState;
		PipelineCreateInfo->layout = PipelineLayout;
		PipelineCreateInfo->renderPass = nullptr;
		PipelineCreateInfo->subpass = 0;
		PipelineCreateInfo->pNext = &RenderingInfo;

		PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
		PipelineCreateInfo->basePipelineIndex = -1;

		VkPipeline Pipeline;
		VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

		Memory::FreeArray(&Shaders);
		Memory::FreeArray(&VertexBindings);
		Memory::FreeArray(&VertexAttributes);

		return Pipeline;
	}

	void DeInit()
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		for (uint32_t i = 0; i < ResContext.Textures.Count; ++i)
		{
			vkDestroyImageView(Device, ResContext.Textures.Data[i].Resource.View, nullptr);
			vkDestroyImage(Device, ResContext.Textures.Data[i].Resource.MeshTexture.Image, nullptr);
			vkFreeMemory(Device, ResContext.Textures.Data[i].Resource.MeshTexture.Memory, nullptr);
		}

		vkDestroyBuffer(Device, ResContext.MaterialBuffer.Buffer, nullptr);
		vkFreeMemory(Device, ResContext.MaterialBuffer.Memory, nullptr);
		vkDestroyBuffer(Device, ResContext.VertexStageData.Buffer, nullptr);
		vkFreeMemory(Device, ResContext.VertexStageData.Memory, nullptr);
		vkDestroyBuffer(Device, ResContext.GPUInstances.Buffer, nullptr);
		vkFreeMemory(Device, ResContext.GPUInstances.Memory, nullptr);

		Memory::FreeArray(&ResContext.Textures);
		Memory::FreeArray(&ResContext.Materials);
		Memory::FreeArray(&ResContext.StaticMeshes);
		Memory::FreeArray(&ResContext.MeshInstances);
		Memory::FreeArray(&ResContext.ResourcesState);

		for (auto It = ResContext.Shaders.begin(); It != ResContext.Shaders.end(); ++It)
		{
			vkDestroyShaderModule(Device, It->second, nullptr);
		}

		for (auto It = ResContext.Samplers.begin(); It != ResContext.Samplers.end(); ++It)
		{
			vkDestroySampler(Device, It->second, nullptr);
		}

		for (auto It = ResContext.DescriptorSetLayouts.begin(); It != ResContext.DescriptorSetLayouts.end(); ++It)
		{
			vkDestroyDescriptorSetLayout(Device, It->second, nullptr);
		}

		ResContext.Shaders.clear();
		ResContext.Samplers.clear();
		ResContext.DescriptorSetLayouts.clear();
		ResContext.VBindings.clear();

		vkDestroyDescriptorPool(Device, ResContext.MainPool, nullptr);

		VulkanCoreContext::DestroyCoreContext(&ResContext.CoreContext);
	}

	void Update()
	{
	}

	void CreateVertices(Yaml::Node& VerticesNode)
	{
		for (auto VertexIt = VerticesNode.Begin(); VertexIt != VerticesNode.End(); VertexIt++)
		{
			Yaml::Node& VertexNode = (*VertexIt).second;

			VulkanHelper::VertexBinding Binding = Util::ParseVertexBindingNode(Util::GetVertexBindingNode(VertexNode));

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
				u32 Size = Util::CalculateFormatSizeFromString(FormatStr.c_str(), (u32)FormatStr.length());

				Attribute.Offset = Offset;
				Offset += Size;
				Stride += Size;

				Binding.Attributes[AttributeName] = Attribute;
			}

			Binding.Stride = Stride;
			ResContext.VBindings[(*VertexIt).first] = Binding;
		}
	}

	void CreateShaders(Yaml::Node& ShadersNode)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		for (auto It = ShadersNode.Begin(); It != ShadersNode.End(); It++)
		{
			std::string ShaderPath = Util::ParseShaderNode((*It).second);

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
				ResContext.Shaders[(*It).first] = NewShaderModule;
			}
			else
			{
				assert(false);
			}
		}
	}

	void CreateSamplers(Yaml::Node& SamplersNode)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		for (auto It = SamplersNode.Begin(); It != SamplersNode.End(); It++)
		{
			VkSamplerCreateInfo CreateInfo = Util::ParseSamplerNode((*It).second);

			VkSampler NewSampler;
			VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, nullptr, &NewSampler));
			ResContext.Samplers[(*It).first] = NewSampler;
		}
	}

	void CreateDescriptorLayouts(Yaml::Node& DescriptorSetLayoutsNode)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		for (auto LayoutIt = DescriptorSetLayoutsNode.Begin(); LayoutIt != DescriptorSetLayoutsNode.End(); LayoutIt++)
		{
			Yaml::Node& BindingsNode = Util::ParseDescriptorSetLayoutNode((*LayoutIt).second);

			Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding> Bindings = Memory::AllocateArray<VkDescriptorSetLayoutBinding>(1);

			for (auto BindingIt = BindingsNode.Begin(); BindingIt != BindingsNode.End(); BindingIt++)
			{
				VkDescriptorSetLayoutBinding Binding = Util::ParseDescriptorSetLayoutBindingNode((*BindingIt).second);
				Memory::PushBackToArray(&Bindings, &Binding);
			}

			VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
			LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo.bindingCount = Bindings.Count;
			LayoutCreateInfo.pBindings = Bindings.Data;
			LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutCreateInfo.pNext = nullptr;

			VkDescriptorSetLayout NewLayout;
			VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &NewLayout));
			ResContext.DescriptorSetLayouts[(*LayoutIt).first] = NewLayout;

			Memory::FreeArray(&Bindings);
		}
	}

	void PostCreateInit()
	{
		const VkDeviceSize MaterialBufferSize = sizeof(Material) * 512;
		ResContext.MaterialBuffer = { };
		ResContext.MaterialBuffer.Buffer = VulkanHelper::CreateBuffer(ResContext.CoreContext.LogicalDevice, MaterialBufferSize, VulkanHelper::BufferUsageFlag::StorageFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(ResContext.CoreContext.PhysicalDevice, ResContext.CoreContext.LogicalDevice,
			ResContext.MaterialBuffer.Buffer, VulkanHelper::MemoryPropertyFlag::GPULocal);
		ResContext.MaterialBuffer.Memory = AllocResult.Memory;
		ResContext.MaterialBuffer.Alignment = AllocResult.Alignment;
		ResContext.MaterialBuffer.Capacity = MaterialBufferSize;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(ResContext.CoreContext.LogicalDevice, ResContext.MaterialBuffer.Buffer, ResContext.MaterialBuffer.Memory, 0));

		VkDescriptorBufferInfo MaterialBufferInfo;
		MaterialBufferInfo.buffer = ResContext.MaterialBuffer.Buffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = MaterialBufferSize;

		ResContext.MaterialLayout = RenderResources::GetSetLayout("MaterialLayout");

		VkDescriptorSetAllocateInfo AllocInfoMat = { };
		AllocInfoMat.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfoMat.descriptorPool = ResContext.MainPool;
		AllocInfoMat.descriptorSetCount = 1;
		AllocInfoMat.pSetLayouts = &ResContext.MaterialLayout;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(ResContext.CoreContext.LogicalDevice, &AllocInfoMat, &ResContext.MaterialSet));

		VkWriteDescriptorSet WriteDescriptorSet = { };
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = ResContext.MaterialSet;
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &MaterialBufferInfo;
		WriteDescriptorSet.pImageInfo = nullptr;

		vkUpdateDescriptorSets(ResContext.CoreContext.LogicalDevice, 1, &WriteDescriptorSet, 0, nullptr);

		const u32 MaxTextures = 64;
		ResContext.Textures = Memory::AllocateArray<RenderResource<MeshTexture2D>>(MaxTextures);
		ResContext.DiffuseSampler = RenderResources::GetSampler("DiffuseTexture");
		ResContext.SpecularSampler = RenderResources::GetSampler("SpecularTexture");
		ResContext.BindlesTexturesLayout = RenderResources::GetSetLayout("BindlesTexturesLayout");

		VkDescriptorSetAllocateInfo AllocInfoTex = { };
		AllocInfoTex.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfoTex.descriptorPool = ResContext.MainPool;
		AllocInfoTex.descriptorSetCount = 1;
		AllocInfoTex.pSetLayouts = &ResContext.BindlesTexturesLayout;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(ResContext.CoreContext.LogicalDevice, &AllocInfoTex, &ResContext.BindlesTexturesSet));
	}

	VulkanCoreContext::VulkanCoreContext* GetCoreContext()
	{
		return &ResContext.CoreContext;
	}

	VkSampler GetSampler(const std::string& Id)
	{
		auto It = ResContext.Samplers.find(Id);
		if (It != ResContext.Samplers.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkDescriptorSetLayout GetSetLayout(const std::string& Id)
	{
		auto It = ResContext.DescriptorSetLayouts.find(Id);
		if (It != ResContext.DescriptorSetLayouts.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkShaderModule GetShader(const std::string& Id)
	{
		auto It = ResContext.Shaders.find(Id);
		if (It != ResContext.Shaders.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id)
	{
		auto It = ResContext.VBindings.find(Id);
		if (It != ResContext.VBindings.end())
		{
			return It->second;
		}

		assert(false);
		return { };
	}

	u32 CreateMaterial(Material* Mat)
	{
		RenderResource<Material> NewMaterialResource;
		NewMaterialResource.ResourceIndex = ResContext.ResourcesState.Count;
		NewMaterialResource.Resource = *Mat;

		bool ResourceState = false;
		Memory::PushBackToArray(&ResContext.ResourcesState, &ResourceState);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(sizeof(Material));
		memcpy(TransferMemory, &NewMaterialResource.Resource, sizeof(Material));

		TransferSystem::TransferTask Task = { };
		Task.DataSize = sizeof(Material);
		Task.DataDescr.DstBuffer = ResContext.MaterialBuffer.Buffer;
		Task.DataDescr.DstOffset = ResContext.MaterialBuffer.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = NewMaterialResource.ResourceIndex;
		Task.Type = ResourceType::Material;

		TransferSystem::AddTask(&Task);

		ResContext.MaterialBuffer.Offset += Task.DataSize;
		Memory::PushBackToArray(&ResContext.Materials, &NewMaterialResource);
		
		return ResContext.Materials.Count - 1;
	}

	u32 CreateStaticMesh(MeshDescription* Description, void* Data)
	{
		const u64 VerticesSize = Description->VertexSize * Description->VerticesCount;
		const u64 DataSize = sizeof(u32) * Description->IndicesCount + VerticesSize;

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
		memcpy(TransferMemory, Data, DataSize);

		RenderResource<VertexData> Resource;
		Resource.ResourceIndex = ResContext.ResourcesState.Count;

		bool ResourceState = false;
		Memory::PushBackToArray(&ResContext.ResourcesState, &ResourceState);

		VertexData* Mesh = &Resource.Resource;
		*Mesh = { };
		Mesh->IndicesCount = Description->IndicesCount;
		Mesh->VertexOffset = ResContext.VertexStageData.Offset;
		Mesh->IndexOffset = ResContext.VertexStageData.Offset + VerticesSize;
		Mesh->VertexDataSize = DataSize;

		TransferSystem::TransferTask Task = { };
		Task.DataSize = DataSize;
		Task.DataDescr.DstBuffer = ResContext.VertexStageData.Buffer;
		Task.DataDescr.DstOffset = ResContext.VertexStageData.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Resource.ResourceIndex;
		Task.Type = ResourceType::Mesh;

		AddTask(&Task);

		ResContext.VertexStageData.Offset += DataSize;
		Memory::PushBackToArray(&ResContext.StaticMeshes, &Resource);

		return ResContext.StaticMeshes.Count - 1;
	}

	u32 CreateTexture2DSRGB(TextureDescription* Description, void* Data)
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();

		const u64 Index = ResContext.Textures.Count;
		RenderResource<MeshTexture2D>* Resource = Memory::ArrayGetNew(&ResContext.Textures);
		Resource->ResourceIndex = ResContext.ResourcesState.Count;

		bool ResourceState = false;
		Memory::PushBackToArray(&ResContext.ResourcesState, &ResourceState);

		MeshTexture2D* NextTexture = &Resource->Resource;

		NextTexture->MeshTexture.Width = Description->Width;
		NextTexture->MeshTexture.Height = Description->Height;

		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Description->Width;
		ImageCreateInfo.extent.height = Description->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = 0;

		VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, nullptr, &NextTexture->MeshTexture.Image));

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			NextTexture->MeshTexture.Image, VulkanHelper::MemoryPropertyFlag::GPULocal);
		NextTexture->MeshTexture.Memory = AllocResult.Memory;
		NextTexture->MeshTexture.Alignment = AllocResult.Alignment;
		NextTexture->MeshTexture.Size = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindImageMemory(Device, NextTexture->MeshTexture.Image, NextTexture->MeshTexture.Memory, 0));

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;
		ViewCreateInfo.image = NextTexture->MeshTexture.Image;

		VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &NextTexture->View));

		VkDescriptorImageInfo DiffuseImageInfo = { };
		DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DiffuseImageInfo.sampler = ResContext.DiffuseSampler;
		DiffuseImageInfo.imageView = NextTexture->View;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = ResContext.SpecularSampler;
		SpecularImageInfo.imageView = NextTexture->View;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = ResContext.BindlesTexturesSet;
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = Index;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = ResContext.BindlesTexturesSet;
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = Index;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 2, Writes, 0, nullptr);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(NextTexture->MeshTexture.Size);
		memcpy(TransferMemory, Data, NextTexture->MeshTexture.Size);

		TransferSystem::TransferTask Task = { };
		Task.DataSize = NextTexture->MeshTexture.Size;
		Task.TextureDescr.DstImage = NextTexture->MeshTexture.Image;
		Task.TextureDescr.Width = Description->Width;
		Task.TextureDescr.Height = Description->Height;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Resource->ResourceIndex;
		Task.Type = ResourceType::Texture;

		AddTask(&Task);
		return Index;
	}

	u32 CreateStaticMeshInstance(InstanceData* Data)
	{
		RenderResource<InstanceData> Resource;
		Resource.ResourceIndex = ResContext.ResourcesState.Count;
		Resource.Resource = *Data;

		bool ResourceState = false;
		Memory::PushBackToArray(&ResContext.ResourcesState, &ResourceState);

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(sizeof(InstanceData));
		memcpy(TransferMemory, &Resource.Resource, sizeof(InstanceData));

		TransferSystem::TransferTask Task = { };
		Task.DataSize = sizeof(InstanceData);
		Task.DataDescr.DstBuffer = ResContext.GPUInstances.Buffer;
		Task.DataDescr.DstOffset = ResContext.GPUInstances.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Resource.ResourceIndex;
		Task.Type = ResourceType::Instance;

		AddTask(&Task);

		ResContext.GPUInstances.Offset += Task.DataSize;
		Memory::PushBackToArray(&ResContext.MeshInstances, &Resource);

		return ResContext.MeshInstances.Count - 1;
	}

	void UpdateStaticMesh(MeshDescription* Description)
	{
	}

	void UpdateMaterial(Material* Mat)
	{
	}

	void UpdateTexture(TextureDescription* Description)
	{
	}

	void UpdateInstance(InstanceData* Data)
	{
	}

	VertexData* GetStaticMesh(u32 Index)
	{
		return &ResContext.StaticMeshes.Data[Index].Resource;
	}

	InstanceData* GetInstanceData(u32 Index)
	{
		return &ResContext.MeshInstances.Data[Index].Resource;
	}

	MeshTexture2D* GetTexture(u32 Index)
	{
		return &ResContext.Textures.Data[Index].Resource;
	}

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity)
	{
		RenderResource<VertexData>* MeshResource = ResContext.StaticMeshes.Data + Entity->StaticMeshIndex;
		if (!ResContext.ResourcesState.Data[MeshResource->ResourceIndex]) return false;

		RenderResource<InstanceData>* InstanceResource = ResContext.MeshInstances.Data + Entity->InstanceDataIndex;
		for (u32 i = Entity->InstanceDataIndex; i < Entity->Instances; ++i)
		{
			InstanceResource = ResContext.MeshInstances.Data + i;
			if (!ResContext.ResourcesState.Data[InstanceResource->ResourceIndex]) return false;
		}

		RenderResource<Material>* MaterialResource = ResContext.Materials.Data + InstanceResource->Resource.MaterialIndex;
		if (!ResContext.ResourcesState.Data[MaterialResource->ResourceIndex]) return false;

		Material* Material = &MaterialResource->Resource;
		RenderResource<MeshTexture2D>* AlbedoTexture = ResContext.Textures.Data + Material->AlbedoTexIndex;
		if (!ResContext.ResourcesState.Data[AlbedoTexture->ResourceIndex]) return false;

		RenderResource<MeshTexture2D>* SpecTexture = ResContext.Textures.Data + Material->SpecularTexIndex;
		if (!ResContext.ResourcesState.Data[SpecTexture->ResourceIndex]) return false;
		return true;
	}

	void SetResourceReadyToRender(u32 ResourceIndex)
	{
		ResContext.ResourcesState.Data[ResourceIndex] = true;
	}

	VkDescriptorSetLayout GetBindlesTexturesLayout()
	{
		return ResContext.BindlesTexturesLayout;
	}

	VkDescriptorSetLayout GetMaterialLayout()
	{
		return ResContext.MaterialLayout;
	}

	VkDescriptorSet GetBindlesTexturesSet()
	{
		return ResContext.BindlesTexturesSet;
	}

	VkDescriptorSet GetMaterialSet()
	{
		return ResContext.MaterialSet;
	}

	VkBuffer GetVertexStageBuffer()
	{
		return ResContext.VertexStageData.Buffer;
	}

	VkBuffer GetInstanceBuffer()
	{
		return ResContext.GPUInstances.Buffer;
	}

	VkDescriptorPool GetMainPool()
	{
		return ResContext.MainPool;
	}
}
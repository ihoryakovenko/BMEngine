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
		std::unordered_map<std::string, RenderResources::StorageBuffer> StorageBuffers;
		std::unordered_map<std::string, RenderResources::MeshBuffer> MeshBuffers;

		u32 MaxResourceRecords;
		u32 ResourceRecordCount;
		u32 MaxTextures;
		u32 TextureCount;
		u32 MaxStaticMeshes;
		u32 StaticMeshCount;

		ResourceRecord* ResourceRecords;
		MeshTexture2D* Textures;
		VertexData* StaticMeshes;
		

		VkDescriptorSetLayout BindlesTexturesLayout;
		VkDescriptorSetLayout MaterialLayout;

		VkDescriptorSet BindlesTexturesSet;
		VkDescriptorSet MaterialSet;

		VkSampler DiffuseSampler;
		VkSampler SpecularSampler;

		VkDescriptorPool MainPool;
	};

	static bool CheckRecordAndDependenciesLoadState(const ResourceContext* Context, const ResourceRecord* Record)
	{
		if (!Record->IsLoaded)
		{
			return false;
		}

		for (u32 i = 0; i < Record->Dependencies.Count; ++i)
		{
			ResourceDependency* Dependency = Record->Dependencies.Data + i;
			ResourceType Type = GetResourceType(Dependency->Handle);
			u32 ResourceIndex = GetResourceCPUIndex(Dependency->Handle);
			
			switch (Type)
			{
				case RenderResources::ResourceType::Texture:

					if (!Context->Textures[ResourceIndex].MeshTexture.IsLoaded)
					{
						return false;
					}

					break;
				case RenderResources::ResourceType::Mesh:
					if (!Context->StaticMeshes[ResourceIndex].IsLoaded)
					{
						return false;
					}

					break;
				case RenderResources::ResourceType::StorageResource:
					if (!CheckRecordAndDependenciesLoadState(Context, Context->ResourceRecords + ResourceIndex))
					{
						return false;
					}

					break;
				default:
					assert(false);
					break;
			}

			return true;
		}
	}

	static ResourceContext ResContext;

	static void TextureReadyToRender(ResourceHandle Handle)
	{
		u32 ResourceIndex = GetResourceCPUIndex(Handle);
		ResContext.Textures[ResourceIndex].MeshTexture.IsLoaded = true;
	}

	static void MeshReadyToRender(ResourceHandle Handle)
	{
		u32 ResourceIndex = GetResourceCPUIndex(Handle);
		ResContext.StaticMeshes[ResourceIndex].IsLoaded = true;
	}

	static void StorageResourceReadyToRender(ResourceHandle Handle)
	{
		u32 ResourceIndex = GetResourceCPUIndex(Handle);
		ResContext.ResourceRecords[ResourceIndex].IsLoaded = true;
	}

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

		ResContext.MaxTextures = 64;
		ResContext.TextureCount = 0;
		ResContext.Textures = (MeshTexture2D*)malloc(ResContext.MaxTextures * sizeof(ResContext.Textures[0]));

		ResContext.MaxStaticMeshes = 30000;
		ResContext.StaticMeshCount = 0;
		ResContext.StaticMeshes = (VertexData*)malloc(ResContext.MaxStaticMeshes * sizeof(ResContext.StaticMeshes[0]));

		ResContext.MaxResourceRecords = 60000;
		ResContext.ResourceRecordCount = 0;
		ResContext.ResourceRecords = (ResourceRecord*)malloc(ResContext.MaxResourceRecords * sizeof(ResContext.ResourceRecords[0]));
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
		*PipelineCreateInfo = { };
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

		for (uint32_t i = 0; i < ResContext.TextureCount; ++i)
		{
			vkDestroyImageView(Device, ResContext.Textures[i].View, nullptr);
			vkDestroyImage(Device, ResContext.Textures[i].MeshTexture.Image, nullptr);
			vkFreeMemory(Device, ResContext.Textures[i].MeshTexture.Memory, nullptr);
		}

		for (u32 i = 0; i < ResContext.ResourceRecordCount; ++i)
		{
			Memory::FreeArray(&ResContext.ResourceRecords[i].Dependencies);
		}

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

		for (auto It = ResContext.StorageBuffers.begin(); It != ResContext.StorageBuffers.end(); ++It)
		{
			vkDestroyBuffer(Device, It->second.Buffer, nullptr);
			vkFreeMemory(Device, It->second.Memory, nullptr);
		}

		for (auto It = ResContext.MeshBuffers.begin(); It != ResContext.MeshBuffers.end(); ++It)
		{
			vkDestroyBuffer(Device, It->second.Buffer, nullptr);
			vkFreeMemory(Device, It->second.Memory, nullptr);
		}

		vkDestroyDescriptorPool(Device, ResContext.MainPool, nullptr);

		VulkanCoreContext::DestroyCoreContext(&ResContext.CoreContext);

		ResContext.Shaders.clear();
		ResContext.Samplers.clear();
		ResContext.DescriptorSetLayouts.clear();
		ResContext.VBindings.clear();
		ResContext.StorageBuffers.clear();
		ResContext.MeshBuffers.clear();

		free(ResContext.Textures);
		free(ResContext.StaticMeshes);
		free(ResContext.ResourceRecords);
	}

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding)
	{
		ResContext.VBindings[Name] = Binding;
	}

	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		VkShaderModuleCreateInfo shaderInfo = { };
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.pNext = nullptr;
		shaderInfo.flags = 0;
		shaderInfo.codeSize = CodeSize;
		shaderInfo.pCode = Code;

		VkShaderModule NewShaderModule;
		VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &shaderInfo, nullptr, &NewShaderModule));
		ResContext.Shaders[Name] = NewShaderModule;
	}

	void CreateSampler(const std::string& Name, const SamplerDescription& Data)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		VkSamplerCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		CreateInfo.pNext = nullptr;
		CreateInfo.flags = 0;
		CreateInfo.magFilter = Data.MagFilter;
		CreateInfo.minFilter = Data.MinFilter;
		CreateInfo.mipmapMode = Data.MipmapMode;
		CreateInfo.addressModeU = Data.AddressModeU;
		CreateInfo.addressModeV = Data.AddressModeV;
		CreateInfo.addressModeW = Data.AddressModeW;
		CreateInfo.mipLodBias = Data.MipLodBias;
		CreateInfo.anisotropyEnable = Data.AnisotropyEnable;
		CreateInfo.maxAnisotropy = Data.MaxAnisotropy;
		CreateInfo.compareEnable = Data.CompareEnable;
		CreateInfo.compareOp = Data.CompareOp;
		CreateInfo.minLod = Data.MinLod;
		CreateInfo.maxLod = Data.MaxLod;
		CreateInfo.borderColor = Data.BorderColor;
		CreateInfo.unnormalizedCoordinates = Data.UnnormalizedCoordinates;

		VkSampler NewSampler;
		VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, nullptr, &NewSampler));
		ResContext.Samplers[Name] = NewSampler;
	}

	void CreateStorageBuffer(const std::string& Name, const StorageBufferDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPhysicalDevice PhysicalDevice = ResContext.CoreContext.PhysicalDevice;

		RenderResources::StorageBuffer NewBuffer = {};
		u64 CalculatedSize = Description.EntrySize * Description.Count;
		NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, CalculatedSize, Description.BufferUsageFlag);
		
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.Buffer, Description.MemoryPropertyFlag);
		NewBuffer.Memory = AllocResult.Memory;
		NewBuffer.Alignment = AllocResult.Alignment;
		NewBuffer.RecordSize = Description.EntrySize;
		NewBuffer.MaxRecords = Description.Count;
		NewBuffer.Count = 0;
		NewBuffer.UsageFlag = Description.BufferUsageFlag;
		NewBuffer.PropertyFlag = Description.MemoryPropertyFlag;
		NewBuffer.StageBarrier = Description.StageBarrier;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.StorageBuffers[Name] = NewBuffer;
	}

	void CreateMeshBuffer(const std::string& Name, const MeshBufferDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPhysicalDevice PhysicalDevice = ResContext.CoreContext.PhysicalDevice;

		RenderResources::MeshBuffer NewBuffer = {};
		NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, Description.Size, Description.BufferUsageFlag);
		
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.Buffer, Description.MemoryPropertyFlag);
		NewBuffer.Memory = AllocResult.Memory;
		NewBuffer.Capacity = AllocResult.Size;
		NewBuffer.Offset = 0;
		NewBuffer.UsageFlag = Description.BufferUsageFlag;
		NewBuffer.PropertyFlag = Description.MemoryPropertyFlag;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.MeshBuffers[Name] = NewBuffer;
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
		VkDescriptorBufferInfo MaterialBufferInfo;
		RenderResources::StorageBuffer* MaterialBuffer = GetStorageBuffer("MaterialBuffer");
		MaterialBufferInfo.buffer = MaterialBuffer->Buffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = MaterialBuffer->MaxRecords * MaterialBuffer->RecordSize;

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

	ResourceHandle CreateStaticMesh(MeshDescription* Description, void* Data, const std::string& BufferName)
	{
		assert(ResContext.StaticMeshCount < ResContext.MaxStaticMeshes);

		const u64 VerticesSize = Description->VertexSize * Description->VerticesCount;
		const u64 DataSize = sizeof(u32) * Description->IndicesCount + VerticesSize;

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
		memcpy(TransferMemory, Data, DataSize);

		VertexData* Resource = &ResContext.StaticMeshes[ResContext.StaticMeshCount];
		Resource->IsLoaded = false;
		Resource->IndicesCount = Description->IndicesCount;
		RenderResources::MeshBuffer* VertexBuffer = GetMeshBuffer(BufferName);
		Resource->VertexOffset = VertexBuffer->Offset;
		Resource->IndexOffset = VertexBuffer->Offset + VerticesSize;
		Resource->VertexDataSize = DataSize;

		TransferSystem::TransferTask Task = { };
		Task.DataSize = DataSize;
		Task.Alignment = 1;
		Task.DataDescr.DstBuffer = VertexBuffer->Buffer;
		Task.DataDescr.DstOffset = VertexBuffer->Offset;
		Task.RawData = TransferMemory;
		Task.Handle = PackResourceHandle(ResourceType::Mesh, ResContext.StaticMeshCount);
		Task.DataDescr.StageBarrier = VulkanHelper::StageBarrier::Vertex;
		Task.OnTransfered = MeshReadyToRender;

		AddTask(&Task);

		VertexBuffer->Offset += DataSize;
		u32 Index = ResContext.StaticMeshCount++;
		return PackResourceHandle(ResourceType::Mesh, Index);
	}

	ResourceHandle CreateTexture(TextureDescription* Description, void* Data)
	{
		assert(ResContext.TextureCount < ResContext.MaxTextures);

		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();

		const u64 Index = ResContext.TextureCount;
		MeshTexture2D* Resource = &ResContext.Textures[ResContext.TextureCount];
		Resource->MeshTexture.IsLoaded = false;

		MeshTexture2D* NextTexture = Resource;

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
		ImageCreateInfo.format = Description->Format;
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
		Task.Alignment = VulkanHelper::GetFormatAlignment(ImageCreateInfo.format);
		Task.TextureDescr.DstImage = NextTexture->MeshTexture.Image;
		Task.TextureDescr.Width = Description->Width;
		Task.TextureDescr.Height = Description->Height;
		Task.RawData = TransferMemory;
		Task.Handle = PackResourceHandle(ResourceType::Texture, Index);
		Task.OnTransfered = TextureReadyToRender;

		AddTask(&Task);
		ResContext.TextureCount++;
		return PackResourceHandle(ResourceType::Texture, Index);
	}

	ResourceHandle CreateStorageBufferResource(u32 DataSize, const std::string& BufferName)
	{
		assert(ResContext.ResourceRecordCount < ResContext.MaxResourceRecords);

		RenderResources::StorageBuffer* Buffer = GetStorageBuffer(BufferName);

		ResourceRecord* NewResource = ResContext.ResourceRecords + ResContext.ResourceRecordCount;
		NewResource->IsLoaded = false;
		NewResource->Dependencies = Memory::AllocateArray<ResourceDependency>(1); // TODO: FIX
		NewResource->RecordGPUIndex = Buffer->Count;
		NewResource->RecordSize = Buffer->RecordSize;
		
		Buffer->Count++;
		u32 Index = ResContext.ResourceRecordCount++;
		return PackResourceHandle(ResourceType::StorageResource, Index);
	}

	void AddResourceDependency(ResourceHandle Handle, ResourceDependency Dependency)
	{
		u32 Index = GetResourceCPUIndex(Handle);
		Memory::PushBackToArray(&ResContext.ResourceRecords[Index].Dependencies, &Dependency);
	}

	void UpdateStorageBufferResource(ResourceHandle Handle, void* Data, u32 DataSize, const std::string& BufferName)
	{
		RenderResources::StorageBuffer* Buffer = GetStorageBuffer(BufferName);

		const u32 Index = GetResourceCPUIndex(Handle);
		const ResourceRecord* Resource = ResContext.ResourceRecords + Index;

		const u64 DstOffset = Resource->RecordGPUIndex * Resource->RecordSize;

		if (Buffer->PropertyFlag == VulkanHelper::MemoryPropertyFlag::HostCompatible)
		{
			VkDevice Device = ResContext.CoreContext.LogicalDevice;
			VkPhysicalDevice PhysicalDevice = ResContext.CoreContext.PhysicalDevice;
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, Buffer->Memory, DataSize, DstOffset, Data);
			StorageResourceReadyToRender(Handle);
		}
		else if (Buffer->PropertyFlag == VulkanHelper::MemoryPropertyFlag::GPULocal)
		{
			ResourceType Type = GetResourceType(Handle);

			// TODO: TMP solution
			void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
			memcpy(TransferMemory, Data, DataSize);

			TransferSystem::TransferTask Task = { };
			Task.DataSize = DataSize;
			Task.Alignment = 1;
			Task.DataDescr.DstBuffer = Buffer->Buffer;
			Task.DataDescr.DstOffset = DstOffset;
			Task.RawData = TransferMemory;
			Task.Handle = PackResourceHandle(Type, Index);
			Task.DataDescr.StageBarrier = Buffer->StageBarrier;
			Task.OnTransfered = StorageResourceReadyToRender;

			TransferSystem::AddTask(&Task);
		}
		else
		{
			assert(false);
		}
	}

	u32 GetResourceGPUIndex(ResourceHandle Handle)
	{
		ResourceType Type = GetResourceType(Handle);
		u32 CPUIndex = GetResourceCPUIndex(Handle);

		switch (Type)
		{
			case RenderResources::ResourceType::Texture:
			case RenderResources::ResourceType::Mesh:
				return CPUIndex;
			case RenderResources::ResourceType::StorageResource:
				return ResContext.ResourceRecords[CPUIndex].RecordGPUIndex;
			default:
				assert(false);
		}
	}

	VertexData* GetStaticMesh(u32 Index)
	{
		return &ResContext.StaticMeshes[Index];
	}

	ResourceRecord* GetInstanceData(u32 Index)
	{
		return &ResContext.ResourceRecords[Index];
	}

	MeshTexture2D* GetTexture(u32 Index)
	{
		return &ResContext.Textures[Index];
	}

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity)
	{
		u32 MeshIndex = GetResourceCPUIndex(Entity->StaticMeshIndex);
		VertexData* MeshResource = ResContext.StaticMeshes + MeshIndex;
		if (!MeshResource->IsLoaded) return false;

		u32 InstanceIndex = GetResourceCPUIndex(Entity->InstanceDataIndex);
		ResourceRecord* InstanceResource = ResContext.ResourceRecords + InstanceIndex;
		return CheckRecordAndDependenciesLoadState(&ResContext, InstanceResource);
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


	VkDescriptorPool GetMainPool()
	{
		return ResContext.MainPool;
	}

	RenderResources::StorageBuffer* GetStorageBuffer(const std::string& Name)
	{
		auto It = ResContext.StorageBuffers.find(Name);
		if (It != ResContext.StorageBuffers.end())
		{
			return &It->second;
		}

		assert(false);
		return nullptr;
	}

	RenderResources::MeshBuffer* GetMeshBuffer(const std::string& Name)
	{
		auto It = ResContext.MeshBuffers.find(Name);
		if (It != ResContext.MeshBuffers.end())
		{
			return &It->second;
		}

		assert(false);
		return nullptr;
	}

}
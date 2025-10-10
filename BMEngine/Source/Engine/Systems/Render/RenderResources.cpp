#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	struct DescriptorSetLayoutBinding
	{
		VkDescriptorType DescriptorType;
	};

	struct GPUBufferEntry
	{
		std::atomic<bool> IsLoaded;
		GPUBuffer* GPUBufferHandle; // GPUBuffer* is TMP, use handle
		u64 BufferOffset;
		u64 Size;
	};

	struct ImageResource
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size;
		std::atomic<bool> IsLoaded;
		VkFormat Format;
	};

	struct ResourceContext
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
		std::unordered_map<std::string, VkSampler> Samplers;
		std::unordered_map<std::string, DescriptorSetLayout> DescriptorSetLayouts;
		std::unordered_map<std::string, VkShaderModule> Shaders;
		std::unordered_map<std::string, RenderResources::GPUBuffer> StorageBuffers;
		std::unordered_map<std::string, VkDescriptorPool> DescriptorPools;
		std::unordered_map<std::string, DescriptorSet> DescriptorSets;
		std::unordered_map<std::string, VkPipeline> Pipelines;
		std::unordered_map<std::string, VkPipelineLayout> PipelineLayouts;

		Memory::Array<VkPushConstantRange> PushConstants;
		Memory::Array<DescriptorSetLayoutBinding> LayoutBindings;
		Memory::Array<GPUBufferEntry> ResourceRecords;
		Memory::Array<ImageResource> Images;
		Memory::Array<VkImageView> ImageViews;
	};

	static ResourceContext ResContext;

	void OnBufferResourceLoaded(BmRender_BufferRegion Handle)
	{
		ResContext.ResourceRecords.Data[(u64)Handle].IsLoaded = true;
	}

	void OnImageResourceLoaded(BmRender_ImageResource Handle)
	{
		ResContext.Images.Data[(u64)Handle].IsLoaded = true;
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

		VkDescriptorPool MainPool;
		VULKAN_CHECK_RESULT(vkCreateDescriptorPool(ResContext.CoreContext.LogicalDevice, &PoolCreateInfo, nullptr, &MainPool));
		ResContext.DescriptorPools["MainPool"] = MainPool;

		ResContext.Images.Capacity = 64;
		ResContext.Images.Count = 0;
		ResContext.Images.Data = (ImageResource*)malloc(ResContext.Images.Capacity * sizeof(ResContext.Images.Data[0]));

		ResContext.ImageViews.Capacity = 64;
		ResContext.ImageViews.Count = 0;
		ResContext.ImageViews.Data = (VkImageView*)malloc(ResContext.Images.Capacity * sizeof(ResContext.Images.Data[0]));

		ResContext.ResourceRecords.Capacity = 60000;
		ResContext.ResourceRecords.Count = 0;
		ResContext.ResourceRecords.Data = (GPUBufferEntry*)malloc(ResContext.ResourceRecords.Capacity * sizeof(ResContext.ResourceRecords.Data[0]));

		ResContext.LayoutBindings.Capacity = 20;
		ResContext.LayoutBindings.Count = 0;
		ResContext.LayoutBindings.Data = (DescriptorSetLayoutBinding*)malloc(ResContext.ResourceRecords.Capacity * sizeof(ResContext.ResourceRecords.Data[0]));

		ResContext.PushConstants.Capacity = 10;
		ResContext.PushConstants.Count = 0;
		ResContext.PushConstants.Data = (VkPushConstantRange*)malloc(ResContext.ResourceRecords.Capacity * sizeof(ResContext.ResourceRecords.Data[0]));
	}

	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPipelineVertexInputStateCreateInfo VertexInputState = {};
		VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputState.vertexBindingDescriptionCount = static_cast<u32>(Description.VertexBindings.size());
		VertexInputState.pVertexBindingDescriptions = Description.VertexBindings.empty() ? nullptr : Description.VertexBindings.data();
		VertexInputState.vertexAttributeDescriptionCount = static_cast<u32>(Description.VertexAttributes.size());
		VertexInputState.pVertexAttributeDescriptions = Description.VertexAttributes.empty() ? nullptr : Description.VertexAttributes.data();

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = Description.ResourceInfo.PipelineAttachmentData.ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = Description.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = Description.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = Description.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat;

		VkPipelineColorBlendStateCreateInfo ColorBlendState = Description.ColorBlendState;
		ColorBlendState.pAttachments = &Description.ColorBlendAttachment;

		VkPipelineViewportStateCreateInfo ViewportState = Description.ViewportState;
		ViewportState.pViewports = &Description.Viewport;
		ViewportState.pScissors = &Description.Scissor;

		auto PipelineCreateInfo = (VkGraphicsPipelineCreateInfo*)Render::FrameAlloc(sizeof(VkGraphicsPipelineCreateInfo));
		*PipelineCreateInfo = { };
		PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfo->stageCount = static_cast<u32>(Description.ShaderStages.size());
		PipelineCreateInfo->pStages = Description.ShaderStages.data();
		PipelineCreateInfo->pVertexInputState = &VertexInputState;
		PipelineCreateInfo->pInputAssemblyState = &Description.InputAssemblyState;
		PipelineCreateInfo->pViewportState = &ViewportState;
		PipelineCreateInfo->pDynamicState = nullptr;
		PipelineCreateInfo->pRasterizationState = &Description.RasterizationState;
		PipelineCreateInfo->pMultisampleState = &Description.MultisampleState;
		PipelineCreateInfo->pColorBlendState = &ColorBlendState;
		PipelineCreateInfo->pDepthStencilState = &Description.DepthStencilState;
		PipelineCreateInfo->layout = Description.PipelineLayout;
		PipelineCreateInfo->renderPass = nullptr;
		PipelineCreateInfo->subpass = 0;
		PipelineCreateInfo->pNext = &RenderingInfo;

		PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
		PipelineCreateInfo->basePipelineIndex = -1;

		VkPipeline Pipeline;
		VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

		ResContext.Pipelines[Name] = Pipeline;
	}

	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPipelineLayoutCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		CreateInfo.setLayoutCount = Description.SetLayoutCount;
		CreateInfo.pSetLayouts = Description.SetLayouts;
		CreateInfo.pushConstantRangeCount = Description.PushConstantRangeCount;
		CreateInfo.pPushConstantRanges = Description.PushConstantRanges;
		CreateInfo.flags = Description.Flags;
		CreateInfo.pNext = Description.Next;

		VkPipelineLayout PipelineLayout;
		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &CreateInfo, nullptr, &PipelineLayout));
		ResContext.PipelineLayouts[Name] = PipelineLayout;
	}

	void CreateBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, PipelineStage BufferStage, BufferUsageFlag Flag, const std::string& Name)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPhysicalDevice PhDevice = ResContext.CoreContext.PhysicalDevice;

		MemoryPropertyFlag MemoryFlag = MemoryPropertyFlag::GPULocal;

		if (UpdateFrequency == BufferUpdateFrequency::PerFrame)
		{
			MemoryFlag = MemoryPropertyFlag::HostCompatible;
		}

		if (Flag == BufferUsageFlag::UniformFlag)
		{
			VkPhysicalDeviceProperties DeviceProperties;
			vkGetPhysicalDeviceProperties(PhDevice, &DeviceProperties);

			if (Capacity > DeviceProperties.limits.maxUniformBufferRange)
			{
				assert(false);
			}
		}

		GPUBuffer NewBuffer = { };

		NewBuffer.Capacity = Capacity;
		NewBuffer.UpdateFrequency = UpdateFrequency;
		NewBuffer.PropertyFlag = MemoryFlag;
		NewBuffer.BufferStage = BufferStage;
		NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, Capacity, Flag);

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhDevice, Device, NewBuffer.Buffer, NewBuffer.PropertyFlag);
		NewBuffer.Memory = AllocResult.Memory;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.StorageBuffers[Name] = NewBuffer;
	}

	VkPipeline GetPipeline(const std::string& Name)
	{
		auto it = ResContext.Pipelines.find(Name);
		if (it != ResContext.Pipelines.end())
		{
			return it->second;
		}
		return VK_NULL_HANDLE;
	}

	VkPipelineLayout GetPipelineLayout(const std::string& Name)
	{
		auto it = ResContext.PipelineLayouts.find(Name);
		if (it != ResContext.PipelineLayouts.end())
		{
			return it->second;
		}
		return VK_NULL_HANDLE;
	}

	VkImage GetImage(BmRender_ImageResource Handle)
	{
		return ResContext.Images.Data[(u64)Handle].Image;
	}

	VkImageView GetImageView(BmRender_ImageViewResource Handle)
	{
		return ResContext.ImageViews.Data[(u64)Handle];
	}

	VkPushConstantRange GetPushConstant(BmRender_PushConstant Handle)
	{
		return ResContext.PushConstants.Data[(u64)Handle];
	}

	void DeInit()
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		for (uint32_t i = 0; i < ResContext.ImageViews.Count; ++i)
		{
			vkDestroyImageView(Device, ResContext.ImageViews.Data[i], nullptr);
		}

		for (uint32_t i = 0; i < ResContext.Images.Count; ++i)
		{
			vkDestroyImage(Device, ResContext.Images.Data[i].Image, nullptr);
			vkFreeMemory(Device, ResContext.Images.Data[i].Memory, nullptr);
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
			vkDestroyDescriptorSetLayout(Device, It->second.Layout, nullptr);
		}

		for (auto It = ResContext.StorageBuffers.begin(); It != ResContext.StorageBuffers.end(); ++It)
		{
			vkDestroyBuffer(Device, It->second.Buffer, nullptr);
			vkFreeMemory(Device, It->second.Memory, nullptr);
		}

		for (auto It = ResContext.Pipelines.begin(); It != ResContext.Pipelines.end(); ++It)
		{
			vkDestroyPipeline(Device, It->second, nullptr);
		}

		for (auto It = ResContext.PipelineLayouts.begin(); It != ResContext.PipelineLayouts.end(); ++It)
		{
			vkDestroyPipelineLayout(Device, It->second, nullptr);
		}

		for (auto It = ResContext.DescriptorPools.begin(); It != ResContext.DescriptorPools.end(); ++It)
		{
			vkDestroyDescriptorPool(Device, It->second, nullptr);
		}

		VulkanCoreContext::DestroyCoreContext(&ResContext.CoreContext);

		ResContext.Shaders.clear();
		ResContext.Samplers.clear();
		ResContext.DescriptorSetLayouts.clear();
		ResContext.VBindings.clear();

		free(ResContext.ImageViews.Data);
		free(ResContext.Images.Data);
		free(ResContext.ResourceRecords.Data);
		free(ResContext.LayoutBindings.Data);
		free(ResContext.PushConstants.Data);
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

	void CreateSampler(const std::string& Name, const BmRender_SamplerDescription& Data)
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

	void CreateGeometryBuffer(u64 Capacity, BufferUpdateFrequency UpdateFrequency, std::string& Name)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		VkPhysicalDevice PhDevice = ResContext.CoreContext.PhysicalDevice;

		VkPhysicalDeviceProperties DeviceProperties;
		vkGetPhysicalDeviceProperties(PhDevice, &DeviceProperties);

		GPUBuffer NewBuffer = { };

		NewBuffer.Capacity = Capacity;
		NewBuffer.PropertyFlag = UpdateFrequency == BufferUpdateFrequency::Static ? MemoryPropertyFlag::GPULocal : MemoryPropertyFlag::HostCompatible;
		NewBuffer.BufferStage = PipelineStage::Vertex;
		NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, Capacity, BufferUsageFlag::CombinedVertexIndexFlag);

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhDevice, Device, NewBuffer.Buffer, NewBuffer.PropertyFlag);
		NewBuffer.Memory = AllocResult.Memory;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.StorageBuffers[Name] = NewBuffer;
	}

	void CreateDescriptorSetLayout(const std::string& Name, const BmRender_DescriptorSetLayoutDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		DescriptorSetLayout Layout = { };
		Layout.BindingsCount = Description.BindingsCount;
		Layout.BindingsIndex = ResContext.LayoutBindings.Count;

		VkDescriptorSetLayoutBinding* LayoutBindings = (VkDescriptorSetLayoutBinding*)Render::FrameAlloc(sizeof(VkDescriptorSetLayoutBinding) * Description.BindingsCount);
		for (u32 i = 0; i < Description.BindingsCount; ++i)
		{
			LayoutBindings[i].binding = i;
			LayoutBindings[i].descriptorCount = Description.Bindings[i].DescriptorCount;
			LayoutBindings[i].descriptorType = Description.Bindings[i].DescriptorType;
			LayoutBindings[i].stageFlags = Description.Bindings[i].StageFlags;
			LayoutBindings[i].pImmutableSamplers = nullptr;

			assert(ResContext.LayoutBindings.Count < ResContext.LayoutBindings.Capacity);
			DescriptorSetLayoutBinding* Binding = ResContext.LayoutBindings.Data + ResContext.LayoutBindings.Count++;
			Binding->DescriptorType = LayoutBindings[i].descriptorType;
		}

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = Description.BindingsCount;
		LayoutCreateInfo.pBindings = LayoutBindings;
		LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		LayoutCreateInfo.pNext = nullptr;

		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &Layout.Layout));
		ResContext.DescriptorSetLayouts[Name] = Layout;
	}

	void UpdateDescriptorSet(std::string DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		DescriptorSet* Set = GetDescriptorSet(DescriptorSetName);
		DescriptorSetLayout* Layout = GetSetLayout(Set->Layout);

		VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Render::FrameAlloc(sizeof(VkWriteDescriptorSet) * BindingsCount);

		for (u32 i = 0; i < BindingsCount; i++)
		{
			const BmRender_DescriptorSetBinding& Binding = Bindings[i];

			VkDescriptorType DescriptorType = ResContext.LayoutBindings.Data[Layout->BindingsIndex + i].DescriptorType;

			WriteDescriptorSets[i] = { };
			WriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[i].dstSet = Set->Set;
			WriteDescriptorSets[i].dstBinding = i;
			WriteDescriptorSets[i].dstArrayElement = Binding.DstArrayElement;
			WriteDescriptorSets[i].descriptorType = DescriptorType;
			WriteDescriptorSets[i].descriptorCount = Binding.BindingCount;
			
			if (DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
				DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			{
				VkDescriptorBufferInfo* BufferInfo = (VkDescriptorBufferInfo*)Render::FrameAlloc(sizeof(VkDescriptorBufferInfo) * Binding.BindingCount);
				for (u32 j = 0; j < Binding.BindingCount; ++j)
				{
					GPUBufferEntry* Entry = ResContext.ResourceRecords.Data + (u64)Binding.BufferRegions[j];

					BufferInfo[j].buffer = Entry->GPUBufferHandle->Buffer;
					BufferInfo[j].offset = Entry->BufferOffset;
					BufferInfo[j].range = Entry->Size;
				}

				WriteDescriptorSets[i].pBufferInfo = BufferInfo;
			}
			else if (VK_DESCRIPTOR_TYPE_SAMPLER || VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				VkDescriptorImageInfo* ImageInfo = (VkDescriptorImageInfo*)Render::FrameAlloc(sizeof(VkDescriptorImageInfo));
				ImageInfo->imageLayout = Binding.ImageBinding.ImageLayout;
				ImageInfo->imageView = GetImageView(Binding.ImageBinding.ImageView);
				ImageInfo->sampler = GetSampler(Binding.ImageBinding.Sampler);

				WriteDescriptorSets[i].pImageInfo = ImageInfo;
			}
			else
			{
				assert(false || "Unimplemented");
			}
		}

		vkUpdateDescriptorSets(Device, BindingsCount, WriteDescriptorSets, 0, nullptr);
	}

	void CreateDescriptorSet(const std::string& Name, const std::string& LayoutName, const std::string& PoolName)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		
		DescriptorSet NewSet;
		NewSet.Layout = LayoutName;

		DescriptorSetLayout* Layout = GetSetLayout(LayoutName);
		VkDescriptorPool Pool = GetDescriptorPool(PoolName);
		
		VkDescriptorSetAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = Pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layout->Layout;
		
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, &NewSet.Set));
		ResContext.DescriptorSets[Name] = NewSet;
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

	DescriptorSetLayout* GetSetLayout(const std::string& Id)
	{
		auto It = ResContext.DescriptorSetLayouts.find(Id);
		if (It != ResContext.DescriptorSetLayouts.end())
		{
			return &It->second;
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

	VkDescriptorPool GetDescriptorPool(const std::string& Id)
	{
		auto It = ResContext.DescriptorPools.find(Id);
		if (It != ResContext.DescriptorPools.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	DescriptorSet* GetDescriptorSet(const std::string& Id)
	{
		auto It = ResContext.DescriptorSets.find(Id);
		if (It != ResContext.DescriptorSets.end())
		{
			return &It->second;
		}

		assert(false);
		return nullptr;
	}

	BmRender_ImageResource CreateImageResource(BmRender_ImageDescription* Description)
	{
		assert(ResContext.Images.Count < ResContext.Images.Capacity);

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;
		VkQueue TransferQueue = RenderResources::GetCoreContext()->GraphicsQueue;

		ImageResource* Resource = &ResContext.Images.Data[ResContext.Images.Count];
		Resource->IsLoaded = false;
		Resource->Format = Description->Format;

		VkImageUsageFlags Usage;

		switch (Description->Type)
		{
			case ImageType::TransferSampled:
				Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				break;

			case ImageType::DepthSamplad:
				Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				break;

			case ImageType::ColorAttachmentSampled:
				Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				break;

			default:
				assert(false);
				break;
		}

		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Description->Width;
		ImageCreateInfo.extent.height = Description->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Description->ArrayLayers;
		ImageCreateInfo.format = Resource->Format;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = Usage;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = 0;

		VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, nullptr, &Resource->Image));

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			Resource->Image, MemoryPropertyFlag::GPULocal);
		Resource->Memory = AllocResult.Memory;
		Resource->Size = AllocResult.Size;

		VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Resource->Image, Resource->Memory, 0));

		return (BmRender_ImageResource)ResContext.Images.Count++;
	}

	BmRender_ImageViewResource CreateImageView(BmRender_ImageResource Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType, VkImageAspectFlags AspectFlags)
	{
		const u32 Index = (u64)Handle;

		ImageResource* Resource = ResContext.Images.Data + Index;
		VkImageView* View = ResContext.ImageViews.Data + ResContext.ImageViews.Count;

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = ViewType;
		ViewCreateInfo.format = Resource->Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
		ViewCreateInfo.subresourceRange.layerCount = LayerCount;
		ViewCreateInfo.image = Resource->Image;

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, View));

		return (BmRender_ImageViewResource)ResContext.ImageViews.Count++;
	}

	BmRender_BufferRegion CreateBufferRegion(u64 BufferOffset, u64 RegionSize, const std::string& BufferName)
	{
		assert(ResContext.ResourceRecords.Count < ResContext.ResourceRecords.Capacity);

		GPUBufferEntry* Entry = ResContext.ResourceRecords.Data + ResContext.ResourceRecords.Count;
		Entry->IsLoaded = false;
		Entry->BufferOffset = BufferOffset;
		Entry->Size = RegionSize;
		Entry->GPUBufferHandle = GetGPUBuffer(BufferName);
	
		return (BmRender_BufferRegion)ResContext.ResourceRecords.Count++;
	}

	BmRender_PushConstant CreatePushConstant(PipelineStage Stage, u32 Offset, u32 Size)
	{
		assert(ResContext.PushConstants.Count < ResContext.PushConstants.Capacity);

		VkPushConstantRange* Constant = ResContext.PushConstants.Data + ResContext.PushConstants.Count;
		Constant->offset = Offset;
		Constant->size = Size;
		Constant->stageFlags = (VkPipelineStageFlagBits)Stage;

		return (BmRender_PushConstant)ResContext.PushConstants.Count;
	}

	void UpdateBufferRegion(BmRender_BufferRegion Handle, u64 ResourceOffset, const void* Data, u32 DataSize)
	{
		const u32 Index = (u64)Handle;
		GPUBufferEntry* Entry = ResContext.ResourceRecords.Data + Index;
		RenderResources::GPUBuffer* Buffer = Entry->GPUBufferHandle;

		const u64 Offset = Entry->BufferOffset + ResourceOffset;

		if (Buffer->PropertyFlag == MemoryPropertyFlag::HostCompatible)
		{
			VkDevice Device = ResContext.CoreContext.LogicalDevice;
			VkPhysicalDevice PhysicalDevice = ResContext.CoreContext.PhysicalDevice;
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, Buffer->Memory, DataSize, Offset, Data);
			OnBufferResourceLoaded(Handle);
		}
		else if (Buffer->PropertyFlag == MemoryPropertyFlag::GPULocal)
		{
			// TODO: TMP solution
			void* TransferMemory = TransferSystem::RequestTransferMemory(DataSize);
			memcpy(TransferMemory, Data, DataSize);

			TransferSystem::TransferTask Task = { };
			Task.DataSize = DataSize;
			Task.Alignment = 1;
			Task.DataDescr.DstBuffer = Buffer->Buffer;
			Task.DataDescr.DstOffset = Offset;
			Task.RawData = TransferMemory;
			Task.DataDescr.Handle = Handle;
			Task.DataDescr.StageBarrier = Buffer->BufferStage;
			Task.Type = TransferSystem::TaskType::Data;

			TransferSystem::AddTask(&Task);
		}
		else
		{
			assert(false);
		}
	}

	void UpdateImageResource(BmRender_ImageResource Handle, BmRender_ImageDescription* Description, void* Data)
	{
		const u32 Index = (u64)Handle;
		ImageResource* Image = ResContext.Images.Data + Index;

		// TODO: TMP solution
		void* TransferMemory = TransferSystem::RequestTransferMemory(Image->Size);
		memcpy(TransferMemory, Data, Image->Size);

		TransferSystem::TransferTask Task = { };
		Task.DataSize = Image->Size;
		Task.Alignment = VulkanHelper::GetFormatAlignment(Description->Format);
		Task.TextureDescr.DstImage = Image->Image;
		Task.TextureDescr.Width = Description->Width;
		Task.TextureDescr.Height = Description->Height;
		Task.RawData = TransferMemory;
		Task.TextureDescr.Handle = Handle;
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
	}

	bool IsBufferResourceReady(BmRender_BufferRegion Handle)
	{
		return ResContext.ResourceRecords.Data[(u64)Handle].IsLoaded;
	}

	bool IsImageResourceReady(BmRender_ImageResource Handle)
	{
		return ResContext.Images.Data[(u64)Handle].IsLoaded;
	}

	RenderResources::GPUBuffer* GetGPUBuffer(const std::string& Name)
	{
		auto It = ResContext.StorageBuffers.find(Name);
		if (It != ResContext.StorageBuffers.end())
		{
			return &It->second;
		}

		assert(false);
		return nullptr;
	}
}
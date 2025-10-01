#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	struct ImageResource
	{
		VkImage Image;
		VkDeviceMemory Memory; 
		std::atomic<bool> IsLoaded;
	};

	struct ResourceContext
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
		std::unordered_map<std::string, VkSampler> Samplers;
		std::unordered_map<std::string, VkDescriptorSetLayout> DescriptorSetLayouts;
		std::unordered_map<std::string, VkShaderModule> Shaders;
		std::unordered_map<std::string, RenderResources::GPUBuffer> StorageBuffers;
		std::unordered_map<std::string, VkDescriptorPool> DescriptorPools;
		std::unordered_map<std::string, VkDescriptorSet> DescriptorSets;
		std::unordered_map<std::string, VkPipeline> Pipelines;
		std::unordered_map<std::string, VkPipelineLayout> PipelineLayouts;

		Memory::Array<std::atomic<bool>> ResourceStates;
		Memory::Array<ImageResource> Images;
		Memory::Array<VkImageView> ImageViews;
	};

	static ResourceHandle PackResourceHandle(ResourceType Type, u32 Index)
	{
		return ((u64)(Type) << 32) | (u64)(Index);
	}

	static ResourceType GetResourceType(ResourceHandle Handle)
	{
		return (ResourceType)(Handle >> 32);
	}

	static u32 GetResourceCPUIndex(ResourceHandle Handle)
	{
		return (u32)(Handle & 0xFFFFFFFF);
	}

	static ResourceContext ResContext;

	void OnResourceLoaded(ResourceHandle Handle)
	{
		ResourceType Type = GetResourceType(Handle);
		u32 ResourceIndex = GetResourceCPUIndex(Handle);

		switch (Type)
		{
			case ResourceType::Texture:
				ResContext.Images.Data[ResourceIndex].IsLoaded = true;
				break;
			case ResourceType::StorageResource:
				ResContext.ResourceStates.Data[ResourceIndex] = true;
				break;
			default:
				assert(false);
				break;
		}
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

		ResContext.ResourceStates.Capacity = 60000;
		ResContext.ResourceStates.Count = 0;
		ResContext.ResourceStates.Data = (std::atomic<bool>*)malloc(ResContext.ResourceStates.Capacity * sizeof(ResContext.ResourceStates.Data[0]));
	}

	void CreateGraphicsPipeline(const std::string& Name, const PipelineDescription& Description)
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

	void CreatePipelineLayout(const std::string& Name, const PipelineLayoutDescription& Description)
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
			vkDestroyDescriptorSetLayout(Device, It->second, nullptr);
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
		ResContext.StorageBuffers.clear();

		free(ResContext.ImageViews.Data);
		free(ResContext.Images.Data);
		free(ResContext.ResourceStates.Data);
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

		RenderResources::GPUBuffer NewBuffer = {};
		NewBuffer.Buffer = VulkanHelper::CreateBuffer(Device, Description.Capacity, Description.BufferUsageFlag);
		
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, NewBuffer.Buffer, Description.MemoryPropertyFlag);
		NewBuffer.Memory = AllocResult.Memory;
		NewBuffer.Capacity = AllocResult.Size;
		NewBuffer.UsageFlag = Description.BufferUsageFlag;
		NewBuffer.PropertyFlag = Description.MemoryPropertyFlag;
		NewBuffer.StageBarrier = Description.StageBarrier;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.StorageBuffers[Name] = NewBuffer;
	}

	void CreateDescriptorSetLayout(const std::string& Name, const DescriptorSetLayoutDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = static_cast<u32>(Description.Bindings.size());
		LayoutCreateInfo.pBindings = Description.Bindings.data();
		LayoutCreateInfo.flags = Description.Flags;
		LayoutCreateInfo.pNext = Description.Next;

		VkDescriptorSetLayout NewLayout;
		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &NewLayout));
		ResContext.DescriptorSetLayouts[Name] = NewLayout;
	}

	void CreateDescriptorSet(const std::string& Name, const DescriptorSetDescription& Description)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;
		
		VkDescriptorSetLayout Layout = GetSetLayout(Description.Layout);
		VkDescriptorPool Pool = GetDescriptorPool(Description.Pool);
		
		VkDescriptorSetAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = Pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layout;
		
		VkDescriptorSet DescriptorSet;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet));
		ResContext.DescriptorSets[Name] = DescriptorSet;

		if (!Description.Bindings.empty())
		{
			std::vector<VkWriteDescriptorSet> WriteDescriptorSets;
			std::vector<VkDescriptorBufferInfo> BufferInfos;

			for (const auto& Binding : Description.Bindings)
			{
				RenderResources::GPUBuffer* Buffer = GetGPUBuffer(Binding.Buffer);
				if (Buffer)
				{
					VkDescriptorBufferInfo BufferInfo = {};
					BufferInfo.buffer = Buffer->Buffer;
					BufferInfo.offset = 0;
					BufferInfo.range = Buffer->Capacity;
					BufferInfos.push_back(BufferInfo);

					VkWriteDescriptorSet WriteDescriptorSet = {};
					WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					WriteDescriptorSet.dstSet = DescriptorSet;
					WriteDescriptorSet.dstBinding = Binding.Binding;
					WriteDescriptorSet.dstArrayElement = 0;
					WriteDescriptorSet.descriptorType = Binding.DescriptorType;
					WriteDescriptorSet.descriptorCount = 1;
					WriteDescriptorSet.pBufferInfo = &BufferInfos.back();
					WriteDescriptorSet.pImageInfo = nullptr;
					WriteDescriptorSets.push_back(WriteDescriptorSet);
				}
			}

			if (!WriteDescriptorSets.empty())
			{
				vkUpdateDescriptorSets(Device, static_cast<u32>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, nullptr);
			}
		}
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

	VkDescriptorSet GetDescriptorSet(const std::string& Id)
	{
		auto It = ResContext.DescriptorSets.find(Id);
		if (It != ResContext.DescriptorSets.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	ResourceHandle CreateImageResource(ImageDescription* Description)
	{
		assert(ResContext.Images.Count < ResContext.Images.Capacity);

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;
		VkQueue TransferQueue = RenderResources::GetCoreContext()->GraphicsQueue;

		ImageResource* Resource = &ResContext.Images.Data[ResContext.Images.Count];
		Resource->IsLoaded = false;

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

		VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, nullptr, &Resource->Image));

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			Resource->Image, VulkanHelper::MemoryPropertyFlag::GPULocal);
		Resource->Memory = AllocResult.Memory;
		//Resource->Alignment = AllocResult.Alignment;
		//NextTexture->MeshTexture.Size = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindImageMemory(Device, Resource->Image, Resource->Memory, 0));

		return PackResourceHandle(ResourceType::Texture, ResContext.Images.Count++);
	}

	ImageViewHandle CreateImageView(ResourceHandle Handle, VkFormat Format)
	{
		const u32 Index = GetResourceCPUIndex(Handle);

		VkImageView* View = ResContext.ImageViews.Data + ResContext.ImageViews.Count;

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;
		ViewCreateInfo.image = ResContext.Images.Data[Index].Image;

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, View));
	}

	ResourceHandle CreateBufferResource()
	{
		assert(ResContext.ResourceStates.Count < ResContext.ResourceStates.Capacity);

		ResContext.ResourceStates.Data[ResContext.ResourceStates.Count] = false;
		return PackResourceHandle(ResourceType::StorageResource, ResContext.ResourceStates.Count++);
	}

	void BindImageView(ImageViewHandle Handle, const std::string* Set, u64 ArrayElement)
	{
		VkDescriptorImageInfo DiffuseImageInfo = { };
		DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DiffuseImageInfo.sampler = RenderResources::GetSampler("DiffuseTexture");
		DiffuseImageInfo.imageView = Handle;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = RenderResources::GetSampler("SpecularTexture");
		SpecularImageInfo.imageView = Handle;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = ResContext.DescriptorSets["BindlesTexturesSet"];
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = ArrayElement;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = ResContext.DescriptorSets["BindlesTexturesSet"];
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = ArrayElement;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(RenderResources::GetCoreContext()->LogicalDevice, 2, Writes, 0, nullptr);
	}

	void UpdateGPUBuffer(ResourceHandle Handle, void* Data, u32 DataSize, u64 Offset, const std::string& BufferName)
	{
		RenderResources::GPUBuffer* Buffer = GetGPUBuffer(BufferName);

		if (Buffer->PropertyFlag == VulkanHelper::MemoryPropertyFlag::HostCompatible)
		{
			VkDevice Device = ResContext.CoreContext.LogicalDevice;
			VkPhysicalDevice PhysicalDevice = ResContext.CoreContext.PhysicalDevice;
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, Buffer->Memory, DataSize, Offset, Data);
			OnResourceLoaded(Handle);
		}
		else if (Buffer->PropertyFlag == VulkanHelper::MemoryPropertyFlag::GPULocal)
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
			Task.Handle = Handle;
			Task.DataDescr.StageBarrier = Buffer->StageBarrier;
			Task.Type = TransferSystem::TaskType::Data;

			TransferSystem::AddTask(&Task);
		}
		else
		{
			assert(false);
		}
	}

	void UpdateImageResource(ResourceHandle Handle, ImageDescription* Description, void* Data)
	{
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
		Task.Handle = PackResourceHandle(ResourceType::Texture, ResContext.TextureCount);
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
	}

	bool IsResourceReady(ResourceHandle Handle)
	{
		const ResourceType Type = GetResourceType(Handle);
		const u32 Index = GetResourceCPUIndex(Handle);

		switch (Type)
		{
			case RenderResources::ResourceType::Texture:
				if (!ResContext.Textures[Index].MeshTexture.IsLoaded)
				{
					return false;
				}

				break;
			case RenderResources::ResourceType::StorageResource:
				if (!ResContext.ResourceStates.Data[Index])
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
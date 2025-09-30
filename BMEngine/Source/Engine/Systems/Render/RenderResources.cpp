#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
		std::unordered_map<std::string, VkDescriptorPool> DescriptorPools;
		std::unordered_map<std::string, VkDescriptorSet> DescriptorSets;
		std::unordered_map<std::string, VkPipeline> Pipelines;
		std::unordered_map<std::string, VkPipelineLayout> PipelineLayouts;

		u32 MaxResourceRecords;
		u32 ResourceRecordCount;

		u32 MaxTextures;
		u32 TextureCount;

		u32 MaxStaticMeshes;
		u32 StaticMeshCount;

		ResourceRecord* ResourceRecords;
		MeshTexture2D* Textures;
		VertexData* StaticMeshes;

		u32 MaxResourceDependency;
		u32 CurrentResourceDependencyIndex;
		ResourceHandle* ResourceDependencyBuffer;
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

	static bool CheckRecordAndDependenciesLoadState(const ResourceContext* Context, const ResourceRecord* Record)
	{
		if (!Record->IsLoaded)
		{
			return false;
		}

		for (u32 i = 0; i < Record->DependencyCount; ++i)
		{
			const ResourceHandle Handle = Context->ResourceDependencyBuffer[Record->Dependency + i];
			const ResourceType Type = GetResourceType(Handle);
			u32 ResourceIndex = GetResourceCPUIndex(Handle);
			
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

	void OnResourceLoaded(ResourceHandle Handle)
	{
		ResourceType Type = GetResourceType(Handle);
		u32 ResourceIndex = GetResourceCPUIndex(Handle);

		switch (Type)
		{
			case ResourceType::Texture:
				ResContext.Textures[ResourceIndex].MeshTexture.IsLoaded = true;
				break;
			case ResourceType::Mesh:
				ResContext.StaticMeshes[ResourceIndex].IsLoaded = true;
				break;
			case ResourceType::StorageResource:
				ResContext.ResourceRecords[ResourceIndex].IsLoaded = true;
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

		ResContext.MaxTextures = 64;
		ResContext.TextureCount = 0;
		ResContext.Textures = (MeshTexture2D*)malloc(ResContext.MaxTextures * sizeof(ResContext.Textures[0]));

		ResContext.MaxStaticMeshes = 30000;
		ResContext.StaticMeshCount = 0;
		ResContext.StaticMeshes = (VertexData*)malloc(ResContext.MaxStaticMeshes * sizeof(ResContext.StaticMeshes[0]));

		ResContext.MaxResourceRecords = 60000;
		ResContext.ResourceRecordCount = 0;
		ResContext.ResourceRecords = (ResourceRecord*)malloc(ResContext.MaxResourceRecords * sizeof(ResContext.ResourceRecords[0]));

		ResContext.MaxResourceDependency = 10000;
		ResContext.CurrentResourceDependencyIndex = 0;
		ResContext.ResourceDependencyBuffer = (ResourceHandle*)malloc(ResContext.MaxResourceDependency * sizeof(ResContext.ResourceDependencyBuffer[0]));
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

		for (uint32_t i = 0; i < ResContext.TextureCount; ++i)
		{
			vkDestroyImageView(Device, ResContext.Textures[i].View, nullptr);
			vkDestroyImage(Device, ResContext.Textures[i].MeshTexture.Image, nullptr);
			vkFreeMemory(Device, ResContext.Textures[i].MeshTexture.Memory, nullptr);
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
		ResContext.MeshBuffers.clear();

		free(ResContext.Textures);
		free(ResContext.StaticMeshes);
		free(ResContext.ResourceRecords);
		free(ResContext.ResourceDependencyBuffer);
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
		NewBuffer.StageBarrier = Description.StageBarrier;

		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, NewBuffer.Buffer, NewBuffer.Memory, 0));

		ResContext.MeshBuffers[Name] = NewBuffer;
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
	}

	void PostCreateInit()
	{
		VkDescriptorBufferInfo MaterialBufferInfo;
		RenderResources::StorageBuffer* MaterialBuffer = GetStorageBuffer("MaterialBuffer");
		MaterialBufferInfo.buffer = MaterialBuffer->Buffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = MaterialBuffer->MaxRecords * MaterialBuffer->RecordSize;

		VkWriteDescriptorSet WriteDescriptorSet = { };
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = ResContext.DescriptorSets["MaterialSet"];
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &MaterialBufferInfo;
		WriteDescriptorSet.pImageInfo = nullptr;

		vkUpdateDescriptorSets(ResContext.CoreContext.LogicalDevice, 1, &WriteDescriptorSet, 0, nullptr);
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
		Task.DataDescr.StageBarrier = VertexBuffer->StageBarrier;
		Task.Type = TransferSystem::TaskType::Data;

		AddTask(&Task);

		VertexBuffer->Offset += DataSize;
		u32 Index = ResContext.StaticMeshCount++;
		return PackResourceHandle(ResourceType::Mesh, Index);
	}

	ResourceHandle CreateTexture(TextureDescription* Description, void* Data)
	{
		assert(ResContext.TextureCount < ResContext.MaxTextures);

		VkDevice Device = RenderResources::GetCoreContext()->LogicalDevice;
		VkPhysicalDevice PhysicalDevice = RenderResources::GetCoreContext()->PhysicalDevice;
		VkQueue TransferQueue = RenderResources::GetCoreContext()->GraphicsQueue;

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
		DiffuseImageInfo.sampler = RenderResources::GetSampler("DiffuseTexture");
		DiffuseImageInfo.imageView = NextTexture->View;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = RenderResources::GetSampler("SpecularTexture");
		SpecularImageInfo.imageView = NextTexture->View;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = ResContext.DescriptorSets["BindlesTexturesSet"];
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = Index;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = ResContext.DescriptorSets["BindlesTexturesSet"];
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = Index;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(RenderResources::GetCoreContext()->LogicalDevice, 2, Writes, 0, nullptr);

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
		Task.Type = TransferSystem::TaskType::Image;

		AddTask(&Task);
		ResContext.TextureCount++;
		return PackResourceHandle(ResourceType::Texture, Index);
	}

	ResourceHandle CreateStorageBufferResource(u32 DataSize, const std::string& BufferName, ResourceDependency Dependency, u32 DependencyCount)
	{
		assert(ResContext.ResourceRecordCount < ResContext.MaxResourceRecords);

		RenderResources::StorageBuffer* Buffer = GetStorageBuffer(BufferName);

		ResourceRecord* NewResource = ResContext.ResourceRecords + ResContext.ResourceRecordCount;
		NewResource->IsLoaded = false;
		NewResource->Dependency = Dependency;
		NewResource->DependencyCount = DependencyCount;
		NewResource->RecordGPUIndex = Buffer->Count;
		NewResource->RecordSize = Buffer->RecordSize;
		
		Buffer->Count++;
		u32 Index = ResContext.ResourceRecordCount++;
		return PackResourceHandle(ResourceType::StorageResource, Index);
	}

	ResourceDependency CreateResourceDependency(ResourceHandle* Handles, u32 HandlesCount)
	{
		assert(ResContext.CurrentResourceDependencyIndex + HandlesCount < ResContext.MaxResourceDependency);

		ResourceDependency Dependency = ResContext.CurrentResourceDependencyIndex;

		for (u32 i = 0; i < HandlesCount; ++i)
		{
			ResContext.ResourceDependencyBuffer[ResContext.CurrentResourceDependencyIndex++] = Handles[i];	
		}

		return Dependency;
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
			OnResourceLoaded(Handle);
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
			Task.Type = TransferSystem::TaskType::Data;

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
		u32 MeshIndex = GetResourceCPUIndex(Entity->StaticMeshHandle);
		VertexData* MeshResource = ResContext.StaticMeshes + MeshIndex;
		if (!MeshResource->IsLoaded) return false;

		u32 InstanceIndex = GetResourceCPUIndex(Entity->InstanceDataHandle);
		ResourceRecord* InstanceResource = ResContext.ResourceRecords + InstanceIndex;
		return CheckRecordAndDependenciesLoadState(&ResContext, InstanceResource);
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
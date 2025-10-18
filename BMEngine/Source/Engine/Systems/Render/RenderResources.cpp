#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"
#include "RenderInterface.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

extern Memory::Array<DescriptorSetLayoutBinding> LayoutBindings;

namespace RenderResources
{
	struct GPUBufferEntry
	{
		std::atomic<bool> IsLoaded;
		GPUBuffer* GPUBufferHandle; // GPUBuffer* is TMP, use handle
		u64 BufferOffset;
		u64 Size;
	};

	struct ResourceContext
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
		std::unordered_map<std::string, BmRender_Sampler> Samplers;
		std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
		std::unordered_map<std::string, BmRender_Shader> Shaders;
		std::unordered_map<std::string, RenderResources::GPUBuffer> StorageBuffers;
		std::unordered_map<std::string, BmRender_DescriptorPool> DescriptorPools;
		std::unordered_map<std::string, DescriptorSet> DescriptorSets;
		std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
		std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;

		Memory::Array<VkPushConstantRange> PushConstants;
		Memory::Array<GPUBufferEntry> ResourceRecords;
		Memory::Array<VkImageView> ImageViews;
	};

	static ResourceContext ResContext;

	void OnBufferResourceLoaded(BmRender_BufferRegion Handle)
	{
		ResContext.ResourceRecords.Data[(u64)Handle].IsLoaded = true;
	}

	void OnImageResourceLoaded(BmRender_Image Handle)
	{
		BmRender_GetIamgeData(Handle)->IsLoaded = true;
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

		BmRender_DescriptorPoolDescription PoolDesc = {};
		PoolDesc.MaxSets = TotalDescriptorCount;
		PoolDesc.PoolSizeCount = PoolSizeCount;
		PoolDesc.PoolSizes = TotalPassPoolSizes;
		PoolDesc.Flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		PoolDesc.Next = nullptr;

		BmRender_DescriptorPool MainPoolHandle = BmRender_CreateDescriptorPool(&PoolDesc);
		ResContext.DescriptorPools["MainPool"] = MainPoolHandle;

		ResContext.ImageViews.Capacity = 64;
		ResContext.ImageViews.Count = 0;
		ResContext.ImageViews.Data = (VkImageView*)malloc(ResContext.ImageViews.Capacity * sizeof(ResContext.ImageViews.Data[0]));

		ResContext.ResourceRecords.Capacity = 60000;
		ResContext.ResourceRecords.Count = 0;
		ResContext.ResourceRecords.Data = (GPUBufferEntry*)malloc(ResContext.ResourceRecords.Capacity * sizeof(ResContext.ResourceRecords.Data[0]));

		ResContext.PushConstants.Capacity = 10;
		ResContext.PushConstants.Count = 0;
		ResContext.PushConstants.Data = (VkPushConstantRange*)malloc(ResContext.PushConstants.Capacity * sizeof(ResContext.PushConstants.Data[0]));
	}

	void CreateGraphicsPipeline(const std::string& Name, const BmRender_PipelineDescription& Description)
	{
		// Use the new interface function to create the pipeline
		BmRender_Pipeline PipelineHandle = BmRender_CreatePipeline(&Description);
		
		// Store the pipeline handle in the map
		ResContext.Pipelines[Name] = PipelineHandle;
	}

	void CreatePipelineLayout(const std::string& Name, const BmRender_PipelineLayoutDescription& Description)
	{
		// Use the new interface function to create the pipeline layout
		BmRender_PipelineLayout PipelineLayoutHandle = BmRender_CreatePipelineLayout(&Description);
		
		// Store the pipeline layout handle in the map
		ResContext.PipelineLayouts[Name] = PipelineLayoutHandle;
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
			return BmRender_GetPipelineData(it->second)->VulkanPipeline;
		}
		return VK_NULL_HANDLE;
	}

	VkPipelineLayout GetPipelineLayout(const std::string& Name)
	{
		auto it = ResContext.PipelineLayouts.find(Name);
		if (it != ResContext.PipelineLayouts.end())
		{
			return BmRender_GetPipelineLayoutData(it->second)->VulkanPipelineLayout;
		}
		return VK_NULL_HANDLE;
	}

	VkImage GetImage(BmRender_Image Handle)
	{
		return BmRender_GetIamgeData(Handle)->Image;
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

		for (auto It = ResContext.StorageBuffers.begin(); It != ResContext.StorageBuffers.end(); ++It)
		{
			vkDestroyBuffer(Device, It->second.Buffer, nullptr);
			vkFreeMemory(Device, It->second.Memory, nullptr);
		}

		VulkanCoreContext::DestroyCoreContext(&ResContext.CoreContext);

		ResContext.Shaders.clear();
		ResContext.Samplers.clear();
		ResContext.DescriptorSetLayouts.clear();
		ResContext.VBindings.clear();

		free(ResContext.ImageViews.Data);
		free(ResContext.ResourceRecords.Data);
		free(ResContext.PushConstants.Data);
	}

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding)
	{
		ResContext.VBindings[Name] = Binding;
	}

	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize)
	{
		BmRender_ShaderDescription ShaderDesc = {};
		ShaderDesc.Code = Code;
		ShaderDesc.CodeSize = CodeSize;
		
		BmRender_Shader ShaderHandle = BmRender_CreateShader(&ShaderDesc);
		ResContext.Shaders[Name] = ShaderHandle;
	}

	void CreateSampler(const std::string& Name, const BmRHI_SamplerDescription& Data)
	{
		ResContext.Samplers[Name] = BmRender_CreateSampler(&Data);
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

		ResContext.DescriptorSetLayouts[Name] = BmRender_CreateDescriptorSetLayout(&Description);
	}

	void UpdateDescriptorSet(std::string DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		DescriptorSet* Set = GetDescriptorSet(DescriptorSetName);
		DescriptorSetLayoutData* Layout = GetSetLayout(Set->Layout);

		VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Render::FrameAlloc(sizeof(VkWriteDescriptorSet) * BindingsCount);

		for (u32 i = 0; i < BindingsCount; i++)
		{
			const BmRender_DescriptorSetBinding& Binding = Bindings[i];

			VkDescriptorType DescriptorType = LayoutBindings.Data[Layout->BindingsIndex + i].DescriptorType;

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

		DescriptorSetLayoutData* Layout = GetSetLayout(LayoutName);
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
			return BmRender_GetSamplerData(It->second)->VulkanSampler;
		}

		assert(false);
		return nullptr;
	}

	DescriptorSetLayoutData* GetSetLayout(const std::string& Id)
	{
		auto It = ResContext.DescriptorSetLayouts.find(Id);
		if (It != ResContext.DescriptorSetLayouts.end())
		{
			return BmRender_GetDescriptorSetLayoutData(It->second);
		}

		assert(false);
		return nullptr;
	}

	VkShaderModule GetShader(const std::string& Id)
	{
		auto It = ResContext.Shaders.find(Id);
		if (It != ResContext.Shaders.end())
		{
			return BmRender_GetShaderData(It->second)->VulkanShaderModule;
		}

		assert(false);
		return VK_NULL_HANDLE;
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
			return BmRender_GetDescriptorPoolData(It->second)->VulkanDescriptorPool;
		}

		assert(false);
		return VK_NULL_HANDLE;
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

	BmRender_ImageViewResource CreateImageView(BmRender_Image Handle, u32 BaseArrayLayer, u32 LayerCount, VkImageViewType ViewType, VkImageAspectFlags AspectFlags)
	{
		ImageResource* Resource = BmRender_GetIamgeData(Handle);
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

	void UpdateImageResource(BmRender_Image Handle, BmRender_ImageDescription* Description, void* Data)
	{
		ImageResource* Image = BmRender_GetIamgeData(Handle);

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

	bool IsImageResourceReady(BmRender_Image Handle)
	{
		return BmRender_GetIamgeData(Handle)->IsLoaded;
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
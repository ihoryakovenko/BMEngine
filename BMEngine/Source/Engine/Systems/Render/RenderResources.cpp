#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "TransferSystem.h"
#include "RenderInterface.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	struct GPUBufferEntryData
	{
		std::atomic<bool> IsLoaded;
		BmRender_GPUBuffer GPUBufferHandle;
		u64 BufferOffset;
		u64 Size;
	};

	struct ResourceContext
	{
		VulkanCoreContext::VulkanCoreContext CoreContext;
		std::unordered_map<std::string, VulkanHelper::VertexBinding> VBindings;
		std::unordered_map<std::string, BmRender_DescriptorSet> DescriptorSets;

		std::unordered_map<std::string, BmRender_Sampler> Samplers;
		std::unordered_map<std::string, BmRender_DescriptorSetLayout> DescriptorSetLayouts;
		std::unordered_map<std::string, BmRender_Shader> Shaders;
		std::unordered_map<std::string, BmRender_DescriptorPool> DescriptorPools;
		std::unordered_map<std::string, BmRender_Pipeline> Pipelines;
		std::unordered_map<std::string, BmRender_PipelineLayout> PipelineLayouts;
		std::unordered_map<std::string, BmRender_GPUBuffer> StorageBuffers;

		Memory::Array<VkPushConstantRange> PushConstants;
		Memory::Array<GPUBufferEntryData> ResourceRecords;
	};

	static ResourceContext ResContext;

	void OnBufferResourceLoaded(BmRender_BufferRegion Handle)
	{
		ResContext.ResourceRecords.Data[(u64)Handle].IsLoaded = true;
	}

	void OnImageResourceLoaded(BmRender_Image Handle)
	{
		GetImageData(Handle)->IsLoaded = true;
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

		ResContext.ResourceRecords.Capacity = 60000;
		ResContext.ResourceRecords.Count = 0;
		ResContext.ResourceRecords.Data = (GPUBufferEntryData*)malloc(ResContext.ResourceRecords.Capacity * sizeof(GPUBufferEntryData));

		ResContext.PushConstants.Capacity = 10;
		ResContext.PushConstants.Count = 0;
		ResContext.PushConstants.Data = (VkPushConstantRange*)malloc(ResContext.PushConstants.Capacity * sizeof(VkPushConstantRange));
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

	void CreateGPUBuffer(BmRender_GPUBuffer Buffer, const std::string& Name)
	{
		ResContext.StorageBuffers[Name] = Buffer;
	}

	VkPipeline GetPipeline(const std::string& Name)
	{
		auto it = ResContext.Pipelines.find(Name);
		if (it != ResContext.Pipelines.end())
		{
			return GetPipelineData(it->second)->VulkanPipeline;
		}
		return VK_NULL_HANDLE;
	}

	VkPipelineLayout GetPipelineLayout(const std::string& Name)
	{
		auto it = ResContext.PipelineLayouts.find(Name);
		if (it != ResContext.PipelineLayouts.end())
		{
			return GetPipelineLayoutData(it->second)->VulkanPipelineLayout;
		}
		return VK_NULL_HANDLE;
	}

	VkImage GetImage(BmRender_Image Handle)
	{
		return GetImageData(Handle)->Image;
	}

	VkImageView GetImageView(BmRender_ImageView Handle)
	{
		return GetImageViewData(Handle)->View;
	}

	VkPushConstantRange GetPushConstant(BmRender_PushConstant Handle)
	{
		return ResContext.PushConstants.Data[(u64)Handle];
	}

	void DeInit()
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		VulkanCoreContext::DestroyCoreContext(&ResContext.CoreContext);

		ResContext.Shaders.clear();
		ResContext.Samplers.clear();
		ResContext.DescriptorSetLayouts.clear();
		ResContext.VBindings.clear();

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

	void CreateDescriptorSetLayout(const std::string& Name, const BmRender_DescriptorSetLayoutDescription& Description)
	{

		ResContext.DescriptorSetLayouts[Name] = BmRender_CreateDescriptorSetLayout(&Description);
	}

	void UpdateDescriptorSet(std::string DescriptorSetName, const BmRender_DescriptorSetBinding* Bindings, u64 BindingsCount)
	{
		VkDevice Device = ResContext.CoreContext.LogicalDevice;

		DescriptorSetData* Set = GetDescriptorSet(DescriptorSetName);
		DescriptorSetLayoutData* Layout = GetDescriptorSetLayoutData(Set->Layout);

		VkWriteDescriptorSet* WriteDescriptorSets = (VkWriteDescriptorSet*)Render::FrameAlloc(sizeof(VkWriteDescriptorSet) * BindingsCount);

		for (u32 i = 0; i < BindingsCount; i++)
		{
			const BmRender_DescriptorSetBinding& Binding = Bindings[i];

			VkDescriptorType DescriptorType = Layout->LayoutBindings[i].DescriptorType;

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
					GPUBufferEntryData* Entry = ResContext.ResourceRecords.Data + (u64)Binding.BufferRegions[j];

					BufferInfo[j].buffer = GetGPUBufferData(Entry->GPUBufferHandle)->Buffer;
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

	void CreateDescriptorSet(const std::string& Name, BmRender_DescriptorSet Set)
	{
		ResContext.DescriptorSets[Name] = Set;
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
			return GetSamplerData(It->second)->VulkanSampler;
		}

		assert(false);
		return nullptr;
	}

	DescriptorSetLayoutData* GetSetLayout(const std::string& Id)
	{
		auto It = ResContext.DescriptorSetLayouts.find(Id);
		if (It != ResContext.DescriptorSetLayouts.end())
		{
			return GetDescriptorSetLayoutData(It->second);
		}

		assert(false);
		return nullptr;
	}

	BmRender_DescriptorSetLayout GetDescriptorSetLayoutHandle(const std::string& Id)
	{
		auto It = ResContext.DescriptorSetLayouts.find(Id);
		if (It != ResContext.DescriptorSetLayouts.end())
		{
			return It->second;
		}

		assert(false);
		return BmRender_DescriptorSetLayout{};
	}

	VkShaderModule GetShader(const std::string& Id)
	{
		auto It = ResContext.Shaders.find(Id);
		if (It != ResContext.Shaders.end())
		{
			return GetShaderData(It->second)->VulkanShaderModule;
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
			return GetDescriptorPoolData(It->second)->VulkanDescriptorPool;
		}

		assert(false);
		return VK_NULL_HANDLE;
	}

	DescriptorSetData* GetDescriptorSet(const std::string& Id)
	{
		auto It = ResContext.DescriptorSets.find(Id);
		if (It != ResContext.DescriptorSets.end())
		{
			
			return GetDescriptorSetData(It->second);
		}

		assert(false);
		return nullptr;
	}

	BmRender_BufferRegion CreateBufferRegion(u64 BufferOffset, u64 RegionSize, const std::string& BufferName)
	{
		assert(ResContext.ResourceRecords.Count < ResContext.ResourceRecords.Capacity);

		GPUBufferEntryData* Entry = ResContext.ResourceRecords.Data + ResContext.ResourceRecords.Count;
		Entry->IsLoaded = false;
		Entry->BufferOffset = BufferOffset;
		Entry->Size = RegionSize;
		Entry->GPUBufferHandle = ResContext.StorageBuffers[BufferName];
	
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
		GPUBufferEntryData* Entry = ResContext.ResourceRecords.Data + Index;
		GPUBufferData* Buffer = GetGPUBufferData(Entry->GPUBufferHandle);

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
		ImageResource* Image = GetImageData(Handle);

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
		return GetImageData(Handle)->IsLoaded;
	}

	GPUBufferData* GetGPUBuffer(const std::string& Name)
	{
		auto It = ResContext.StorageBuffers.find(Name);
		if (It != ResContext.StorageBuffers.end())
		{
			return GetGPUBufferData(It->second);
		}

		assert(false);
		return nullptr;
	}
}
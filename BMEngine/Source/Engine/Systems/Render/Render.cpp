#include "Render.h"

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"
#include "Engine/Systems/Render/DeferredPass.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "RenderResources.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "Util/Util.h"
#include "Util/Settings.h"

namespace Render
{
	static void AddTask(TransferTask* Task, TaskQueue* Queue)
	{
		std::unique_lock<std::mutex> PendingLock(Queue->Mutex);
		Memory::PushToRingBuffer(&Queue->TasksBuffer, Task);
		Render::NotifyTransfer();
	}

	static void InitImGuiPipeline(VkDescriptorPool* ImGuiPool, VulkanCoreContext::VulkanCoreContext* CoreContext, GLFWwindow* Wnd)
	{
		ImGui_ImplGlfw_InitForVulkan(Wnd, true);

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = DeferredPass::GetAttachmentData()->ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = DeferredPass::GetAttachmentData()->ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = DeferredPass::GetAttachmentData()->DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = DeferredPass::GetAttachmentData()->DepthAttachmentFormat;

		VkDescriptorPoolSize PoolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};
		VkDescriptorPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		PoolInfo.maxSets = 1;
		PoolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(PoolSizes);
		PoolInfo.pPoolSizes = PoolSizes;
		vkCreateDescriptorPool(CoreContext->LogicalDevice, &PoolInfo, nullptr, ImGuiPool);

		ImGui_ImplVulkan_InitInfo InitInfo = { };
		InitInfo.Instance = CoreContext->VulkanInstance;
		InitInfo.PhysicalDevice = CoreContext->PhysicalDevice;
		InitInfo.Device = CoreContext->LogicalDevice;
		InitInfo.QueueFamily = CoreContext->Indices.GraphicsFamily;
		InitInfo.Queue = CoreContext->GraphicsQueue;
		InitInfo.PipelineCache = nullptr;
		InitInfo.DescriptorPool = *ImGuiPool;
		InitInfo.RenderPass = nullptr;
		InitInfo.UseDynamicRendering = true;
		InitInfo.MinImageCount = 2;
		InitInfo.ImageCount = 3;
		InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		InitInfo.PipelineRenderingCreateInfo = RenderingInfo;
		ImGui_ImplVulkan_Init(&InitInfo);

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	static void DeInitImGuiPipeline(VkDevice Device, VkDescriptorPool ImGuiPool)
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	}

	static void InitStaticMeshPipeline(VkDevice Device, const ResourceStorage* Storage, StaticMeshPipeline* MeshPipeline)
	{
		MeshPipeline->ShadowMapArraySampler = RenderResources::GetSampler("ShadowMap");

		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		MeshPipeline->StaticMeshLightLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		MeshPipeline->EntityLightBufferHandle = FrameManager::ReserveUniformMemory(LightBufferSize);
		MeshPipeline->StaticMeshLightSet = FrameManager::CreateAndBindSet(MeshPipeline->EntityLightBufferHandle, LightBufferSize, MeshPipeline->StaticMeshLightLayout);

		MeshPipeline->ShadowMapArrayLayout = RenderResources::GetSetLayout("ShadowMapArrayLayout");

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VkImageViewCreateInfo ViewCreateInfo = {};
			ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo.image = LightningPass::GetShadowMapArray()[i].Image;
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			ViewCreateInfo.format = DepthFormat;
			ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo.subresourceRange.levelCount = 1;
			ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo.subresourceRange.layerCount = 2;
			ViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &MeshPipeline->ShadowMapArrayImageInterface[i]));

			VkDescriptorImageInfo ShadowMapArrayImageInfo;
			ShadowMapArrayImageInfo.imageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapArrayImageInfo.sampler = MeshPipeline->ShadowMapArraySampler;
			ShadowMapArrayImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorSetAllocateInfo AllocInfo = {};
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = VulkanInterface::GetDescriptorPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &MeshPipeline->ShadowMapArrayLayout;
			
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, MeshPipeline->ShadowMapArraySet + i));

			VkWriteDescriptorSet WriteDescriptorSet = {};
			WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet.dstSet = MeshPipeline->ShadowMapArraySet[i];
			WriteDescriptorSet.dstBinding = 0;
			WriteDescriptorSet.dstArrayElement = 0;
			WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSet.descriptorCount = 1;
			WriteDescriptorSet.pBufferInfo = nullptr;
			WriteDescriptorSet.pImageInfo = &ShadowMapArrayImageInfo;

			vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
		}

		VkDescriptorSetLayout StaticMeshDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			Storage->Textures.BindlesTexturesLayout,
			MeshPipeline->StaticMeshLightLayout,
			Storage->Materials.MaterialLayout,
			MeshPipeline->ShadowMapArrayLayout
		};

		const u32 StaticMeshDescriptorLayoutCount = sizeof(StaticMeshDescriptorLayouts) / sizeof(StaticMeshDescriptorLayouts[0]);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = StaticMeshDescriptorLayoutCount;
		PipelineLayoutCreateInfo.pSetLayouts = StaticMeshDescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.pPushConstantRanges = &MeshPipeline->PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &MeshPipeline->Pipeline.PipelineLayout));

		VulkanHelper::BMRVertexInputBinding VertexInputBinding[2];
		VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Position) };
		VertexInputBinding[0].InputAttributes[1] = { "TextureCoords", VK_FORMAT_R32G32_SFLOAT, offsetof(StaticMeshVertex, TextureCoords) };
		VertexInputBinding[0].InputAttributes[2] = { "Normal", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Normal) };
		VertexInputBinding[0].InputAttributesCount = 3;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(StaticMeshVertex);
		VertexInputBinding[0].VertexInputBindingName = "EntityVertex";

		u32 Offset = 0;
		VertexInputBinding[1].InputAttributes[0] = { "InstanceModel0", VK_FORMAT_R32G32B32A32_SFLOAT, Offset }; Offset += sizeof(glm::vec4);
		VertexInputBinding[1].InputAttributes[1] = { "InstanceModel1", VK_FORMAT_R32G32B32A32_SFLOAT, Offset }; Offset += sizeof(glm::vec4);
		VertexInputBinding[1].InputAttributes[2] = { "InstanceModel2", VK_FORMAT_R32G32B32A32_SFLOAT, Offset }; Offset += sizeof(glm::vec4);
		VertexInputBinding[1].InputAttributes[3] = { "InstanceModel3", VK_FORMAT_R32G32B32A32_SFLOAT, Offset }; Offset += sizeof(glm::vec4);
		VertexInputBinding[1].InputAttributes[4] = { "InstanceMaterialIndex", VK_FORMAT_R32_UINT, Offset }; Offset += sizeof(u32);
		VertexInputBinding[1].InputAttributesCount = 5;
		VertexInputBinding[1].InputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
		VertexInputBinding[1].Stride = sizeof(InstanceData);
		VertexInputBinding[1].VertexInputBindingName = "InstanceData";

		VulkanHelper::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/StaticMesh.yaml");
		PipelineSettings.Extent = MainScreenExtent;

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = MeshPipeline->Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		MeshPipeline->Pipeline.Pipeline = VulkanHelper::BatchPipelineCreation(Device, PipelineSettings.Shaders.Data, PipelineSettings.Shaders.Count, VertexInputBinding, 2,
			&PipelineSettings, &ResourceInfo);
	}

	static void DeInitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline)
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, MeshPipeline->ShadowMapArrayImageInterface[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, MeshPipeline->StaticMeshLightLayout, nullptr);

		vkDestroyPipeline(Device, MeshPipeline->Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, MeshPipeline->Pipeline.PipelineLayout, nullptr);
	}

	static void DrawStaticMeshes(VkDevice Device, VkCommandBuffer CmdBuffer, const ResourceStorage* Storage, StaticMeshPipeline* MeshPipeline, const DrawScene* Scene)
	{
		FrameManager::UpdateUniformMemory(MeshPipeline->EntityLightBufferHandle, Scene->LightEntity, sizeof(LightBuffer));

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.Pipeline);

		const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet();
		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			0, 1, &VpSet, 1, &DynamicOffset);

		VkDescriptorSet BindlesTexturesSet = Storage->Textures.BindlesTexturesSet;
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			1, 1, &BindlesTexturesSet, 0, nullptr);

		const u32 LightDynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(LightBuffer);

		for (u32 i = 0; i < Storage->DrawEntities.Count; ++i)
		{
			DrawEntity* DrawEntity = Storage->DrawEntities.Data + i;
			if (!IsDrawEntityLoaded(Storage, DrawEntity))
			{
				continue;
			}

			RenderResource<StaticMesh>* MeshResource = Storage->Meshes.StaticMeshes.Data + DrawEntity->StaticMeshIndex;
			StaticMesh* Mesh = &MeshResource->Resource;

			RenderResource<InstanceData>* InstanceResource = Storage->Meshes.MeshInstances.Data + DrawEntity->InstanceDataIndex;
			InstanceData* Instance = &InstanceResource->Resource;

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				MeshPipeline->StaticMeshLightSet,
				Storage->Materials.MaterialSet,
				MeshPipeline->ShadowMapArraySet[VulkanInterface::TestGetImageIndex()],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			const VkBuffer Buffers[] =
			{
				Storage->Meshes.VertexStageData.Buffer,
				Storage->Meshes.GPUInstances.Buffer
			};

			const u64 Offsets[] = 
			{
				Mesh->VertexOffset,
				sizeof(Render::InstanceData)* DrawEntity->InstanceDataIndex
			};

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
				2, DescriptorSetGroupCount, DescriptorSetGroup, 1, &LightDynamicOffset);

			vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
			vkCmdBindIndexBuffer(CmdBuffer, Storage->Meshes.VertexStageData.Buffer, Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, DrawEntity->Instances, 0, 0, 0);
		}
	}

	static void InitDrawState(VkDevice Device, u32 GraphicsFamily, u32 MaxDrawFrames, DrawState* State)
	{
		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = GraphicsFamily;

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, nullptr, &State->GraphicsCommandPool));
		
		VkCommandBufferAllocateInfo GraphicsCommandBufferAllocateInfo = { };
		GraphicsCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		GraphicsCommandBufferAllocateInfo.commandPool = State->GraphicsCommandPool;
		GraphicsCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		GraphicsCommandBufferAllocateInfo.commandBufferCount = MaxDrawFrames;

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &GraphicsCommandBufferAllocateInfo, State->Frames.CommandBuffers));

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (u64 i = 0; i < MaxDrawFrames; i++)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, &State->Frames.Fences[i]));
			VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &State->Frames.ImagesAvailable[i]));
			VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &State->Frames.RenderFinished[i]));
		}

		const u32 PoolSizeCount = 11;
		auto TotalPassPoolSizes = Memory::FramePointer<VkDescriptorPoolSize>::Create(PoolSizeCount);
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
		PoolCreateInfo.pPoolSizes = TotalPassPoolSizes.Data;
		PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VULKAN_CHECK_RESULT(vkCreateDescriptorPool(Device, &PoolCreateInfo, nullptr, &State->MainPool));
	}

	static void DeInitDrawState(VkDevice Device, u32 MaxDrawFrames, DrawState* State)
	{
		vkDestroyCommandPool(Device, State->GraphicsCommandPool, nullptr);

		for (u64 i = 0; i < MaxDrawFrames; i++)
		{
			vkDestroyFence(Device, State->Frames.Fences[i], nullptr);
			vkDestroySemaphore(Device, State->Frames.ImagesAvailable[i], nullptr);
			vkDestroySemaphore(Device, State->Frames.RenderFinished[i], nullptr);
		}

		vkDestroyDescriptorPool(Device, State->MainPool, nullptr);
	}

	static void InitStaticMeshStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Storage, u64 VertexCapacity, u64 InstanceCapacity)
	{
		*Storage = { };
		Storage->StaticMeshes = Memory::AllocateArray<RenderResource<StaticMesh>>(512);
		Storage->MeshInstances = Memory::AllocateArray<RenderResource<InstanceData>>(512);

		Storage->VertexStageData.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::CombinedVertexIndexFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Storage->VertexStageData.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Storage->VertexStageData.Memory = AllocResult.Memory;
		Storage->VertexStageData.Alignment = AllocResult.Alignment;
		Storage->VertexStageData.Capacity = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, Storage->VertexStageData.Buffer, Storage->VertexStageData.Memory, 0));

		Storage->GPUInstances.Buffer = VulkanHelper::CreateBuffer(Device, InstanceCapacity, VulkanHelper::BufferUsageFlag::InstanceFlag);
		AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Storage->GPUInstances.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Storage->GPUInstances.Memory = AllocResult.Memory;
		Storage->GPUInstances.Alignment = AllocResult.Alignment;
		Storage->GPUInstances.Capacity = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, Storage->GPUInstances.Buffer, Storage->GPUInstances.Memory, 0));
	}

	static void DeInitStaticMeshStorage(VkDevice Device, StaticMeshStorage* Storage)
	{
		vkDestroyBuffer(Device, Storage->VertexStageData.Buffer, nullptr);
		vkFreeMemory(Device, Storage->VertexStageData.Memory, nullptr);
		vkDestroyBuffer(Device, Storage->GPUInstances.Buffer, nullptr);
		vkFreeMemory(Device, Storage->GPUInstances.Memory, nullptr);
		Memory::FreeArray(&Storage->StaticMeshes);
		Memory::FreeArray(&Storage->MeshInstances);
	}

	static void InitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
	{
		Storage->Materials = Memory::AllocateArray<RenderResource<Material>>(512);

		const VkDeviceSize MaterialBufferSize = sizeof(Material) * 512;
		Storage->MaterialBuffer = VulkanHelper::CreateBuffer(Device, MaterialBufferSize, VulkanHelper::BufferUsageFlag::StorageFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(VulkanInterface::GetPhysicalDevice(), Device,
			Storage->MaterialBuffer, VulkanHelper::MemoryPropertyFlag::GPULocal);
		Storage->MaterialBufferMemory = AllocResult.Memory;
		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, Storage->MaterialBuffer, Storage->MaterialBufferMemory, 0));

		VkDescriptorBufferInfo MaterialBufferInfo;
		MaterialBufferInfo.buffer = Storage->MaterialBuffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = MaterialBufferSize;

		const VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		const VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		
		Storage->MaterialLayout = RenderResources::GetSetLayout("MaterialLayout");

		VkDescriptorSetAllocateInfo allocInfoMat = {};
		allocInfoMat.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfoMat.descriptorPool = VulkanInterface::GetDescriptorPool();
		allocInfoMat.descriptorSetCount = 1;
		allocInfoMat.pSetLayouts = &Storage->MaterialLayout;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &allocInfoMat, &Storage->MaterialSet));

		VkWriteDescriptorSet WriteDescriptorSet = {};
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = Storage->MaterialSet;
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = MaterialDescriptorType;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &MaterialBufferInfo;
		WriteDescriptorSet.pImageInfo = nullptr;

		vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
	}

	static void DeInitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
	{
		vkDestroyBuffer(Device, Storage->MaterialBuffer, nullptr);
		vkFreeMemory(Device, Storage->MaterialBufferMemory, nullptr);
		Memory::FreeArray(&Storage->Materials);
	}

	static void InitTextureStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, TextureStorage* Storage)
	{
		const u32 MaxTextures = 64;

		Storage->Textures = Memory::AllocateArray<MeshTexture2D>(MaxTextures);

		Storage->DiffuseSampler = RenderResources::GetSampler("DiffuseTexture");
		Storage->SpecularSampler = RenderResources::GetSampler("SpecularTexture");

		Storage->BindlesTexturesLayout = RenderResources::GetSetLayout("BindlesTexturesLayout");

		VkDescriptorSetAllocateInfo AllocInfoTex = {};
		AllocInfoTex.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfoTex.descriptorPool = VulkanInterface::GetDescriptorPool();
		AllocInfoTex.descriptorSetCount = 1;
		AllocInfoTex.pSetLayouts = &Storage->BindlesTexturesLayout;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfoTex, &Storage->BindlesTexturesSet));
	}

	static void DeInitTextureStorage(VkDevice Device, TextureStorage* Storage)
	{
		for (uint32_t i = 0; i < Storage->Textures.Count; ++i)
		{
			vkDestroyImageView(Device, Storage->Textures.Data[i].View, nullptr);
			vkDestroyImage(Device, Storage->Textures.Data[i].MeshTexture.Image, nullptr);
			vkFreeMemory(Device, Storage->Textures.Data[i].MeshTexture.Memory, nullptr);
		}


		Memory::FreeArray(&Storage->Textures);
	}

	static void InitTransferState(VkDevice Device, VkPhysicalDevice PhysicalDevice, DataTransferState* TransferState)
	{
		TransferState->CurrentFrame = 0;

		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = VulkanInterface::GetQueueGraphicsFamilyIndex();

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, nullptr, &TransferState->TransferCommandPool));

		VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
		TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		TransferCommandBufferAllocateInfo.commandPool = TransferState->TransferCommandPool;
		TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		TransferCommandBufferAllocateInfo.commandBufferCount = VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT;

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, TransferState->Frames.CommandBuffers));

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (u32 i = 0; i < VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, TransferState->Frames.Fences + i));
		}

		VkSemaphoreTypeCreateInfo TimelineCreateInfo = { };
		TimelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		TimelineCreateInfo.pNext = nullptr;
		TimelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		TimelineCreateInfo.initialValue = 0;

		VkSemaphoreCreateInfo SemaphoreInfo = { };
		SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		SemaphoreInfo.pNext = &TimelineCreateInfo;
		SemaphoreInfo.flags = 0;

		VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &TransferState->TransferSemaphore));
		TransferState->CompletedTransfer = 0;
		TransferState->TasksInFly = 0;

		TransferState->PendingTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024 * 100);
		TransferState->ActiveTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024 * 100);

		TransferState->TransferStagingPool = { };

		TransferState->TransferStagingPool.Buffer = VulkanHelper::CreateBuffer(Device, MB16, VulkanHelper::StagingFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			TransferState->TransferStagingPool.Buffer, VulkanHelper::HostCompatible);
		TransferState->TransferStagingPool.Memory = AllocResult.Memory;
		//TransferState->TransferStagingPool.ControlBlock.Alignment = AllocResult.Alignment;
		TransferState->TransferStagingPool.ControlBlock.Capacity = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, TransferState->TransferStagingPool.Buffer, TransferState->TransferStagingPool.Memory, 0));

		TransferState->TransferMemory = Memory::AllocateRingBuffer<u8>(MB16);
	}

	static void DeInitTransferState(VkDevice Device, DataTransferState* TransferState)
	{
		vkDestroyBuffer(Device, TransferState->TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, TransferState->TransferStagingPool.Memory, nullptr);

		vkDestroyCommandPool(Device, TransferState->TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, TransferState->Frames.Fences[i], nullptr);
		}

		vkDestroySemaphore(Device, TransferState->TransferSemaphore, nullptr);

		Memory::FreeRingBuffer(&TransferState->PendingTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState->ActiveTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState->TransferMemory);
	}

	static void Transfer(DataTransferState* TransferState, std::mutex* QueueSubmitMutex)
	{
		u64 FrameTotalTransferSize = 0;
		const u32 CurrentFrame = TransferState->CurrentFrame;
		VkDevice Device = VulkanInterface::GetDevice();

		VkFence TransferFence = TransferState->Frames.Fences[CurrentFrame];
		VkCommandBuffer TransferCommandBuffer = TransferState->Frames.CommandBuffers[CurrentFrame];

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));

		std::unique_lock<std::mutex> PendingLock(TransferState->PendingTransferTasksQueue.Mutex);
		TransferState->PendingCv.wait(PendingLock, [TransferState]
		{
			return !Memory::IsRingBufferEmpty(&TransferState->PendingTransferTasksQueue.TasksBuffer);
		});

		while (!Memory::IsRingBufferEmpty(&TransferState->PendingTransferTasksQueue.TasksBuffer))
		{
			TransferTask* Task = Memory::RingBufferGetFirst(&TransferState->PendingTransferTasksQueue.TasksBuffer);

			switch (Task->Type)
			{
				case TransferTaskType::TransferTaskType_Mesh:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 1);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkBufferMemoryBarrier2 Barrier = { };
					Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
					Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
					Barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
					Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.buffer = Task->DataDescr.DstBuffer;
					Barrier.offset = Task->DataDescr.DstOffset;
					Barrier.size = Task->DataSize;

					VkDependencyInfo DepInfo = { };
					DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					DepInfo.bufferMemoryBarrierCount = 1;
					DepInfo.pBufferMemoryBarriers = &Barrier;

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = TransferOffset;
					IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdCopyBuffer(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);

					break;
				}
				case TransferTaskType::TransferTaskType_Material:
				case TransferTaskType::TransferTaskType_Instance:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 1);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkBufferMemoryBarrier2 Barrier = { };
					Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
					Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					Barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
					Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.buffer = Task->DataDescr.DstBuffer;
					Barrier.offset = Task->DataDescr.DstOffset;
					Barrier.size = Task->DataSize;

					VkDependencyInfo DepInfo = { };
					DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					DepInfo.bufferMemoryBarrierCount = 1;
					DepInfo.pBufferMemoryBarriers = &Barrier;

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = TransferOffset;
					IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdCopyBuffer(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);

					break;
				}
				case TransferTaskType::TransferTaskType_Texture:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 16);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkImageMemoryBarrier2 TransferImageBarrier = { };
					TransferImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					TransferImageBarrier.pNext = nullptr;
					TransferImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
					TransferImageBarrier.srcAccessMask = 0;
					TransferImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					TransferImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					TransferImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					TransferImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					TransferImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					TransferImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					TransferImageBarrier.image = Task->TextureDescr.DstImage;
					TransferImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					TransferImageBarrier.subresourceRange.baseMipLevel = 0;
					TransferImageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					TransferImageBarrier.subresourceRange.baseArrayLayer = 0;
					TransferImageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

					VkDependencyInfo TransferDepInfo = { };
					TransferDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					TransferDepInfo.pNext = nullptr;
					TransferDepInfo.dependencyFlags = 0;
					TransferDepInfo.imageMemoryBarrierCount = 1;
					TransferDepInfo.pImageMemoryBarriers = &TransferImageBarrier;
					TransferDepInfo.memoryBarrierCount = 0;
					TransferDepInfo.pMemoryBarriers = nullptr;
					TransferDepInfo.bufferMemoryBarrierCount = 0;
					TransferDepInfo.pBufferMemoryBarriers = nullptr;

					VkBufferImageCopy ImageRegion = { };
					ImageRegion.bufferOffset = TransferOffset;
					ImageRegion.bufferRowLength = 0;
					ImageRegion.bufferImageHeight = 0;
					ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					ImageRegion.imageSubresource.mipLevel = 0;
					ImageRegion.imageSubresource.baseArrayLayer = 0;
					ImageRegion.imageSubresource.layerCount = 1;
					ImageRegion.imageOffset = { 0, 0, 0 };
					ImageRegion.imageExtent = { Task->TextureDescr.Width, Task->TextureDescr.Height, 1 };

					VkImageMemoryBarrier2 PresentationBarrier = { };
					PresentationBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					PresentationBarrier.pNext = nullptr;
					PresentationBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					PresentationBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					PresentationBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					PresentationBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
					PresentationBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					PresentationBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					PresentationBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					PresentationBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					PresentationBarrier.image = Task->TextureDescr.DstImage;
					PresentationBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					PresentationBarrier.subresourceRange.baseMipLevel = 0;
					PresentationBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					PresentationBarrier.subresourceRange.baseArrayLayer = 0;
					PresentationBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

					VkDependencyInfo PresentDepInfo = { };
					PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					PresentDepInfo.pNext = nullptr;
					PresentDepInfo.dependencyFlags = 0;
					PresentDepInfo.imageMemoryBarrierCount = 1;
					PresentDepInfo.pImageMemoryBarriers = &PresentationBarrier;
					PresentDepInfo.memoryBarrierCount = 0;
					PresentDepInfo.pMemoryBarriers = nullptr;
					PresentDepInfo.bufferMemoryBarrierCount = 0;
					PresentDepInfo.pBufferMemoryBarriers = nullptr;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &TransferDepInfo);
					vkCmdCopyBufferToImage(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer,
						Task->TextureDescr.DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

					break;
				}
			}

			std::unique_lock ActiveTasksLock(TransferState->ActiveTransferTasksQueue.Mutex);
			Memory::PushToRingBuffer(&TransferState->ActiveTransferTasksQueue.TasksBuffer, Task);
			++TransferState->TasksInFly;
			ActiveTasksLock.unlock();

			Memory::RingBufferPopFirst(&TransferState->PendingTransferTasksQueue.TasksBuffer);

			FrameTotalTransferSize += Task->DataSize;
			if (FrameTotalTransferSize >= TransferState->MaxTransferSizePerFrame)
			{
				break;
			}
		}

		PendingLock.unlock();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		const u64 TasksInFly = TransferState->TasksInFly;

		VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = { };
		TimelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		TimelineSubmitInfo.signalSemaphoreValueCount = 1;
		TimelineSubmitInfo.pSignalSemaphoreValues = &TasksInFly;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;
		SubmitInfo.pNext = &TimelineSubmitInfo;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &TransferState->TransferSemaphore;

		std::unique_lock SubmitLock(*QueueSubmitMutex);
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
		SubmitLock.unlock();

		TransferState->CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	static void ProcessTransferTasks(VkDevice Device, VkCommandBuffer CmdBuffer, DataTransferState* TransferState, const ResourceStorage* Storage)
	{
		u64 CompletedCounter = 0;
		vkGetSemaphoreCounterValue(Device, TransferState->TransferSemaphore, &CompletedCounter);

		u64 Counter = CompletedCounter - TransferState->CompletedTransfer;
		if (Counter > 0)
		{
			std::unique_lock Lock(TransferState->ActiveTransferTasksQueue.Mutex);

			while (Counter--)
			{
				TransferTask* Task = Memory::RingBufferGetFirst(&TransferState->ActiveTransferTasksQueue.TasksBuffer);

				switch (Task->Type)
				{
					case TransferTaskType::TransferTaskType_Mesh:
					{
						Storage->Meshes.StaticMeshes.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
					case TransferTaskType::TransferTaskType_Material:
					{
						Storage->Materials.Materials.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
					case TransferTaskType::TransferTaskType_Instance:
					{
						Storage->Meshes.MeshInstances.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
					case TransferTaskType::TransferTaskType_Texture:
					{
						Storage->Textures.Textures.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
				}

				Memory::RingFree(&TransferState->TransferMemory.ControlBlock, Task->DataSize, 1);
				Memory::RingBufferPopFirst(&TransferState->ActiveTransferTasksQueue.TasksBuffer);
			}

			TransferState->CompletedTransfer = CompletedCounter;
		}
	}

	static RenderState State;

	static void* RequestTransferMemory(u64 Size)
	{
		return State.TransferState.TransferMemory.DataArray + Memory::RingAlloc(&State.TransferState.TransferMemory.ControlBlock, Size, 1);
	}

	void Init(GLFWwindow* WindowHandler)
	{
		State.RenderResources.DrawEntities = Memory::AllocateArray<DrawEntity>(512);
		
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		InitDrawState(Device, VulkanInterface::GetQueueGraphicsFamilyIndex(), MAX_DRAW_FRAMES, &State.RenderDrawState);
		InitTransferState(Device, PhysicalDevice, &State.TransferState);
		InitTextureStorage(Device, PhysicalDevice, &State.RenderResources.Textures);
		InitMaterialStorage(Device, &State.RenderResources.Materials);
		InitStaticMeshStorage(Device, PhysicalDevice, &State.RenderResources.Meshes, MB4, MB4);

		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(Device, &State.RenderResources, &State.MeshPipeline);
		InitImGuiPipeline(&State.DebugUiPool, RenderResources::GetCoreContext(), WindowHandler);

		std::thread TransferThread(
			[]()
			{
				while (true) // todo use atomic or think about valid break
				{
					Transfer(&State.TransferState, &State.QueueSubmitMutex);
				}
			});

		TransferThread.detach();
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		DeInitImGuiPipeline(Device, State.DebugUiPool);
		DeInitTransferState(Device, &State.TransferState);
		DeInitTextureStorage(Device, &State.RenderResources.Textures);
		DeInitMaterialStorage(Device, &State.RenderResources.Materials);
		DeInitStaticMeshStorage(Device, &State.RenderResources.Meshes);
		DeInitDrawState(Device, MAX_DRAW_FRAMES, &State.RenderDrawState);

		//TerrainRender::DeInit();
		DeInitStaticMeshPipeline(Device, &State.MeshPipeline);

		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();

		Memory::FreeArray(&State.RenderResources.DrawEntities);
	}

	void Draw(const DrawScene* Scene)
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkDevice Device = VulkanInterface::GetDevice();
		const u32 CurrentFrame = State.RenderDrawState.CurrentFrame;
		u32 ImageIndex;
		VkFence FrameFence = State.RenderDrawState.Frames.Fences[CurrentFrame];
		VkSemaphore ImagesAvailable = State.RenderDrawState.Frames.ImagesAvailable[CurrentFrame];

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &FrameFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &FrameFence));
		VULKAN_CHECK_RESULT(vkAcquireNextImageKHR(Device, VulkanInterface::GetSwapchain(), UINT64_MAX, ImagesAvailable, nullptr, &ImageIndex));
		State.RenderDrawState.CurrentImageIndex = ImageIndex;

		FrameManager::UpdateViewProjection(&Scene->ViewProjection);

		VkCommandBuffer DrawCmdBuffer = State.RenderDrawState.Frames.CommandBuffers[ImageIndex];
		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(DrawCmdBuffer, &CommandBufferBeginInfo));

		ProcessTransferTasks(Device, DrawCmdBuffer, &State.TransferState, &State.RenderResources);

		LightningPass::Draw(Scene, &State.RenderResources);
		MainPass::BeginPass();
		//TerrainRender::Draw();
		DrawStaticMeshes(Device, DrawCmdBuffer, &State.RenderResources, &State.MeshPipeline, Scene);
		MainPass::EndPass();
		DeferredPass::BeginPass();
		DeferredPass::Draw();
		ImGui::Render();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CmdBuffer);
		DeferredPass::EndPass();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(DrawCmdBuffer));

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};

		VkSemaphore RenderFinished = State.RenderDrawState.Frames.RenderFinished[CurrentFrame];
		VkSwapchainKHR Swapchain = VulkanInterface::GetSwapchain();

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImagesAvailable;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &DrawCmdBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderFinished;

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished;
		PresentInfo.pSwapchains = &Swapchain;
		PresentInfo.pImageIndices = &ImageIndex;

		std::unique_lock Lock(State.QueueSubmitMutex);
		VULKAN_CHECK_RESULT(vkQueueSubmit(VulkanInterface::GetGraphicsQueue(), 1, &SubmitInfo, FrameFence));
		VULKAN_CHECK_RESULT(vkQueuePresentKHR(VulkanInterface::GetGraphicsQueue(), &PresentInfo));
		Lock.unlock();

		State.RenderDrawState.CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	void NotifyTransfer()
	{
		State.TransferState.PendingCv.notify_one();
	}

	u32 CreateEntity(const DrawEntity* Entity)
	{
		Memory::PushBackToArray(&State.RenderResources.DrawEntities, Entity);
		return 0;
	}

	u32 CreateMaterial(Material* Mat)
	{
		const u64 Index = State.RenderResources.Materials.Materials.Count;

		RenderResource<Material>* NewMaterialResource = Memory::ArrayGetNew(&State.RenderResources.Materials.Materials);
		NewMaterialResource->IsLoaded = false;
		NewMaterialResource->Resource = *Mat;

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(sizeof(Material));
		memcpy(TransferMemory, &NewMaterialResource->Resource, sizeof(Material));

		TransferTask Task = { };
		Task.DataSize = sizeof(Material);
		Task.DataDescr.DstBuffer = State.RenderResources.Materials.MaterialBuffer;
		Task.DataDescr.DstOffset = sizeof(Material) * Index;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Material;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		return Index;
	}

	u64 CreateStaticMesh(void* MeshVertexData, u64 VertexSize, u64 VerticesCount, u64 IndicesCount)
	{
		const u64 VerticesSize = VertexSize * VerticesCount;
		const u64 DataSize = sizeof(u32) * IndicesCount + VerticesSize;

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(DataSize);
		memcpy(TransferMemory, MeshVertexData, DataSize);

		const u64 Index = State.RenderResources.Meshes.StaticMeshes.Count;
		RenderResource<StaticMesh>* Resource = Memory::ArrayGetNew(&State.RenderResources.Meshes.StaticMeshes);
		StaticMesh* Mesh = &Resource->Resource;
		*Mesh = { };
		Mesh->IndicesCount = IndicesCount;
		Mesh->VertexOffset = State.RenderResources.Meshes.VertexStageData.Offset;
		Mesh->IndexOffset = State.RenderResources.Meshes.VertexStageData.Offset + VerticesSize;
		Mesh->VertexDataSize = DataSize;

		TransferTask Task = { };
		Task.DataSize = DataSize;
		Task.DataDescr.DstBuffer = State.RenderResources.Meshes.VertexStageData.Buffer;
		Task.DataDescr.DstOffset = State.RenderResources.Meshes.VertexStageData.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Mesh;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		State.RenderResources.Meshes.VertexStageData.Offset += DataSize;
		return Index;
	}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height)
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();

		const u64 Index = State.RenderResources.Textures.Textures.Count;
		MeshTexture2D* NextTexture = Memory::ArrayGetNew(&State.RenderResources.Textures.Textures);
		*NextTexture = { };

		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Width;
		ImageCreateInfo.extent.height = Height;
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
			NextTexture->MeshTexture.Image, VulkanHelper::GPULocal);
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
		DiffuseImageInfo.sampler = State.RenderResources.Textures.DiffuseSampler;
		DiffuseImageInfo.imageView = NextTexture->View;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = State.RenderResources.Textures.SpecularSampler;
		SpecularImageInfo.imageView = NextTexture->View;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = State.RenderResources.Textures.BindlesTexturesSet;
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = Index;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = State.RenderResources.Textures.BindlesTexturesSet;
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = Index;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 2, Writes, 0, nullptr);

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(NextTexture->MeshTexture.Size);
		memcpy(TransferMemory, Data, NextTexture->MeshTexture.Size);

		TransferTask Task = { };
		Task.DataSize = NextTexture->MeshTexture.Size;
		Task.TextureDescr.DstImage = NextTexture->MeshTexture.Image;
		Task.TextureDescr.Width = Width;
		Task.TextureDescr.Height = Height;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Texture;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		return Index;
	}

	u32 CreateStaticMeshInstance(InstanceData* Data)
	{
		const u64 Index = State.RenderResources.Meshes.MeshInstances.Count;

		RenderResource<InstanceData>* NewInstanceResource = Memory::ArrayGetNew(&State.RenderResources.Meshes.MeshInstances);
		NewInstanceResource->IsLoaded = false;
		NewInstanceResource->Resource = *Data;

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(sizeof(InstanceData));
		memcpy(TransferMemory, &NewInstanceResource->Resource, sizeof(InstanceData));

		TransferTask Task = { };
		Task.DataSize = sizeof(InstanceData);
		Task.DataDescr.DstBuffer = State.RenderResources.Meshes.GPUInstances.Buffer;
		Task.DataDescr.DstOffset = sizeof(InstanceData) * Index;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Instance;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		return Index;
	}

	RenderState* GetRenderState()
	{
		return &State;
	}

	bool IsDrawEntityLoaded(const ResourceStorage* Storage, const DrawEntity* Entity)
	{
		RenderResource<StaticMesh>* MeshResource = Storage->Meshes.StaticMeshes.Data + Entity->StaticMeshIndex;
		if (!MeshResource->IsLoaded) return false;

		RenderResource<InstanceData>* InstanceResource = Storage->Meshes.MeshInstances.Data + Entity->InstanceDataIndex;
		for (u32 i = Entity->InstanceDataIndex; i < Entity->Instances; ++i)
		{
			InstanceResource = Storage->Meshes.MeshInstances.Data + i;
			if (!InstanceResource->IsLoaded) return false;
		}

		RenderResource<Material>* MaterialResource = Storage->Materials.Materials.Data + InstanceResource->Resource.MaterialIndex;
		if (!MaterialResource->IsLoaded) return false;
		Material* Material = &MaterialResource->Resource;
		MeshTexture2D* AlbedoTexture = Storage->Textures.Textures.Data + Material->AlbedoTexIndex;
		if (!AlbedoTexture->IsLoaded) return false;
		MeshTexture2D* SpecTexture = Storage->Textures.Textures.Data + Material->SpecularTexIndex;
		if (!SpecTexture->IsLoaded) return false;
		return true;
	}
}
#include "BMRInterface.h"

#include <Windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "Memory/MemoryManagmentSystem.h"
#include "VulkanMemoryManagementSystem.h"

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"
#include "VulkanHelper.h"

#include "Util/Settings.h"



#include "BMRVulkan/BMRVulkan.h"

namespace BMR
{
	static void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection);
	static void UpdateLightBuffer(const BMRLightBuffer* Buffer);
	static void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix);



	static BMRConfig Config;

	static BMRMainRenderPass MainRenderPass;



	static u32 CurrentFrame = 0;



	

	static BMRUniform VertexBuffer;
	static u32 VertexBufferOffset = 0;
	static BMRUniform IndexBuffer;
	static u32 IndexBufferOffset = 0;

	



	extern BMRDevice Device;
	extern VkExtent2D SwapExtent;
	extern VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	extern VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	extern VkFence DrawFences[MAX_DRAW_FRAMES];
	extern VkQueue GraphicsQueue;
	extern VkQueue PresentationQueue;
	extern VkCommandPool GraphicsCommandPool;
	extern BMRSwapchain SwapInstance;
	extern VkCommandBuffer DrawCommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];


	// TODO!!!!!!!!!!!!!!!!!!!!!!!!!
	BMRRenderPass MainPass;
	BMRRenderPass DepthPass;
	BMR::BMRUniform* VpBuffer;
	VkDescriptorSetLayout VpLayout;
	VkDescriptorSet* VpSet;
	BMRUniform* EntityLight;
	VkDescriptorSetLayout EntityLightLayout;
	VkDescriptorSet* EntityLightSet;
	BMRUniform* LightSpaceBuffer;
	VkDescriptorSetLayout LightSpaceLayout;
	VkDescriptorSet* LightSpaceSet;
	BMRUniform Material;
	VkDescriptorSetLayout materialLayout;
	VkDescriptorSet MaterialSet;
	VkImageView* DeferredInputDepthImage;
	VkImageView* DeferredInputColorImage;
	VkDescriptorSetLayout DeferredInputLayout;
	VkDescriptorSet* DeferredInputSet;
	BMRUniform* ShadowArray;
	VkDescriptorSetLayout ShadowArrayLayout;
	VkDescriptorSet* ShadowArraySet;
	VkDescriptorSetLayout EntitySamplerLayout;

	BMRPipelineShaderInputDepr ShaderInputs[BMR::BMRShaderNames::ShaderNamesCount];
	











	bool Init(HWND WindowHandler, const BMRConfig& InConfig)
	{
		Config = InConfig;

		SetLogHandler(Config.LogHandler);

		BMRVkConfig BMRVkConfig;
		BMRVkConfig.EnableValidationLayers = Config.EnableValidationLayers;
		BMRVkConfig.LogHandler = (BMRVkLogHandler)Config.LogHandler;
		BMRVkConfig.MaxTextures = Config.MaxTextures;

		BMRVkInit(WindowHandler, BMRVkConfig);

		VkBufferCreateInfo BufferInfo = {};
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = MB64;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VertexBuffer = CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		IndexBuffer = CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		
		for (u32 i = 0; i < BMR::BMRShaderNames::ShaderNamesCount; ++i)
		{
			ShaderInputs[i].Code = Config.RenderShaders[i].Code;
			ShaderInputs[i].CodeSize = Config.RenderShaders[i].CodeSize;
			ShaderInputs[i].EntryPoint = "main";
			ShaderInputs[i].Handle = Config.RenderShaders[i].Handle;
			ShaderInputs[i].Stage = Config.RenderShaders[i].Stage;
		}

		return true;
	}

	void DeInit()
	{
		vkDeviceWaitIdle(Device.LogicalDevice);
		DestroyUniformBuffer(VertexBuffer);
		DestroyUniformBuffer(IndexBuffer);
		BMRVkDeInit();
	}

	void TestSetRendeRpasses(BMRRenderPass Main, BMRRenderPass Depth,
		BMRUniform* inVpBuffer, VkDescriptorSetLayout iVpLayout, VkDescriptorSet* iVpSet,
		BMRUniform* inEntityLight, VkDescriptorSetLayout iEntityLightLayout, VkDescriptorSet* iEntityLightSet,
		BMRUniform* iLightSpaceBuffer, VkDescriptorSetLayout iLightSpaceLayout, VkDescriptorSet* iLightSpaceSet,
		BMRUniform iMaterial, VkDescriptorSetLayout iMaterialLayout, VkDescriptorSet iMaterialSet,
		VkImageView* iDeferredInputDepthImage, VkImageView* iDeferredInputColorImage,
		VkDescriptorSetLayout iDeferredInputLayout, VkDescriptorSet* iDeferredInputSet,
		BMRUniform* iShadowArray, VkDescriptorSetLayout iShadowArrayLayout, VkDescriptorSet* iShadowArraySet,
		VkDescriptorSetLayout iEntitySamplerLayout, VkDescriptorSetLayout iTerrainSkyBoxSamplerLayout)
	{
		MainPass = Main;
		DepthPass = Depth;
		VpBuffer = inVpBuffer;
		VpLayout = iVpLayout;
		VpSet = iVpSet;
		EntityLight = inEntityLight;
		EntityLightLayout = iEntityLightLayout;
		EntityLightSet = iEntityLightSet;
		LightSpaceBuffer = iLightSpaceBuffer;
		LightSpaceLayout = iLightSpaceLayout;
		LightSpaceSet = iLightSpaceSet;
		Material = iMaterial;
		materialLayout = iMaterialLayout;
		MaterialSet = iMaterialSet;
		DeferredInputDepthImage = iDeferredInputDepthImage;
		DeferredInputColorImage = iDeferredInputColorImage;
		DeferredInputLayout = iDeferredInputLayout;
		DeferredInputSet = iDeferredInputSet;
		ShadowArray = iShadowArray;
		ShadowArrayLayout = iShadowArrayLayout;
		ShadowArraySet = iShadowArraySet;
		EntitySamplerLayout = iEntitySamplerLayout;

		MainRenderPass.SetupPushConstants();
		MainRenderPass.CreatePipelineLayouts(Device.LogicalDevice, VpLayout, EntityLightLayout, LightSpaceLayout, materialLayout,
			DeferredInputLayout, ShadowArrayLayout, EntitySamplerLayout, iTerrainSkyBoxSamplerLayout);
		MainRenderPass.CreatePipelinesDepr(Device.LogicalDevice, SwapExtent, ShaderInputs, MainPass.Pass, DepthPass.Pass);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount)
	{
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = VertexBufferOffset;
		VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = IndexBufferOffset;
		IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void UpdateLightBuffer(const BMRLightBuffer* Buffer)
	{
		assert(Buffer);

		const u32 UpdateIndex = (MainRenderPass.ActiveLightSet + 1) % SwapInstance.ImagesCount;

		UpdateUniformBuffer(EntityLight[UpdateIndex], sizeof(BMRLightBuffer), 0,
			Buffer);

		MainRenderPass.ActiveLightSet = UpdateIndex;
	}

	void UpdateMaterialBuffer(const BMRMaterial* Buffer)
	{
		assert(Buffer);
		UpdateUniformBuffer(Material, sizeof(BMRMaterial), 0,
			Buffer);
	}

	void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveLightSpaceMatrixSet + 1) % SwapInstance.ImagesCount;

		UpdateUniformBuffer(LightSpaceBuffer[UpdateIndex], sizeof(BMRLightSpaceMatrix), 0,
			LightSpaceMatrix);

		MainRenderPass.ActiveLightSpaceMatrixSet = UpdateIndex;
	}

	void Draw(const BMRDrawScene& Scene)
	{
		// Todo Update only when changed
		UpdateLightBuffer(Scene.LightEntity);
		UpdateVpBuffer(Scene.ViewProjection);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const u32 MainPassClearValuesSize = 3;
		VkClearValue MainPassClearValues[MainPassClearValuesSize];
		// Do not forget about position in array AttachmentDescriptions
		MainPassClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MainPassClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MainPassClearValues[2].depthStencil.depth = 1.0f;

		const u32 DepthPassClearValuesSize = 1;
		VkClearValue DepthPassClearValues[DepthPassClearValuesSize];
		// Do not forget about position in array AttachmentDescriptions
		DepthPassClearValues[0].depthStencil.depth = 1.0f;

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		VkFence Fence = DrawFences[CurrentFrame];
		VkSemaphore ImageAvailable = ImagesAvailable[CurrentFrame];

		vkWaitForFences(Device.LogicalDevice, 1, &Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device.LogicalDevice, 1, &Fence);

		u32 ImageIndex;
		vkAcquireNextImageKHR(Device.LogicalDevice, SwapInstance.VulkanSwapchain, UINT64_MAX,
			ImageAvailable, VK_NULL_HANDLE, &ImageIndex);

		VkCommandBuffer CommandBuffer = DrawCommandBuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		{
			const BMRLightSpaceMatrix* LightViews[] =
			{
				&Scene.LightEntity->DirectionLight.LightSpaceMatrix,
				&Scene.LightEntity->SpotLight.LightSpaceMatrix,
			};

			for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
			{
				UpdateLightSpaceBuffer(LightViews[LightCaster]);

				VkRenderPassBeginInfo DepthRenderPassBeginInfo = { };
				DepthRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				DepthRenderPassBeginInfo.renderPass = DepthPass.Pass;
				DepthRenderPassBeginInfo.renderArea.offset = { 0, 0 };
				DepthRenderPassBeginInfo.pClearValues = DepthPassClearValues;
				DepthRenderPassBeginInfo.clearValueCount = DepthPassClearValuesSize;
				DepthRenderPassBeginInfo.renderArea.extent = DepthViewportExtent; // Size of region to run render pass on (starting at offset)
				DepthRenderPassBeginInfo.framebuffer = DepthPass.FramebufferSets[LightCaster].FrameBuffers[ImageIndex];
				
				vkCmdBeginRenderPass(CommandBuffer, &DepthRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
				vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Depth].Pipeline);

				for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
				{
					BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

					const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
					const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

					const u32 DescriptorSetGroupCount = 1;
					const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
					{
						LightSpaceSet[MainRenderPass.ActiveLightSpaceMatrixSet],
					};

					const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::Depth].PipelineLayout;

					vkCmdPushConstants(CommandBuffer, PipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

					vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
						0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

					vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
					vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
				}

				vkCmdEndRenderPass(CommandBuffer);
			}
		}

		VkRenderPassBeginInfo MainRenderPassBeginInfo = { };
		MainRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		MainRenderPassBeginInfo.renderPass = MainPass.Pass;
		MainRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		MainRenderPassBeginInfo.pClearValues = MainPassClearValues;
		MainRenderPassBeginInfo.clearValueCount = MainPassClearValuesSize;
		MainRenderPassBeginInfo.renderArea.extent = SwapInstance.SwapExtent; // Size of region to run render pass on (starting at offset)
		MainRenderPassBeginInfo.framebuffer = MainPass.FramebufferSets[0].FrameBuffers[ImageIndex];

		vkCmdBeginRenderPass(CommandBuffer, &MainRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Terrain].Pipeline);

			for (u32 i = 0; i < Scene.DrawTerrainEntitiesCount; ++i)
			{
				BMRDrawTerrainEntity* DrawTerrainEntity = Scene.DrawTerrainEntities + i;

				const VkBuffer TerrainVertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize TerrainBuffersOffsets[] = { DrawTerrainEntity->VertexOffset };

				const u32 TerrainDescriptorSetGroupCount = 2;
				const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
					VpSet[MainRenderPass.ActiveVpSet],
					DrawTerrainEntity->TextureSet
				};

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Terrain].PipelineLayout,
					0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawTerrainEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawTerrainEntity->IndicesCount, 1, 0, 0, 0);
			}
		}

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Entity].Pipeline);

			// TODO: Support rework to not create identical index buffers
			for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

				const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const VkDescriptorSet DescriptorSetGroup[] =
				{
					VpSet[MainRenderPass.ActiveVpSet],
					DrawEntity->TextureSet,
					EntityLightSet[MainRenderPass.ActiveLightSet],
					MaterialSet,
					ShadowArraySet[ImageIndex],
				};
				const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

				const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::Entity].PipelineLayout;

				vkCmdPushConstants(CommandBuffer, PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
			}
		}

		{
			if (Scene.DrawSkyBox)
			{
				vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::SkyBox].Pipeline);

				const u32 SkyBoxDescriptorSetGroupCount = 2;
				const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
					VpSet[MainRenderPass.ActiveVpSet],
					Scene.SkyBox.TextureSet,
				};

				const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::SkyBox].PipelineLayout;

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Scene.SkyBox.VertexOffset);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, Scene.SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, Scene.SkyBox.IndicesCount, 1, 0, 0, 0);
			}
		}

		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Deferred].Pipeline);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Deferred].PipelineLayout,
				0, 1, &DeferredInputSet[ImageIndex], 0, nullptr);

			vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		}

		vkCmdEndRenderPass(CommandBuffer);

		Result = vkEndCommandBuffer(CommandBuffer);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImageAvailable;
		SubmitInfo.pCommandBuffers = &CommandBuffer; // Command buffer to submit
		SubmitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame]; // Semaphores to signal when command buffer finishes
		Result = vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, Fence);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkQueueSubmit result is %d", Result);
			assert(false);
		}

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];
		PresentInfo.pSwapchains = &SwapInstance.VulkanSwapchain; // Swapchains to present images to
		PresentInfo.pImageIndices = &ImageIndex; // Index of images in swapchains to present
		Result = vkQueuePresentKHR(PresentationQueue, &PresentInfo);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkQueuePresentKHR result is %d", Result);
		}

		CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveVpSet + 1) % SwapInstance.ImagesCount;

		UpdateUniformBuffer(VpBuffer[UpdateIndex], sizeof(BMRUboViewProjection), 0,
			&ViewProjection);

		MainRenderPass.ActiveVpSet = UpdateIndex;
	}
}

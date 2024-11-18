#include "MainRenderPass.h"

#include "VulkanHelper.h"
#include "VulkanMemoryManagementSystem.h"

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"


#include "BMRInterface.h"

namespace BMR
{
	namespace SubpassIndex
	{
		enum
		{
			MainSubpass = 0,
			DeferredSubpas, // Fullscreen effects

			Count
		};
	}

	static const u32 TmpDepthSubpassIndex = 0;

	void BMRMainRenderPass::SetupPushConstants()
	{
		PushConstants[PushConstantHandles::Model].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants[PushConstantHandles::Model].offset = 0;
		// Todo: check constant and model size?
		PushConstants[PushConstantHandles::Model].size = sizeof(BMRModel);
	}

	void BMRMainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice, VkDescriptorSetLayout VpLayout,
		VkDescriptorSetLayout EntityLightLayout, VkDescriptorSetLayout LightSpaceMatrixLayout, VkDescriptorSetLayout Material,
		VkDescriptorSetLayout Deferred, VkDescriptorSetLayout ShadowArray, VkDescriptorSetLayout EntitySampler,
		VkDescriptorSetLayout TerrainSkyBoxSampler)
	{
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] = {
			VpLayout,
			TerrainSkyBoxSampler
		};

		const u32 EntityDescriptorLayoutCount = 5;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] = {
			VpLayout,
			EntitySampler,
			EntityLightLayout,
			Material,
			ShadowArray
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] = {
			VpLayout,
			TerrainSkyBoxSampler,
		};

		u32 SetLayoutCountTable[BMRPipelineHandles::PipelineHandlesCount];
		SetLayoutCountTable[BMRPipelineHandles::Entity] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Terrain] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[BMRPipelineHandles::Deferred] = 1;
		SetLayoutCountTable[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Depth] = 1;

		const VkDescriptorSetLayout* SetLayouts[BMRPipelineHandles::PipelineHandlesCount];
		SetLayouts[BMRPipelineHandles::Entity] = EntityDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Terrain] = TerrainDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Deferred] = &Deferred;
		SetLayouts[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Depth] = &LightSpaceMatrixLayout;

		u32 PushConstantRangeCountTable[BMRPipelineHandles::PipelineHandlesCount];
		PushConstantRangeCountTable[BMRPipelineHandles::Entity] = 1;
		PushConstantRangeCountTable[BMRPipelineHandles::Terrain] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Deferred] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::SkyBox] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Depth] = 1;


		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			Pipelines[i].PipelineLayout = CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PushConstants[PushConstantHandles::Model], PushConstantRangeCountTable[i]);
		}
	}

	void BMRMainRenderPass::CreatePipelinesDepr(VkDevice LogicalDevice, VkExtent2D SwapExtent,
		BMRPipelineShaderInputDepr ShaderInputs[BMRShaderNames::ShaderNamesCount], VkRenderPass main, VkRenderPass depth)
	{
		const VkShaderStageFlagBits NamesToStagesTable[] =
		{
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		BMRSPipelineShaderInfo ShaderInfos[BMRPipelineHandles::PipelineHandlesCount];
		for (u32 i = 0; i < BMRShaderNames::ShaderNamesCount; ++i)
		{
			const BMRPipelineShaderInputDepr& Input = ShaderInputs[i];
			BMRSPipelineShaderInfo& Info = ShaderInfos[Input.Handle];

			Info.Infos[Info.InfosCounter] = { };
			Info.Infos[Info.InfosCounter].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Info.Infos[Info.InfosCounter].stage = NamesToStagesTable[Input.Stage];
			Info.Infos[Info.InfosCounter].pName = Input.EntryPoint;
			CreateShader(LogicalDevice, Input.Code, Input.CodeSize, Info.Infos[Info.InfosCounter].module);
			++Info.InfosCounter;
		}

		BMRVertexInput VertexInput[BMRPipelineHandles::PipelineHandlesCount];
		VertexInput[BMRPipelineHandles::Entity] = EntityVertexInput;
		VertexInput[BMRPipelineHandles::Terrain] = TerrainVertexInput;
		VertexInput[BMRPipelineHandles::Deferred] = { };
		VertexInput[BMRPipelineHandles::SkyBox] = SkyBoxVertexInput;
		VertexInput[BMRPipelineHandles::Depth] = DepthVertexInput;

		BMRPipelineSettings PipelineSettings[BMRPipelineHandles::PipelineHandlesCount];
		PipelineSettings[BMRPipelineHandles::Entity] = EntityPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Terrain] = TerrainPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Deferred] = DeferredPipelineSettings;
		PipelineSettings[BMRPipelineHandles::SkyBox] = SkyBoxPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Depth] = DepthPipelineSettings;


		BMRPipelineResourceInfo PipelineResourceInfo[BMRPipelineHandles::PipelineHandlesCount];
		PipelineResourceInfo[BMRPipelineHandles::Entity].PipelineLayout = Pipelines[BMRPipelineHandles::Entity].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Entity].RenderPass = main;
		PipelineResourceInfo[BMRPipelineHandles::Entity].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Terrain].PipelineLayout = Pipelines[BMRPipelineHandles::Terrain].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Terrain].RenderPass = main;
		PipelineResourceInfo[BMRPipelineHandles::Terrain].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Deferred].PipelineLayout = Pipelines[BMRPipelineHandles::Deferred].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Deferred].RenderPass = main;
		PipelineResourceInfo[BMRPipelineHandles::Deferred].SubpassIndex = SubpassIndex::DeferredSubpas;

		PipelineResourceInfo[BMRPipelineHandles::SkyBox].PipelineLayout = Pipelines[BMRPipelineHandles::SkyBox].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::SkyBox].RenderPass = main;
		PipelineResourceInfo[BMRPipelineHandles::SkyBox].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Depth].PipelineLayout = Pipelines[BMRPipelineHandles::Depth].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Depth].RenderPass = depth;
		PipelineResourceInfo[BMRPipelineHandles::Depth].SubpassIndex = TmpDepthSubpassIndex;

		VkPipeline NewPipelines[BMRPipelineHandles::PipelineHandlesCount];
		CreatePipelines(ShaderInfos, VertexInput, PipelineSettings,
			PipelineResourceInfo, BMRPipelineHandles::PipelineHandlesCount, NewPipelines);

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			Pipelines[i].Pipeline = NewPipelines[i];
		}
	}
}

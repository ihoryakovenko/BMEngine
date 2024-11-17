#include "MainRenderPass.h"

#include "VulkanHelper.h"
#include "VulkanMemoryManagementSystem.h"
#include "VulkanResourceManagementSystem.h"

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

	void BMRMainRenderPass::ClearResources(VkDevice LogicalDevice, u32 ImagesCount)
	{
		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorLayouts[i], nullptr);
		}
	}

	void BMRMainRenderPass::SetupPushConstants()
	{
		PushConstants[PushConstantHandles::Model].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants[PushConstantHandles::Model].offset = 0;
		// Todo: check constant and model size?
		PushConstants[PushConstantHandles::Model].size = sizeof(BMRModel);
	}

	void BMRMainRenderPass::CreateDescriptorLayouts(VkDevice LogicalDevice)
	{
		const u32 EntitySamplerLayoutBindingCount = 2;
		VkDescriptorSetLayoutBinding EntitySamplerLayoutBinding[EntitySamplerLayoutBindingCount];
		EntitySamplerLayoutBinding[0].binding = 0;
		EntitySamplerLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		EntitySamplerLayoutBinding[0].descriptorCount = 1;
		EntitySamplerLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		EntitySamplerLayoutBinding[0].pImmutableSamplers = nullptr;
		EntitySamplerLayoutBinding[1] = EntitySamplerLayoutBinding[0];
		EntitySamplerLayoutBinding[1].binding = 1;

		VkDescriptorSetLayoutBinding TerrainSamplerLayoutBinding = { };
		TerrainSamplerLayoutBinding.binding = 0;
		TerrainSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		TerrainSamplerLayoutBinding.descriptorCount = 1;
		TerrainSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainSamplerLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding SkyBoxSamplerLayoutBinding = { };
		SkyBoxSamplerLayoutBinding.binding = 0;
		SkyBoxSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SkyBoxSamplerLayoutBinding.descriptorCount = 1;
		SkyBoxSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SkyBoxSamplerLayoutBinding.pImmutableSamplers = nullptr;

		u32 BindingCountTable[DescriptorLayoutHandles::Count];
		BindingCountTable[DescriptorLayoutHandles::TerrainSampler] = 1;
		BindingCountTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBindingCount;
		BindingCountTable[DescriptorLayoutHandles::SkyBoxSampler] = 1;

		const VkDescriptorSetLayoutBinding* BindingsTable[DescriptorLayoutHandles::Count];
		BindingsTable[DescriptorLayoutHandles::TerrainSampler] = &TerrainSamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::SkyBoxSampler] = &SkyBoxSamplerLayoutBinding;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfos[DescriptorLayoutHandles::Count];
		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			LayoutCreateInfos[i] = { };
			LayoutCreateInfos[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfos[i].bindingCount = BindingCountTable[i];
			LayoutCreateInfos[i].pBindings = BindingsTable[i];

			const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &(LayoutCreateInfos[i]), nullptr, &(DescriptorLayouts[i]));
			if (Result != VK_SUCCESS)
			{
				HandleLog(BMRLogType::LogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
			}
		}
	}

	void BMRMainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice, BMRUniformLayout VpLayout,
		BMRUniformLayout EntityLightLayout, BMRUniformLayout LightSpaceMatrixLayout, BMRUniformLayout Material,
		BMRUniformLayout Deferred, BMRUniformLayout ShadowArray)
	{
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] = {
			VpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::TerrainSampler]
		};

		const u32 EntityDescriptorLayoutCount = 5;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] = {
			VpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::EntitySampler],
			EntityLightLayout,
			Material,
			ShadowArray
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] = {
			VpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::SkyBoxSampler],
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
			Pipelines[i].PipelineLayout = VulkanResourceManagementSystem::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PushConstants[PushConstantHandles::Model], PushConstantRangeCountTable[i]);
		}
	}

	void BMRMainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
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


		VkPipeline* NewPipelines = VulkanResourceManagementSystem::CreatePipelines(ShaderInfos, VertexInput, PipelineSettings,
			PipelineResourceInfo, BMRPipelineHandles::PipelineHandlesCount);

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			Pipelines[i].Pipeline = NewPipelines[i];
		}
	}
}

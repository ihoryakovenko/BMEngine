#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

#include <mini-yaml/yaml/Yaml.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "Render.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

namespace RenderResources
{
	template<typename T>
	struct RenderResource
	{
		T Resource;
		u32 ResourceIndex;
	};

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertices(Yaml::Node& VerticesNode);
	void CreateShaders(Yaml::Node& ShadersNode);
	void CreateSamplers(Yaml::Node& SamplersNode);
	void CreateDescriptorLayouts(Yaml::Node& DescriptorSetLayoutsNode);

	void PostCreateInit();

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);

	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);

	VkPipeline CreateGraphicsPipeline(VkDevice Device, Yaml::Node& Root,
		VkExtent2D Extent, VkPipelineLayout PipelineLayout, const VulkanHelper::PipelineResourceInfo* ResourceInfo);

	u32 CreateStaticMesh(void* MeshVertexData, u64 VertexSize, u64 VerticesCount, u64 IndicesCount);
	u32 CreateMaterial(Render::Material* Mat);
	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height);
	u32 CreateStaticMeshInstance(Render::InstanceData* Data);

	Render::StaticMesh* GetStaticMesh(u32 Index);
	Render::InstanceData* GetInstanceData(u32 Index);

	void SetResourceReadyToRender(u32 ResourceIndex);
	VkDescriptorSetLayout GetBindlesTexturesLayout();
	VkDescriptorSetLayout GetMaterialLayout();
	VkDescriptorSet GetBindlesTexturesSet();
	VkDescriptorSet GetMaterialSet();
	VkBuffer GetVertexStageBuffer();
	VkBuffer GetInstanceBuffer();
	VkDescriptorPool GetMainPool();

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity);
}
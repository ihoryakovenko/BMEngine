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
	enum class ResourceType
	{
		Invalid,

		Texture,
		Mesh,
		Material,
		Instance,
	};

	struct VertexData
	{
		u64 VertexOffset;
		u32 IndexOffset;
		u32 IndicesCount;
		u64 VertexDataSize;
	};

	struct Texture
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size;
		u64 Alignment;
		u32 Width;
		u32 Height;
	};

	struct MeshTexture2D
	{
		Texture MeshTexture;
		VkImageView View;
	};

	struct Material
	{
		u32 AlbedoTexIndex;
		u32 SpecularTexIndex;
		f32 Shininess;
	};

	struct InstanceData
	{
		glm::mat4 ModelMatrix;
		u32 MaterialIndex;
	};

	struct MeshDescription
	{
		u64 VertexSize;
		u64 VerticesCount;
		u64 IndicesCount;
	};

	struct TextureDescription
	{
		u32 Width;
		u32 Height;
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

	u32 CreateStaticMesh(MeshDescription* Description, void* Data);
	u32 CreateMaterial(Material* Mat);
	u32 CreateTexture2DSRGB(TextureDescription* Description, void* Data);
	u32 CreateStaticMeshInstance(InstanceData* Data);

	VertexData* GetStaticMesh(u32 Index);
	InstanceData* GetInstanceData(u32 Index);
	MeshTexture2D* GetTexture(u32 Index);

	void SetResourceReadyToRender(u32 ResourceIndex, ResourceType Type);
	VkDescriptorSetLayout GetBindlesTexturesLayout();
	VkDescriptorSetLayout GetMaterialLayout();
	VkDescriptorSet GetBindlesTexturesSet();
	VkDescriptorSet GetMaterialSet();
	VkBuffer GetVertexStageBuffer();
	VkBuffer GetInstanceBuffer();
	VkDescriptorPool GetMainPool();

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity);
}
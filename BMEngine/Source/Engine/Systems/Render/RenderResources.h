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

	struct ResourceDependency
	{
		ResourceType Type;
		u32 ResourceIndex;
	};

	struct ResourceRecord
	{
		std::atomic<bool> IsLoaded;
		u64 GPUBufferOffset;
		Memory::DynamicHeapArray<ResourceDependency> Dependencies;
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
		VkFormat Format;
	};

	struct SamplerDescription
	{
		VkFilter MagFilter;
		VkFilter MinFilter;
		VkSamplerMipmapMode MipmapMode;
		VkSamplerAddressMode AddressModeU;
		VkSamplerAddressMode AddressModeV;
		VkSamplerAddressMode AddressModeW;
		f32 MipLodBias;
		VkBool32 AnisotropyEnable;
		f32 MaxAnisotropy;
		VkBool32 CompareEnable;
		VkCompareOp CompareOp;
		f32 MinLod;
		f32 MaxLod;
		VkBorderColor BorderColor;
		VkBool32 UnnormalizedCoordinates;
	};

	struct BufferDescription
	{
		VulkanHelper::BufferUsageFlag BufferUsageFlag;
		VulkanHelper::MemoryPropertyFlag MemoryPropertyFlag;
		u64 Size;
	};

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding);
	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize);
	void CreateSampler(const std::string& Name, const SamplerDescription& Data);
	void CreateGPUBuffer(const std::string& Name, const BufferDescription& Description);
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
	
	u32 CreateTexture(TextureDescription* Description, void* Data);
	
	u32 CreateStaticMeshInstance(u32 DataSize, const std::string& BufferName);
	u32 CreateMaterial(u32 DataSize, const std::string& BufferName);

	void AddResourceDependencyToMaterial(u32 Handle, ResourceDependency Dependency);
	void AddResourceDependencyToInstance(u32 Handle, ResourceDependency Dependency);

	void UpdateMaterial(u32 Handle, void* Data, u32 DataSize, const std::string& BufferName);
	void UpdateInstance(u32 Handle, void* Data, u32 DataSize, const std::string& BufferName);

	VertexData* GetStaticMesh(u32 Index);
	ResourceRecord* GetInstanceData(u32 Index);
	MeshTexture2D* GetTexture(u32 Index);

	void SetResourceReadyToRender(u32 ResourceIndex, ResourceType Type);
	VkDescriptorSetLayout GetBindlesTexturesLayout();
	VkDescriptorSetLayout GetMaterialLayout();
	VkDescriptorSet GetBindlesTexturesSet();
	VkDescriptorSet GetMaterialSet();
	VkBuffer GetVertexStageBuffer();
	VkBuffer GetInstanceBuffer();
	VkDescriptorPool GetMainPool();
	VulkanHelper::GPUBuffer* GetGPUBuffer(const std::string& Name);

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity);
}
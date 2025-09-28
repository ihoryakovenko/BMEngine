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
	typedef u64 ResourceHandle;

	enum class ResourceType : u32
	{
		Texture = 0,
		Mesh = 1,
		Material = 2,
		Instance = 3,
	};

	// Helper functions for ResourceHandle
	inline ResourceHandle PackResourceHandle(ResourceType Type, u32 Index)
	{
		return (static_cast<u64>(Type) << 32) | static_cast<u64>(Index);
	}

	inline ResourceType GetResourceType(ResourceHandle Handle)
	{
		return static_cast<ResourceType>(Handle >> 32);
	}

	inline u32 GetResourceIndex(ResourceHandle Handle)
	{
		return static_cast<u32>(Handle & 0xFFFFFFFF);
	}

	struct ResourceDependency
	{
		ResourceHandle Handle;
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
		std::atomic<bool> IsLoaded;
	};

	struct Texture
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size;
		u64 Alignment;
		u32 Width;
		u32 Height;
		std::atomic<bool> IsLoaded;
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

	ResourceHandle CreateStaticMesh(MeshDescription* Description, void* Data);
	
	ResourceHandle CreateTexture(TextureDescription* Description, void* Data);
	
	ResourceHandle CreateStaticMeshInstance(u32 DataSize, const std::string& BufferName);
	ResourceHandle CreateMaterial(u32 DataSize, const std::string& BufferName);

	void AddResourceDependencyToMaterial(ResourceHandle Handle, ResourceDependency Dependency);
	void AddResourceDependencyToInstance(ResourceHandle Handle, ResourceDependency Dependency);

	void UpdateMaterial(ResourceHandle Handle, void* Data, u32 DataSize, const std::string& BufferName);
	void UpdateInstance(ResourceHandle Handle, void* Data, u32 DataSize, const std::string& BufferName);

	//void UpdateResource(u32 Handle, ResourceType Type, const std::string& BufferName, void* Data, u32 DataSize, u32 Offset);

	VertexData* GetStaticMesh(u32 Index);
	ResourceRecord* GetInstanceData(u32 Index);
	MeshTexture2D* GetTexture(u32 Index);

	void SetResourceReadyToRender(ResourceHandle Handle);
	VkDescriptorSetLayout GetBindlesTexturesLayout();
	VkDescriptorSetLayout GetMaterialLayout();
	VkDescriptorSet GetBindlesTexturesSet();
	VkDescriptorSet GetMaterialSet();
	VkDescriptorPool GetMainPool();
	VulkanHelper::GPUBuffer* GetGPUBuffer(const std::string& Name);

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity);
}
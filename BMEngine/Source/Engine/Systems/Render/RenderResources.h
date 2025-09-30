#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>
#include <vector>

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
	typedef u64 ResourceDependency;

	struct StorageBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Alignment;
		u32 RecordSize;
		u32 MaxRecords;
		u32 Count;
		VulkanHelper::BufferUsageFlag UsageFlag;
		VulkanHelper::MemoryPropertyFlag PropertyFlag;
		VulkanHelper::StageBarrier StageBarrier;
	};

	struct MeshBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		u64 Offset;
		VulkanHelper::BufferUsageFlag UsageFlag;
		VulkanHelper::MemoryPropertyFlag PropertyFlag;
		VulkanHelper::StageBarrier StageBarrier;
	};

	enum class ResourceType : u32
	{
		Texture = 0,
		Mesh = 1,
		StorageResource = 2,
	};

	struct ResourceRecord
	{
		std::atomic<bool> IsLoaded;
		ResourceDependency Dependency;
		u32 DependencyCount;
		u32 RecordSize;
		u32 RecordGPUIndex;
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

	struct StorageBufferDescription
	{
		VulkanHelper::BufferUsageFlag BufferUsageFlag;
		VulkanHelper::MemoryPropertyFlag MemoryPropertyFlag;
		u32 EntrySize;
		u32 Count;
		VulkanHelper::StageBarrier StageBarrier;
	};

	struct MeshBufferDescription
	{
		VulkanHelper::BufferUsageFlag BufferUsageFlag;
		VulkanHelper::MemoryPropertyFlag MemoryPropertyFlag;
		u64 Size;
		VulkanHelper::StageBarrier StageBarrier;
	};

	struct DescriptorSetDescription
	{
		std::string Layout;
		std::string Pool;
	};

	struct DescriptorSetLayoutDescription
	{
		std::vector<VkDescriptorSetLayoutBinding> Bindings;
		VkDescriptorSetLayoutCreateFlags Flags;
		const void* Next;
	};

	struct PipelineLayoutDescription
	{
		u32 SetLayoutCount;
		const VkDescriptorSetLayout* SetLayouts;
		u32 PushConstantRangeCount;
		const VkPushConstantRange* PushConstantRanges;
		VkPipelineLayoutCreateFlags Flags;
		const void* Next;
	};

	struct PipelineDescription
	{
		VkExtent2D Extent;
		VkPipelineLayout PipelineLayout;
		VulkanHelper::PipelineResourceInfo ResourceInfo;
		
		std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
		std::vector<VkVertexInputBindingDescription> VertexBindings;
		std::vector<VkVertexInputAttributeDescription> VertexAttributes;
		
		VkPipelineRasterizationStateCreateInfo RasterizationState;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo ColorBlendState;
		VkPipelineDepthStencilStateCreateInfo DepthStencilState;
		VkPipelineMultisampleStateCreateInfo MultisampleState;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
		VkPipelineViewportStateCreateInfo ViewportState;
		VkViewport Viewport;
		VkRect2D Scissor;
	};

	void Init(GLFWwindow* WindowHandler);
	void DeInit();

	void CreateVertex(const std::string& Name, VulkanHelper::VertexBinding& Binding);
	void CreateShader(const std::string& Name, const u32* Code, u64 CodeSize);
	void CreateSampler(const std::string& Name, const SamplerDescription& Data);
	void CreateStorageBuffer(const std::string& Name, const StorageBufferDescription& Description);
	void CreateMeshBuffer(const std::string& Name, const MeshBufferDescription& Description);
	void CreateDescriptorSetLayout(const std::string& Name, const DescriptorSetLayoutDescription& Description);
	void CreateDescriptorSet(const std::string& Name, const DescriptorSetDescription& Description);
	void CreateGraphicsPipeline(const std::string& Name, const PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const PipelineLayoutDescription& Description);

	void PostCreateInit();

	ResourceHandle CreateStaticMesh(MeshDescription* Description, void* Data, const std::string& BufferName);
	ResourceHandle CreateTexture(TextureDescription* Description, void* Data);
	ResourceHandle CreateStorageBufferResource(u32 DataSize, const std::string& BufferName, ResourceDependency Dependency, u32 DependencyCount);
	ResourceDependency CreateResourceDependency(ResourceHandle* Handles, u32 HandlesCount);

	void UpdateStorageBufferResource(ResourceHandle Handle, void* Data, u32 DataSize, const std::string& BufferName);

	void OnResourceLoaded(ResourceHandle Handle);

	u32 GetResourceGPUIndex(ResourceHandle Handle);
	VertexData* GetStaticMesh(u32 Index);
	ResourceRecord* GetInstanceData(u32 Index);
	MeshTexture2D* GetTexture(u32 Index);
	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VkDescriptorPool GetDescriptorPool(const std::string& Id);
	VkDescriptorSet GetDescriptorSet(const std::string& Id);
	RenderResources::StorageBuffer* GetStorageBuffer(const std::string& Name);
	RenderResources::MeshBuffer* GetMeshBuffer(const std::string& Name);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);

	bool IsDrawEntityLoaded(const Render::DrawEntity* Entity);
}
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

	struct GPUBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Capacity;
		VulkanHelper::BufferUsageFlag UsageFlag;
		VulkanHelper::MemoryPropertyFlag PropertyFlag;
		VulkanHelper::StageBarrier StageBarrier;
	};

	enum class ResourceType : u32
	{
		Texture = 0,
		StorageResource = 1,
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
		u64 Capacity;
		VulkanHelper::StageBarrier StageBarrier;
	};

	struct DescriptorSetBinding
	{
		std::string Buffer;
		u32 Binding;
		VkDescriptorType DescriptorType;
	};

	struct DescriptorSetDescription
	{
		std::string Layout;
		std::string Pool;
		std::vector<DescriptorSetBinding> Bindings;
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
	void CreateDescriptorSetLayout(const std::string& Name, const DescriptorSetLayoutDescription& Description);
	void CreateDescriptorSet(const std::string& Name, const DescriptorSetDescription& Description);
	void CreateGraphicsPipeline(const std::string& Name, const PipelineDescription& Description);
	void CreatePipelineLayout(const std::string& Name, const PipelineLayoutDescription& Description);

	ResourceHandle CreateTexture(TextureDescription* Description, void* Data);
	ResourceHandle CreateStorageBufferResource();

	void UpdateGPUBuffer(ResourceHandle Handle, void* Data, u32 DataSize, u64 Offset, const std::string& BufferName);

	void OnResourceLoaded(ResourceHandle Handle);

	MeshTexture2D* GetTexture(u32 Index);
	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);
	VkDescriptorPool GetDescriptorPool(const std::string& Id);
	VkDescriptorSet GetDescriptorSet(const std::string& Id);
	RenderResources::GPUBuffer* GetGPUBuffer(const std::string& Name);
	VulkanHelper::VertexBinding GetVertexBinding(const std::string& Id);
	VkPipeline GetPipeline(const std::string& Name);
	VkPipelineLayout GetPipelineLayout(const std::string& Name);

	bool IsResourceReady(ResourceHandle Handle);
}
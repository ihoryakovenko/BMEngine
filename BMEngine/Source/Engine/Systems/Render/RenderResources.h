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

typedef u64 BmRender_ResourceHandle;
typedef u64 BmRender_ImageViewHandle;

namespace RenderResources
{
	struct DescriptorSet
	{
		VkDescriptorSet Set;

	};

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

	struct ImageDescription
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
		u64 Offset;
		u64 Range;
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
		const void* Next;
	};

	struct ImageViewBindingDescription
	{
		const char* Sampler;
		u32 BindingIndex;
		u64 ArrayElement;
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

	BmRender_ResourceHandle CreateImageResource(ImageDescription* Description);
	BmRender_ImageViewHandle CreateImageView(BmRender_ResourceHandle Handle, VkFormat Format);
	BmRender_ResourceHandle CreateBufferResource(u64 BufferOffset, const std::string& BufferName);

	void BindImageView(BmRender_ImageViewHandle Handle, const std::string& Set, const ImageViewBindingDescription* BindingDescriptions, u32 Count);

	void UpdateBufferResource(BmRender_ResourceHandle Handle, u64 ResourceOffset, const void* Data, u32 DataSize);
	void UpdateImageResource(BmRender_ResourceHandle Handle, ImageDescription* Description, void* Data);

	void OnResourceLoaded(BmRender_ResourceHandle Handle);

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

	bool IsResourceReady(BmRender_ResourceHandle Handle);
}
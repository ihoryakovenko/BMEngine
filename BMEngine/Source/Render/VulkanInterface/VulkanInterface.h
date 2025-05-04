#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"
#include "Platform/WinPlatform.h"

namespace VulkanInterface
{
	static const u32 MAX_SWAPCHAIN_IMAGES_COUNT = 3;
	static const u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	static const u32 MAX_VERTEX_INPUT_BINDINGS = 16;

	enum LogType
	{
		BMRVkLogType_Error,
		BMRVkLogType_Warning,
		BMRVkLogType_Info
	};

	typedef void (*BMRVkLogHandler)(LogType, const char*, va_list);

	struct VulkanInterfaceConfig
	{
		BMRVkLogHandler LogHandler = nullptr;
		bool EnableValidationLayers = false;
		u32 MaxTextures = 0;
	};

	struct VertexInputAttribute
	{
		const char* VertexInputAttributeName = nullptr;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		u32 AttributeOffset = 0;
	};

	struct BMRVertexInputBinding
	{
		const char* VertexInputBindingName = nullptr;

		VertexInputAttribute InputAttributes[MAX_VERTEX_INPUTS_ATTRIBUTES];
		u32 InputAttributesCount = 0;

		u32 Stride = 0;
		VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;
	};

	struct PipelineSettings
	{
		const char* PipelineName = nullptr;

		VkExtent2D Extent;

		VkBool32 DepthClampEnable = VK_FALSE;
		VkBool32 RasterizerDiscardEnable = VK_FALSE;
		VkPolygonMode PolygonMode = VK_POLYGON_MODE_MAX_ENUM;
		f32 LineWidth = 0.0f;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_MAX_ENUM;
		VkBool32 DepthBiasEnable = VK_FALSE;

		VkBool32 LogicOpEnable = VK_FALSE;
		u32 AttachmentCount = 0;
		u32 ColorWriteMask = 0;
		VkBool32 BlendEnable = VK_FALSE;
		VkBlendFactor SrcColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp ColorBlendOp = VK_BLEND_OP_MAX_ENUM;
		VkBlendFactor SrcAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendFactor DstAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		VkBlendOp AlphaBlendOp = VK_BLEND_OP_MAX_ENUM;

		VkBool32 DepthTestEnable = VK_FALSE;
		VkBool32 DepthWriteEnable = VK_FALSE;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_MAX_ENUM;
		VkBool32 DepthBoundsTestEnable = VK_FALSE;
		VkBool32 StencilTestEnable = VK_FALSE;
	};

	struct UniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
		u64 Offset;
	};

	struct UniformImage
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformSetAttachmentInfo
	{
		union
		{
			VkDescriptorBufferInfo BufferInfo;
			VkDescriptorImageInfo ImageInfo;
		};

		VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	};

	struct UniformImageInterfaceCreateInfo
	{
		const void* pNext = nullptr;
		VkImageViewCreateFlags Flags = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkComponentMapping Components;
		VkImageSubresourceRange SubresourceRange;
	};

	struct AttachmentData
	{
		u32 ColorAttachmentCount;
		VkFormat ColorAttachmentFormats[16]; // get max attachments from device
		VkFormat DepthAttachmentFormat;
		VkFormat StencilAttachmentFormat;
	};

	struct PipelineResourceInfo
	{
		AttachmentData PipelineAttachmentData;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct RenderPipeline
	{
		VkPipeline Pipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct LayoutLayerTransitionData
	{
		u32 BaseArrayLayer;
		u32 LayerCount;
	};

	void Init(Platform::BMRWindowHandler WindowHandler, const VulkanInterfaceConfig& InConfig);
	void DeInit();

	// TO refactor
	u32 GetImageCount();
	VkImageView* GetSwapchainImageViews();
	VkImage* GetSwapchainImages();
	u32 AcquireNextImageIndex(); // Locks thread
	u32 TestGetImageIndex();
	VkCommandBuffer BeginDraw(u32 ImageIndex);
	void EndDraw(u32 ImageIndex);

	VkPipelineLayout CreatePipelineLayout(const VkDescriptorSetLayout* DescriptorLayouts,
		u32 DescriptorLayoutsCount, const VkPushConstantRange* PushConstant, u32 PushConstantsCount);
	UniformBuffer CreateUniformBufferInternal(const VkBufferCreateInfo* BufferInfo, VkMemoryPropertyFlags Properties);
	UniformImage CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo);
	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages,
		const VkDescriptorBindingFlags* BindingFlags, u32 BindingCount, u32 DescriptorCount);
	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets);
	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32* DescriptorCounts, u32 Count, VkDescriptorSet* OutSets);
	VkImageView CreateImageInterface(const UniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image);
	VkSampler CreateSampler(const VkSamplerCreateInfo* CreateInfo);
	bool CreateShader(const u32* Code, u32 CodeSize, VkShaderModule& VertexShaderModule);

	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, VkDeviceSize AlignedLayerSize,
		u32 LayersCount, void* Data, u32 Baselayer = 0);

	void DestroyPipelineLayout(VkPipelineLayout Layout);
	void DestroyPipeline(VkPipeline Pipeline);
	void DestroyUniformBuffer(UniformBuffer Buffer);
	void DestroyUniformImage(UniformImage Image);
	void DestroyUniformLayout(VkDescriptorSetLayout Layout);
	void DestroyImageInterface(VkImageView Interface);
	void DestroySampler(VkSampler Sampler);
	void DestroyShader(VkShaderModule Shader);

	void AttachUniformsToSet(VkDescriptorSet Set, const UniformSetAttachmentInfo* Infos, u32 BufferCount);
	void UpdateUniformBuffer(UniformBuffer Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, u32 LayersCount, void* Data, u32 Baselayer = 0);

	void TransitImageLayout(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask,
		VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage,
		LayoutLayerTransitionData* LayerData, u32 LayerDataCount);

	void WaitDevice();




	typedef UniformBuffer IndexBuffer;
	typedef UniformBuffer VertexBuffer;

	struct Shader
	{
		VkShaderStageFlagBits Stage;
		const char* Code;
		u32 CodeSize;
	};

	VkPipeline BatchPipelineCreation(const Shader* Shaders, u32 ShadersCount,
		const BMRVertexInputBinding* VertexInputBinding, u32 VertexInputBindingCount,
		const PipelineSettings* Settings, const PipelineResourceInfo* ResourceInfo);

	UniformBuffer CreateUniformBuffer(u64 Size);
	VertexBuffer CreateVertexBuffer(u64 Size);
	IndexBuffer CreateIndexBuffer(u64 Size);

	VkPhysicalDeviceProperties* GetDeviceProperties();
	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	
	VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);












	VkCommandBuffer GetCommandBuffer();
	VkCommandBuffer GetTransferCommandBuffer();
	VkQueue GetTransferQueue();
	VkFormat GetSurfaceFormat();
}

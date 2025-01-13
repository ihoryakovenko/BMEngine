#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"
#include "Platform/WinPlatform.h"

namespace BMRVulkan
{
	static const u32 MAX_SWAPCHAIN_IMAGES_COUNT = 3;
	static const u32 MAX_VERTEX_INPUTS_ATTRIBUTES = 16;
	static const u32 MAX_VERTEX_INPUT_BINDINGS = 16;

	enum BMRVkLogType
	{
		BMRVkLogType_Error,
		BMRVkLogType_Warning,
		BMRVkLogType_Info
	};

	typedef void (*BMRVkLogHandler)(BMRVkLogType, const char*, va_list);

	struct BMRVkConfig
	{
		BMRVkLogHandler LogHandler = nullptr;
		bool EnableValidationLayers = false;
		u32 MaxTextures = 0;
	};

	struct BMRVertexInputAttribute
	{
		const char* VertexInputAttributeName = nullptr;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		u32 AttributeOffset = 0;
	};

	struct BMRVertexInputBinding
	{
		const char* VertexInputBindingName = nullptr;

		BMRVertexInputAttribute InputAttributes[MAX_VERTEX_INPUTS_ATTRIBUTES];
		u32 InputAttributesCount = 0;

		u32 Stride = 0;
		VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;
	};

	struct BMRVertexInput
	{
		BMRVertexInputBinding VertexInputBinding[MAX_VERTEX_INPUT_BINDINGS];
		u32 VertexInputBindingCount = 0;
	};

	struct BMRPipelineSettings
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

	struct BMRSubpassSettings
	{
		const char* SubpassName = nullptr;

		const VkAttachmentReference* ColorAttachmentsReferences = nullptr;
		u32 ColorAttachmentsReferencesCount = 0;

		const VkAttachmentReference* DepthAttachmentReferences = nullptr;

		// First subpass has to not contain InputAttachments
		const VkAttachmentReference* InputAttachmentsReferences = nullptr;
		u32 InputAttachmentsReferencesCount = 0;
	};

	struct BMRRenderPassSettings
	{
		const char* RenderPassName = nullptr;

		// AttachmentDescriptions can be modified if contain undefined format
		VkAttachmentDescription* AttachmentDescriptions = nullptr;
		u32 AttachmentDescriptionsCount = 0;

		const BMRSubpassSettings* SubpassesSettings = nullptr;
		u32 SubpassSettingsCount = 0;

		VkSubpassDependency* SubpassDependencies = nullptr;
		// Has to be 1 more then SubpassSettingsCount
		u32 SubpassDependenciesCount = 0;

		const VkClearValue* ClearValues = nullptr;
		u32 ClearValuesCount = 0;
	};

	struct BMRUniform
	{
		union
		{
			VkBuffer Buffer = nullptr;
			VkImage Image;
		};

		VkDeviceMemory Memory = nullptr;
	};

	struct BMRUniformSetAttachmentInfo
	{
		union
		{
			VkDescriptorBufferInfo BufferInfo;
			VkDescriptorImageInfo ImageInfo;
		};

		VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	};

	struct BMRUniformImageInterfaceCreateInfo
	{
		const void* pNext = nullptr;
		VkImageViewCreateFlags Flags = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkComponentMapping Components;
		VkImageSubresourceRange SubresourceRange;
	};

	struct BMRAttachmentView
	{
		// Count should be equal to AttachmentDescriptionsCount
		VkImageView* ImageViews = nullptr;
	};

	struct BMRRenderTarget
	{
		BMRAttachmentView AttachmentViews[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct BMRFramebufferSet
	{
		VkFramebuffer FrameBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	};

	struct BMRRenderPass
	{
		VkRenderPass Pass = nullptr;

		VkClearValue* ClearValues = nullptr;
		u32 ClearValuesCount = 0;

		BMRFramebufferSet* RenderTargets = nullptr;
		u32 RenderTargetCount = 0;
	};

	struct BMRPipelineResourceInfo
	{
		VkRenderPass RenderPass = nullptr;
		u32 SubpassIndex = -1;

		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct BMRSPipelineShaderInfo
	{
		const VkPipelineShaderStageCreateInfo* Infos = nullptr;
		u32 InfosCounter = 0;
	};

	struct BMRPipeline
	{
		VkPipeline Pipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct BMRLayoutLayerTransitionData
	{
		u32 BaseArrayLayer;
		u32 LayerCount;
	};

	void Init(Platform::BMRWindowHandler WindowHandler, const BMRVkConfig& InConfig);
	void DeInit();

	u32 GetImageCount();
	VkImageView* GetSwapchainImageViews();
	u32 AcquireNextImageIndex(); // Locks thread
	VkCommandBuffer BeginDraw(u32 ImageIndex);
	void EndDraw(u32 ImageIndex);
	void BeginRenderPass(const BMRRenderPass* Pass, VkRect2D RenderArea, u32 RenderTargetIndex, u32 ImageIndex);

	void CreateRenderPass(const BMRRenderPassSettings* Settings, const BMRRenderTarget* Targets,
		VkExtent2D TargetExtent, u32 TargetCount, u32 SwapchainImagesCount, BMRRenderPass* OutPass);
	VkPipelineLayout CreatePipelineLayout(const VkDescriptorSetLayout* DescriptorLayouts,
		u32 DescriptorLayoutsCount, const VkPushConstantRange* PushConstant, u32 PushConstantsCount);
	void CreatePipelines(const BMRSPipelineShaderInfo* ShaderInputs, const BMRVertexInput* VertexInputs,
		const BMRPipelineSettings* PipelinesSettings, const BMRPipelineResourceInfo* ResourceInfos,
		u32 PipelinesCount, VkPipeline* OutputPipelines);
	BMRUniform CreateUniformBuffer(const VkBufferCreateInfo* BufferInfo, VkMemoryPropertyFlags Properties);
	BMRUniform CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo);
	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages, u32 Count);
	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets);
	VkImageView CreateImageInterface(const BMRUniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image);
	VkSampler CreateSampler(const VkSamplerCreateInfo* CreateInfo);
	bool CreateShader(const u32* Code, u32 CodeSize, VkShaderModule& VertexShaderModule);

	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, VkDeviceSize AlignedLayerSize,
		u32 LayersCount, void* Data);

	void DestroyRenderPass(BMRRenderPass* Pass);
	void DestroyPipelineLayout(VkPipelineLayout Layout);
	void DestroyPipeline(VkPipeline Pipeline);
	void DestroyUniformBuffer(BMRUniform Buffer);
	void DestroyUniformImage(BMRUniform Image);
	void DestroyUniformLayout(VkDescriptorSetLayout Layout);
	void DestroyImageInterface(VkImageView Interface);
	void DestroySampler(VkSampler Sampler);
	void DestroyShader(VkShaderModule Shader);

	void AttachUniformsToSet(VkDescriptorSet Set, const BMRUniformSetAttachmentInfo* Infos, u32 BufferCount);
	void UpdateUniformBuffer(BMRUniform Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, u32 LayersCount, void* Data);

	void TransitImageLayout(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask,
		VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage,
		BMRLayoutLayerTransitionData* LayerData, u32 LayerDataCount);

	void WaitDevice();
}

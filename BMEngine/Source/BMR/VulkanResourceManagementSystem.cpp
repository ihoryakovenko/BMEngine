#include "VulkanResourceManagementSystem.h"

#include "Memory/MemoryManagmentSystem.h"

#include "VulkanHelper.h"

namespace BMR::VulkanResourceManagementSystem
{
	static VkDevice LogicalDevice = nullptr;

	static u32 MaxPipelines = 0;
	static u32 MaxBuffers = 0;
	static u32 MaxDeviceMemorySources = 0;

	static VkPipelineLayout* PipelineLayouts = nullptr;
	static VkPipeline* Pipelines = nullptr;

	static u32 NextPipelineLayoutIndex = 0;
	static u32 NextPipelineIndex = 0;

	void Init(VkDevice ActiveLogicalDevice, u32 MaximumPipelines)
	{
		LogicalDevice = ActiveLogicalDevice;

		MaxPipelines = MaximumPipelines;

		PipelineLayouts = Memory::BmMemoryManagementSystem::Allocate<VkPipelineLayout>(MaxPipelines);
		Pipelines = Memory::BmMemoryManagementSystem::Allocate<VkPipeline>(MaxPipelines);
	}

	void DeInit()
	{
		for (u32 i = 0; i < NextPipelineLayoutIndex; ++i)
		{
			vkDestroyPipelineLayout(LogicalDevice, PipelineLayouts[i], nullptr);
		}

		for (u32 i = 0; i < NextPipelineIndex; ++i)
		{
			vkDestroyPipeline(LogicalDevice, Pipelines[i], nullptr);
		}

		Memory::BmMemoryManagementSystem::Deallocate(PipelineLayouts);
		Memory::BmMemoryManagementSystem::Deallocate(Pipelines);
	}

	VkPipelineLayout CreatePipelineLayout(const VkDescriptorSetLayout* DescriptorLayouts,
		u32 DescriptorLayoutsCount, const VkPushConstantRange* PushConstant, u32 PushConstantsCount)
	{
		assert(NextPipelineIndex < MaxPipelines);
		assert(DescriptorLayoutsCount <= MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = DescriptorLayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = DescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = PushConstantsCount;
		PipelineLayoutCreateInfo.pPushConstantRanges = PushConstant;

		VkPipelineLayout* PipelineLayout = PipelineLayouts + NextPipelineLayoutIndex;
		const VkResult Result = vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr,
			PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreatePipelineLayout result is %d", Result);
		}

		++NextPipelineLayoutIndex;
		return *PipelineLayout;
	}

	VkPipeline* CreatePipelines(const BMRSPipelineShaderInfo* ShaderInputs, const BMRVertexInput* VertexInputs,
		const BMRPipelineSettings* PipelinesSettings, const BMRPipelineResourceInfo* ResourceInfos,
		u32 PipelinesCount)
	{
		assert(NextPipelineLayoutIndex < MaxPipelines);

		auto PipelineCreateInfos = Memory::BmMemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>(PipelinesCount);
		auto VertexInputInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineVertexInputStateCreateInfo>(PipelinesCount);
		auto ViewportStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineViewportStateCreateInfo>(PipelinesCount);
		auto RasterizationStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineRasterizationStateCreateInfo>(PipelinesCount);
		auto ColorBlendAttachmentState = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendAttachmentState>(PipelinesCount);
		auto ColorBlendInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendStateCreateInfo>(PipelinesCount);
		auto DepthStencilInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineDepthStencilStateCreateInfo>(PipelinesCount);
		auto Viewports = Memory::BmMemoryManagementSystem::FrameAlloc<VkViewport>(PipelinesCount);
		auto Scissors = Memory::BmMemoryManagementSystem::FrameAlloc<VkRect2D>(PipelinesCount);

		// INPUTASSEMBLY
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { };
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// MULTISAMPLING
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { };
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		for (u32 PipelineIndex = 0; PipelineIndex < PipelinesCount; ++PipelineIndex)
		{
			const BMRVertexInput* VertexInput = VertexInputs + PipelineIndex;
			const BMRPipelineSettings* Settings = PipelinesSettings + PipelineIndex;
			VkViewport* Viewport = Viewports + PipelineIndex;
			VkRect2D* Scissor = Scissors + PipelineIndex;

			// VERTEX INPUT
			auto VertexInputBindings = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputBindingDescription>(VertexInput->VertexInputBindingCount);
			auto VertexInputAttributes = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputAttributeDescription>(VertexInput->VertexInputBindingCount * MAX_VERTEX_INPUTS_ATTRIBUTES);
			u32 VertexInputAttributesIndex = 0;

			HandleLog(BMR::BMRLogType::LogType_Info,
				"CREATING PIPELINE %s\n"
				"Extent - Width: %d, Height: %d\n"
				"DepthClampEnable: %d, RasterizerDiscardEnable: %d\n"
				"PolygonMode: %d, LineWidth: %f, CullMode: %d, FrontFace: %d, DepthBiasEnable: %d\n"
				"LogicOpEnable: %d, AttachmentCount: %d, ColorWriteMask: %d\n"
				"BlendEnable: %d, SrcColorBlendFactor: %d, DstColorBlendFactor: %d, ColorBlendOp: %d\n"
				"SrcAlphaBlendFactor: %d, DstAlphaBlendFactor: %d, AlphaBlendOp: %d\n"
				"DepthTestEnable: %d, DepthWriteEnable: %d, DepthCompareOp: %d\n"
				"DepthBoundsTestEnable: %d, StencilTestEnable: %d",
				Settings->PipelineName,
				Settings->Extent.Width, Settings->Extent.Height,
				Settings->DepthClampEnable, Settings->RasterizerDiscardEnable,
				Settings->PolygonMode, Settings->LineWidth, Settings->CullMode,
				Settings->FrontFace, Settings->DepthBiasEnable,
				Settings->LogicOpEnable, Settings->AttachmentCount, Settings->ColorWriteMask,
				Settings->BlendEnable, Settings->SrcColorBlendFactor,
				Settings->DstColorBlendFactor, Settings->ColorBlendOp,
				Settings->SrcAlphaBlendFactor, Settings->DstAlphaBlendFactor,
				Settings->AlphaBlendOp,
				Settings->DepthTestEnable, Settings->DepthWriteEnable,
				Settings->DepthCompareOp, Settings->DepthBoundsTestEnable,
				Settings->StencilTestEnable
			);

			HandleLog(BMR::BMRLogType::LogType_Info, "Creating vertex input, VertexInputBindingCount: %d", VertexInput->VertexInputBindingCount);

			for (u32 BindingIndex = 0; BindingIndex < VertexInput->VertexInputBindingCount; ++BindingIndex)
			{
				const BMRVertexInputBinding* BMRBinding = VertexInput->VertexInputBinding + BindingIndex;
				VkVertexInputBindingDescription* VertexInputBinding = VertexInputBindings + BindingIndex;

				VertexInputBinding->binding = BindingIndex;
				VertexInputBinding->inputRate = ToVkVertexInputRate(BMRBinding->InputRate);
				VertexInputBinding->stride = BMRBinding->Stride;

				HandleLog(BMR::BMRLogType::LogType_Info, "Initialized VkVertexInputBindingDescription, BindingName: %s, "
					"BindingIndex: %d, VkInputRate: %d, Stride: %d, InputAttributesCount: %d",
					BMRBinding->VertexInputBindingName, VertexInputBinding->binding, VertexInputBinding->inputRate,
					VertexInputBinding->stride, BMRBinding->InputAttributesCount);

				u32 AttributeOffset = 0;
				for (u32 CurrentAttributeIndex = 0; CurrentAttributeIndex < BMRBinding->InputAttributesCount; ++CurrentAttributeIndex)
				{
					const BMRVertexInputAttribute* BMRAttribute = BMRBinding->InputAttributes + CurrentAttributeIndex;
					VkVertexInputAttributeDescription* VertexInputAttribute = VertexInputAttributes + VertexInputAttributesIndex;

					VertexInputAttribute->binding = BindingIndex;
					VertexInputAttribute->location = CurrentAttributeIndex;
					VertexInputAttribute->format = ToVkFormat(BMRAttribute->Format);
					VertexInputAttribute->offset = AttributeOffset;

					AttributeOffset += BMRFormatSizesTable[BMRAttribute->Format];

					HandleLog(BMR::BMRLogType::LogType_Info, "Initialized VkVertexInputAttributeDescription, "
						"AttributeName: %s, BindingIndex: %d, Location: %d, VkFormat: %d, Offset: %d, Index in creation array: %d",
						BMRAttribute->VertexInputAttributeName, BindingIndex, CurrentAttributeIndex,
						VertexInputAttribute->format, VertexInputAttribute->offset, VertexInputAttributesIndex);

					++VertexInputAttributesIndex;
				}
			}

			VertexInputInfo[PipelineIndex] = { };
			VertexInputInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputInfo[PipelineIndex].vertexBindingDescriptionCount = VertexInput->VertexInputBindingCount;
			VertexInputInfo[PipelineIndex].pVertexBindingDescriptions = VertexInputBindings;
			VertexInputInfo[PipelineIndex].vertexAttributeDescriptionCount = VertexInputAttributesIndex;
			VertexInputInfo[PipelineIndex].pVertexAttributeDescriptions = VertexInputAttributes;

			// VIEWPORT
			Viewport->width = Settings->Extent.Width;
			Viewport->height = Settings->Extent.Height;
			Viewport->minDepth = 0.0f;
			Viewport->maxDepth = 1.0f;
			Viewport->x = 0.0f;
			Viewport->y = 0.0f;

			Scissor->extent.width = Settings->Extent.Width;
			Scissor->extent.height = Settings->Extent.Height;
			Scissor->offset = { };

			ViewportStateCreateInfo[PipelineIndex] = { };
			ViewportStateCreateInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			ViewportStateCreateInfo[PipelineIndex].viewportCount = 1;
			ViewportStateCreateInfo[PipelineIndex].pViewports = Viewport;
			ViewportStateCreateInfo[PipelineIndex].scissorCount = 1;
			ViewportStateCreateInfo[PipelineIndex].pScissors = Scissor;

			// RASTERIZATION
			RasterizationStateCreateInfo[PipelineIndex] = { };
			RasterizationStateCreateInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			RasterizationStateCreateInfo[PipelineIndex].depthClampEnable = Settings->DepthClampEnable;
			// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
			RasterizationStateCreateInfo[PipelineIndex].rasterizerDiscardEnable = Settings->RasterizerDiscardEnable;
			RasterizationStateCreateInfo[PipelineIndex].polygonMode = ToVkPolygonMode(Settings->PolygonMode);
			RasterizationStateCreateInfo[PipelineIndex].lineWidth = Settings->LineWidth;
			RasterizationStateCreateInfo[PipelineIndex].cullMode = Settings->CullMode;
			RasterizationStateCreateInfo[PipelineIndex].frontFace = ToVkFrontFace(Settings->FrontFace);
			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
			RasterizationStateCreateInfo[PipelineIndex].depthBiasEnable = Settings->DepthBiasEnable;

			// COLOR BLENDING
			// Colors to apply blending to
			ColorBlendAttachmentState[PipelineIndex].colorWriteMask = Settings->ColorWriteMask;
			ColorBlendAttachmentState[PipelineIndex].blendEnable = Settings->BlendEnable;
			// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)// Enable blending
			ColorBlendAttachmentState[PipelineIndex].srcColorBlendFactor = ToVkBlendFactor(Settings->SrcColorBlendFactor);
			ColorBlendAttachmentState[PipelineIndex].dstColorBlendFactor = ToVkBlendFactor(Settings->DstColorBlendFactor);
			ColorBlendAttachmentState[PipelineIndex].colorBlendOp = ToVkBlendOp(Settings->ColorBlendOp);
			ColorBlendAttachmentState[PipelineIndex].srcAlphaBlendFactor = ToVkBlendFactor(Settings->SrcAlphaBlendFactor);
			// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
//			   (new color alpha * new color) + ((1 - new color alpha) * old color)
			ColorBlendAttachmentState[PipelineIndex].dstAlphaBlendFactor = ToVkBlendFactor(Settings->DstAlphaBlendFactor);
			ColorBlendAttachmentState[PipelineIndex].alphaBlendOp = ToVkBlendOp(Settings->AlphaBlendOp);
			// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

			ColorBlendInfo[PipelineIndex] = { };
			ColorBlendInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ColorBlendInfo[PipelineIndex].logicOpEnable = Settings->LogicOpEnable; // Alternative to calculations is to use logical operations
			ColorBlendInfo[PipelineIndex].attachmentCount = Settings->AttachmentCount;
			ColorBlendInfo[PipelineIndex].pAttachments = Settings->AttachmentCount > 0 ? ColorBlendAttachmentState + PipelineIndex : nullptr;

			// DEPTH STENCIL
			DepthStencilInfo[PipelineIndex] = { };
			DepthStencilInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilInfo[PipelineIndex].depthTestEnable = Settings->DepthTestEnable;
			DepthStencilInfo[PipelineIndex].depthWriteEnable = Settings->DepthWriteEnable;
			DepthStencilInfo[PipelineIndex].depthCompareOp = ToVkCompareOp(Settings->DepthCompareOp);
			DepthStencilInfo[PipelineIndex].depthBoundsTestEnable = Settings->DepthBoundsTestEnable;
			DepthStencilInfo[PipelineIndex].stencilTestEnable = Settings->StencilTestEnable;

			// CREATE INFO
			const BMRPipelineResourceInfo* ResourceInfo = ResourceInfos + PipelineIndex;

			VkGraphicsPipelineCreateInfo* PipelineCreateInfo = PipelineCreateInfos + PipelineIndex;
			PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			PipelineCreateInfo->stageCount = ShaderInputs[PipelineIndex].InfosCounter;
			PipelineCreateInfo->pStages = ShaderInputs[PipelineIndex].Infos;
			PipelineCreateInfo->pVertexInputState = VertexInputInfo + PipelineIndex;
			PipelineCreateInfo->pInputAssemblyState = &InputAssemblyStateCreateInfo;
			PipelineCreateInfo->pViewportState = ViewportStateCreateInfo + PipelineIndex;
			PipelineCreateInfo->pDynamicState = nullptr;
			PipelineCreateInfo->pRasterizationState = RasterizationStateCreateInfo + PipelineIndex;
			PipelineCreateInfo->pMultisampleState = &MultisampleStateCreateInfo;
			PipelineCreateInfo->pColorBlendState = ColorBlendInfo + PipelineIndex;
			PipelineCreateInfo->pDepthStencilState = DepthStencilInfo + PipelineIndex;
			PipelineCreateInfo->layout = ResourceInfo->PipelineLayout;
			PipelineCreateInfo->renderPass = ResourceInfo->RenderPass;
			PipelineCreateInfo->subpass = ResourceInfo->SubpassIndex;

			// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
			PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
			PipelineCreateInfo->basePipelineIndex = -1;
		}

		VkPipeline* PipelinesToCreate = Pipelines + NextPipelineIndex;
		const VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, PipelinesCount,
			PipelineCreateInfos, nullptr, PipelinesToCreate);

		for (u32 i = 0; i < PipelinesCount; ++i)
		{
			for (u32 j = 0; j < PipelineCreateInfos[i].stageCount; ++j)
			{
				vkDestroyShaderModule(LogicalDevice, PipelineCreateInfos[i].pStages[j].module, nullptr);
			}
		}

		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateGraphicsPipelines result is %d", Result);
		}

		NextPipelineIndex += PipelinesCount;
		return PipelinesToCreate;
	}
}

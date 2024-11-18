#include "VulkanCoreTypes.h"

#include "VulkanHelper.h"

#include "BMRVulkan/BMRVulkan.h"

namespace BMR
{
	

	

	

	// CREATING DESCRIPTOR LAYOUTS AND PUSH CONSTATNTS
	//const BMRUniformInput* PipelineUniformInput = UniformInputs + PipelineIdex;

	//assert(PipelineUniformInput->layoutCount <= MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE);

	//auto DescriptorLayouts = Memory::BmMemoryManagementSystem::Allocate<VkDescriptorSetLayout>(PipelineUniformInput->layoutCount);

	//for (u32 SetIndex = 0; SetIndex < PipelineUniformInput->layoutCount; ++SetIndex)
	//{
	//	const BMRUniformSet* Set = PipelineUniformInput->BMRUniformLayout + SetIndex;

	//	assert(Set->BuffersCount <= MAX_DESCRIPTOR_BINDING_PER_SET);
	//	VkDescriptorSetLayoutBinding Bindings[MAX_DESCRIPTOR_BINDING_PER_SET];

	//	for (u32 BufferIndex = 0; BufferIndex < Set->BuffersCount; ++BufferIndex)
	//	{
	//		Bindings[BufferIndex].binding = BufferIndex;
	//		Bindings[BufferIndex].descriptorType = Set->BMRUniformBuffers[BufferIndex].Type;
	//		Bindings[BufferIndex].stageFlags = Set->BMRUniformBuffers[BufferIndex].StageFlags;
	//		Bindings[BufferIndex].descriptorCount = 1;
	//		Bindings[BufferIndex].pImmutableSamplers = nullptr;
	//	}

	//	VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
	//	LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//	LayoutCreateInfo.bindingCount = Set->BuffersCount;
	//	LayoutCreateInfo.pBindings = Bindings;

	//	const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo, nullptr,
	//		DescriptorLayouts + SetIndex);
	//	if (Result != VK_SUCCESS)
	//	{
	//		Memory::BmMemoryManagementSystem::Deallocate(DescriptorLayouts);
	//		HandleLog(BMRLogType::LogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
	//		continue;
	//	}
	//}

	//Pipeline->DescriptorLayoutsCount = PipelineUniformInput->layoutCount;

	//u32 PushConstantRangeCount = 0;
	//if (PipelineUniformInput->PushBuffer.Size > 0)
	//{
	//	PushConstantRangeCount = 1;
	//	Pipeline->PushConstants.offset = PipelineUniformInput->PushBuffer.Offset;
	//	Pipeline->PushConstants.stageFlags = PipelineUniformInput->PushBuffer.StageFlags;
	//	Pipeline->PushConstants.size = PipelineUniformInput->PushBuffer.Size;
	//}
}
#include "VulkanHelper.h"

#include <cassert>
#include <cstdarg>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "RenderTypes.h"
#include "RenderHelper.h"

VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface, const VkSurfaceFormatKHR* AvailableFormats, u32 Count)
{
	VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };

	// All formats available
	if (Count == 1 && AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		for (u32 i = 0; i < Count; ++i)
		{
			VkSurfaceFormatKHR AvailableFormat = AvailableFormats[i];
			if ((AvailableFormat.format == VK_FORMAT_R8G8B8_UNORM || AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				&& AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				SurfaceFormat = AvailableFormat;
				break;
			}
		}
	}

	assert(SurfaceFormat.format != VK_FORMAT_UNDEFINED);
	return SurfaceFormat;
}

u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties)
{
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

	for (u32 MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
	{
		if ((AllowedTypes & (1 << MemoryTypeIndex))	// Index of memory type must match corresponding bit in allowedTypes
			&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties) // Desired property bit flags are part of memory type's property flags
		{
			return MemoryTypeIndex;
		}
	}

	assert(false);
	return 0;
}

VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag, const VkAllocationCallbacks* Allocator)
{
	VkBufferCreateInfo BufferInfo = { };
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = Size;
	BufferInfo.usage = (VkFlags)Flag;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer Buffer;
	VULKAN_CHECK_RESULT(vkCreateBuffer(Device, &BufferInfo, Allocator, &Buffer));

	return Buffer;
}

DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer, MemoryPropertyFlag Properties, VkBufferUsageFlags BufferUsageFlags, const VkAllocationCallbacks* Allocator)
{
	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

	const u32 MemoryTypeIndex = GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, MemoryPropertyFlagToVkFlags(Properties));

	VkMemoryAllocateInfo MemoryAllocInfo = { };
	MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	MemoryAllocInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

	VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
	if (BufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		AllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		AllocateFlagsInfo.deviceMask = 0;
		MemoryAllocInfo.pNext = &AllocateFlagsInfo;
	}

	DeviceMemoryAllocResult Result;
	VULKAN_CHECK_RESULT(vkAllocateMemory(Device, &MemoryAllocInfo, Allocator, &Result.Memory));
	Result.Alignment = MemoryRequirements.alignment;
	Result.Size = MemoryRequirements.size;

	return Result;
}

DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkImage Image, MemoryPropertyFlag Properties, const VkAllocationCallbacks* Allocator)
{
	VkMemoryRequirements MemoryRequirements;
	vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

	const u32 MemoryTypeIndex = GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, MemoryPropertyFlagToVkFlags(Properties));

	VkMemoryAllocateInfo MemoryAllocInfo = { };
	MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	MemoryAllocInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

	DeviceMemoryAllocResult Result;
	VULKAN_CHECK_RESULT(vkAllocateMemory(Device, &MemoryAllocInfo, Allocator, &Result.Memory));
	Result.Alignment = MemoryRequirements.alignment;
	Result.Size = MemoryRequirements.size;

	return Result;
}

void UpdateHostCompatibleBufferMemory(VkDevice Device, VkDeviceMemory Memory, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data)
{
	void* MappedMemory;
	vkMapMemory(Device, Memory, Offset, DataSize, 0, &MappedMemory);
	std::memcpy(MappedMemory, Data, DataSize);
	vkUnmapMemory(Device, Memory);
}


void GetRequiredInstanceExtensions(const char** RequiredInstanceExtensions, u32 RequiredExtensionsCount,
	const char** ValidationExtensions, u32 ValidationExtensionsCount, const char** OutInstanceExtensions)
{
	for (u32 i = 0; i < RequiredExtensionsCount; ++i)
	{
		OutInstanceExtensions[i] = RequiredInstanceExtensions[i];
		RenderLog(LogType::Info, "Requested %s extension", OutInstanceExtensions[i]);
	}

	for (u32 i = 0; i < ValidationExtensionsCount; ++i)
	{
		OutInstanceExtensions[i + RequiredExtensionsCount] = ValidationExtensions[i];
		RenderLog(LogType::Info, "Requested %s extension", OutInstanceExtensions[i + RequiredExtensionsCount]);
	}
}

PhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
	VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
	PhysicalDeviceIndices Indices;
	Indices.GraphicsFamily = -1;
	Indices.PresentationFamily = -1;
	Indices.TransferFamily = -1;

	for (u32 i = 0; i < PropertiesCount; ++i)
	{
		const auto& prop = Properties[i];

		// Select graphics queue (only if not already selected)
		if (Indices.GraphicsFamily == -1 &&
			prop.queueCount > 0 &&
			(prop.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			Indices.GraphicsFamily = i;
		}

		// Select presentation queue (only if not already selected)
		if (Indices.PresentationFamily == -1 && prop.queueCount > 0)
		{
			VkBool32 presentationSupported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &presentationSupported);
			if (presentationSupported)
			{
				Indices.PresentationFamily = i;
			}
		}

		// Prefer a dedicated transfer queue
		if (prop.queueCount > 0 &&
			(prop.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
			!(prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&  // Prefer not to overlap with graphics
			Indices.TransferFamily == -1)
		{
			Indices.TransferFamily = i;
		}
	}

	// If no dedicated transfer queue, fall back to graphics queue
	if (Indices.TransferFamily == -1)
	{
		Indices.TransferFamily = Indices.GraphicsFamily;
	}

	return Indices;
}

bool CheckFormatSupport(VkPhysicalDevice PhysicalDevice, VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags)
{
	VkFormatProperties Properties;
	vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &Properties);

	if (Tiling == VK_IMAGE_TILING_LINEAR && (Properties.linearTilingFeatures & FeatureFlags) == FeatureFlags)
	{
		return true;
	}
	else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Properties.optimalTilingFeatures & FeatureFlags) == FeatureFlags)
	{
		return true;
	}

	return false;
}

void PrintDeviceData(VkPhysicalDeviceProperties* DeviceProperties, VkPhysicalDeviceFeatures* AvailableFeatures)
{
	RenderLog(LogType::Info,
		"VkPhysicalDeviceProperties:\n"
		"  apiVersion: %u\n  driverVersion: %u\n  vendorID: %u\n  deviceID: %u\n"
		"  deviceType: %u\n  deviceName: %s\n"
		"  pipelineCacheUUID: %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X\n"
		"  sparseProperties:\n    residencyStandard2DBlockShape: %u\n"
		"    residencyStandard2DMultisampleBlockShape: %u\n    residencyStandard3DBlockShape: %u\n"
		"    residencyAlignedMipSize: %u\n    residencyNonResidentStrict: %u",
		DeviceProperties->apiVersion, DeviceProperties->driverVersion,
		DeviceProperties->vendorID, DeviceProperties->deviceID,
		DeviceProperties->deviceType, DeviceProperties->deviceName,
		DeviceProperties->pipelineCacheUUID[0], DeviceProperties->pipelineCacheUUID[1], DeviceProperties->pipelineCacheUUID[2], DeviceProperties->pipelineCacheUUID[3],
		DeviceProperties->pipelineCacheUUID[4], DeviceProperties->pipelineCacheUUID[5], DeviceProperties->pipelineCacheUUID[6], DeviceProperties->pipelineCacheUUID[7],
		DeviceProperties->pipelineCacheUUID[8], DeviceProperties->pipelineCacheUUID[9], DeviceProperties->pipelineCacheUUID[10], DeviceProperties->pipelineCacheUUID[11],
		DeviceProperties->pipelineCacheUUID[12], DeviceProperties->pipelineCacheUUID[13], DeviceProperties->pipelineCacheUUID[14], DeviceProperties->pipelineCacheUUID[15],
		DeviceProperties->sparseProperties.residencyStandard2DBlockShape,
		DeviceProperties->sparseProperties.residencyStandard2DMultisampleBlockShape,
		DeviceProperties->sparseProperties.residencyStandard3DBlockShape,
		DeviceProperties->sparseProperties.residencyAlignedMipSize,
		DeviceProperties->sparseProperties.residencyNonResidentStrict
	);

	RenderLog(LogType::Info,
		"VkPhysicalDeviceFeatures:\n  robustBufferAccess: %d\n  fullDrawIndexUint32: %d\n"
		"  imageCubeArray: %d\n  independentBlend: %d\n  geometryShader: %d\n"
		"  tessellationShader: %d\n  sampleRateShading: %d\n  dualSrcBlend: %d\n"
		"  logicOp: %d\n  multiDrawIndirect: %d\n  drawIndirectFirstInstance: %d\n"
		"  depthClamp: %d\n  depthBiasClamp: %d\n  fillModeNonSolid: %d\n"
		"  depthBounds: %d\n  wideLines: %d\n  largePoints: %d\n  alphaToOne: %d\n"
		"  multiViewport: %d\n  samplerAnisotropy: %d\n  textureCompressionETC2: %d\n"
		"  textureCompressionASTC_LDR: %d\n  textureCompressionBC: %d\n"
		"  occlusionQueryPrecise: %d\n  pipelineStatisticsQuery: %d\n"
		"  vertexPipelineStoresAndAtomics: %d\n  fragmentStoresAndAtomics: %d\n"
		"  shaderTessellationAndGeometryPointSize: %d\n  shaderImageGatherExtended: %d\n"
		"  shaderStorageImageExtendedFormats: %d\n  shaderStorageImageMultisample: %d\n"
		"  shaderStorageImageReadWithoutFormat: %d\n  shaderStorageImageWriteWithoutFormat: %d\n"
		"  shaderUniformBufferArrayDynamicIndexing: %d\n  shaderSampledImageArrayDynamicIndexing: %d\n"
		"  shaderStorageBufferArrayDynamicIndexing: %d\n  shaderStorageImageArrayDynamicIndexing: %d\n"
		"  shaderClipDistance: %d\n  shaderCullDistance: %d\n  shaderFloat64: %d\n"
		"  shaderInt64: %d\n  shaderInt16: %d\n  shaderResourceResidency: %d\n"
		"  shaderResourceMinLod: %d\n  sparseBinding: %d\n  sparseResidencyBuffer: %d\n"
		"  sparseResidencyImage2D: %d\n  sparseResidencyImage3D: %d\n  sparseResidency2Samples: %d\n"
		"  sparseResidency4Samples: %d\n  sparseResidency8Samples: %d\n  sparseResidency16Samples: %d\n"
		"  sparseResidencyAliased: %d\n  variableMultisampleRate: %d\n  inheritedQueries: %d",
		AvailableFeatures->robustBufferAccess, AvailableFeatures->fullDrawIndexUint32, AvailableFeatures->imageCubeArray,
		AvailableFeatures->independentBlend, AvailableFeatures->geometryShader, AvailableFeatures->tessellationShader,
		AvailableFeatures->sampleRateShading, AvailableFeatures->dualSrcBlend, AvailableFeatures->logicOp,
		AvailableFeatures->multiDrawIndirect, AvailableFeatures->drawIndirectFirstInstance, AvailableFeatures->depthClamp,
		AvailableFeatures->depthBiasClamp, AvailableFeatures->fillModeNonSolid, AvailableFeatures->depthBounds,
		AvailableFeatures->wideLines, AvailableFeatures->largePoints, AvailableFeatures->alphaToOne,
		AvailableFeatures->multiViewport, AvailableFeatures->samplerAnisotropy, AvailableFeatures->textureCompressionETC2,
		AvailableFeatures->textureCompressionASTC_LDR, AvailableFeatures->textureCompressionBC,
		AvailableFeatures->occlusionQueryPrecise, AvailableFeatures->pipelineStatisticsQuery,
		AvailableFeatures->vertexPipelineStoresAndAtomics, AvailableFeatures->fragmentStoresAndAtomics,
		AvailableFeatures->shaderTessellationAndGeometryPointSize, AvailableFeatures->shaderImageGatherExtended,
		AvailableFeatures->shaderStorageImageExtendedFormats, AvailableFeatures->shaderStorageImageMultisample,
		AvailableFeatures->shaderStorageImageReadWithoutFormat, AvailableFeatures->shaderStorageImageWriteWithoutFormat,
		AvailableFeatures->shaderUniformBufferArrayDynamicIndexing, AvailableFeatures->shaderSampledImageArrayDynamicIndexing,
		AvailableFeatures->shaderStorageBufferArrayDynamicIndexing, AvailableFeatures->shaderStorageImageArrayDynamicIndexing,
		AvailableFeatures->shaderClipDistance, AvailableFeatures->shaderCullDistance,
		AvailableFeatures->shaderFloat64, AvailableFeatures->shaderInt64, AvailableFeatures->shaderInt16,
		AvailableFeatures->shaderResourceResidency, AvailableFeatures->shaderResourceMinLod,
		AvailableFeatures->sparseBinding, AvailableFeatures->sparseResidencyBuffer,
		AvailableFeatures->sparseResidencyImage2D, AvailableFeatures->sparseResidencyImage3D,
		AvailableFeatures->sparseResidency2Samples, AvailableFeatures->sparseResidency4Samples,
		AvailableFeatures->sparseResidency8Samples, AvailableFeatures->sparseResidency16Samples,
		AvailableFeatures->sparseResidencyAliased, AvailableFeatures->variableMultisampleRate,
		AvailableFeatures->inheritedQueries
	);

	RenderLog(LogType::Info,
		"VkPhysicalDeviceLimits:\n  maxImageDimension1D: %u\n  maxImageDimension2D: %u\n"
		"  maxImageDimension3D: %u\n  maxImageDimensionCube: %u\n"
		"  maxImageArrayLayers: %u\n  maxTexelBufferElements: %u\n"
		"  maxUniformBufferRange: %u\n  maxStorageBufferRange: %u\n"
		"  maxPushConstantsSize: %u\n  maxMemoryAllocationCount: %u\n"
		"  maxSamplerAllocationCount: %u\n  bufferImageGranularity: %llu\n"
		"  sparseAddressSpaceSize: %llu\n  maxBoundDescriptorSets: %u\n"
		"  maxPerStageDescriptorSamplers: %u\n  maxPerStageDescriptorUniformBuffers: %u\n"
		"  maxPerStageDescriptorStorageBuffers: %u\n  maxPerStageDescriptorSampledImages: %u\n"
		"  maxPerStageDescriptorStorageImages: %u\n  maxPerStageDescriptorInputAttachments: %u\n"
		"  maxPerStageResources: %u\n  maxDescriptorSetSamplers: %u\n"
		"  maxDescriptorSetUniformBuffers: %u\n  maxDescriptorSetUniformBuffersDynamic: %u\n"
		"  maxDescriptorSetStorageBuffers: %u\n  maxDescriptorSetStorageBuffersDynamic: %u\n"
		"  maxDescriptorSetSampledImages: %u\n  maxDescriptorSetStorageImages: %u\n"
		"  maxDescriptorSetInputAttachments: %u\n  maxVertexInputAttributes: %u\n"
		"  maxVertexInputBindings: %u\n  maxVertexInputAttributeOffset: %u\n"
		"  maxVertexInputBindingStride: %u\n  maxVertexOutputComponents: %u\n"
		"  maxTessellationGenerationLevel: %u\n  maxTessellationPatchSize: %u\n"
		"  maxTessellationControlPerVertexInputComponents: %u\n  maxTessellationControlPerVertexOutputComponents: %u\n"
		"  maxTessellationControlPerPatchOutputComponents: %u\n  maxTessellationControlTotalOutputComponents: %u\n"
		"  maxTessellationEvaluationInputComponents: %u\n  maxTessellationEvaluationOutputComponents: %u\n"
		"  maxGeometryShaderInvocations: %u\n  maxGeometryInputComponents: %u\n"
		"  maxGeometryOutputComponents: %u\n  maxGeometryOutputVertices: %u\n"
		"  maxGeometryTotalOutputComponents: %u\n  maxFragmentInputComponents: %u\n"
		"  maxFragmentOutputAttachments: %u\n  maxFragmentDualSrcAttachments: %u\n"
		"  maxFragmentCombinedOutputResources: %u\n  maxComputeSharedMemorySize: %u\n"
		"  maxComputeWorkGroupCount[0]: %u\n  maxComputeWorkGroupCount[1]: %u\n"
		"  maxComputeWorkGroupCount[2]: %u\n  maxComputeWorkGroupInvocations: %u\n"
		"  maxComputeWorkGroupSize[0]: %u\n  maxComputeWorkGroupSize[1]: %u\n"
		"  maxComputeWorkGroupSize[2]: %u\n  subPixelPrecisionBits: %u\n"
		"  subTexelPrecisionBits: %u\n  mipmapPrecisionBits: %u\n"
		"  maxDrawIndexedIndexValue: %u\n  maxDrawIndirectCount: %u\n"
		"  maxSamplerLodBias: %f\n  maxSamplerAnisotropy: %f\n"
		"  maxViewports: %u\n  maxViewportDimensions[0]: %u\n"
		"  maxViewportDimensions[1]: %u\n  viewportBoundsRange[0]: %f\n"
		"  viewportBoundsRange[1]: %f\n  viewportSubPixelBits: %u\n"
		"  minMemoryMapAlignment: %zu\n  minTexelBufferOffsetAlignment: %llu\n"
		"  minUniformBufferOffsetAlignment: %llu\n  minStorageBufferOffsetAlignment: %llu\n"
		"  minTexelOffset: %d\n  maxTexelOffset: %u\n"
		"  minTexelGatherOffset: %d\n  maxTexelGatherOffset: %u\n"
		"  minInterpolationOffset: %f\n  maxInterpolationOffset: %f\n"
		"  subPixelInterpolationOffsetBits: %u\n  maxFramebufferWidth: %u\n"
		"  maxFramebufferHeight: %u\n  maxFramebufferLayers: %u\n"
		"  framebufferColorSampleCounts: %u\n  framebufferDepthSampleCounts: %u\n"
		"  framebufferStencilSampleCounts: %u\n  framebufferNoAttachmentsSampleCounts: %u\n"
		"  maxColorAttachments: %u\n  sampledImageColorSampleCounts: %u\n"
		"  sampledImageIntegerSampleCounts: %u\n  sampledImageDepthSampleCounts: %u\n"
		"  sampledImageStencilSampleCounts: %u\n  storageImageSampleCounts: %u\n"
		"  maxSampleMaskWords: %u\n  timestampComputeAndGraphics: %d\n"
		"  timestampPeriod: %f\n  maxClipDistances: %u\n"
		"  maxCullDistances: %u\n  maxCombinedClipAndCullDistances: %u\n"
		"  discreteQueuePriorities: %u\n  pointSizeRange[0]: %f\n"
		"  pointSizeRange[1]: %f\n  lineWidthRange[0]: %f\n"
		"  lineWidthRange[1]: %f\n  pointSizeGranularity: %f\n"
		"  lineWidthGranularity: %f\n  strictLines: %d\n"
		"  standardSampleLocations: %d\n  optimalBufferCopyOffsetAlignment: %llu\n"
		"  optimalBufferCopyRowPitchAlignment: %llu\n  nonCoherentAtomSize: %llu",
		DeviceProperties->limits.maxImageDimension1D, DeviceProperties->limits.maxImageDimension2D,
		DeviceProperties->limits.maxImageDimension3D, DeviceProperties->limits.maxImageDimensionCube,
		DeviceProperties->limits.maxImageArrayLayers, DeviceProperties->limits.maxTexelBufferElements,
		DeviceProperties->limits.maxUniformBufferRange, DeviceProperties->limits.maxStorageBufferRange,
		DeviceProperties->limits.maxPushConstantsSize, DeviceProperties->limits.maxMemoryAllocationCount,
		DeviceProperties->limits.maxSamplerAllocationCount, DeviceProperties->limits.bufferImageGranularity,
		DeviceProperties->limits.sparseAddressSpaceSize, DeviceProperties->limits.maxBoundDescriptorSets,
		DeviceProperties->limits.maxPerStageDescriptorSamplers, DeviceProperties->limits.maxPerStageDescriptorUniformBuffers,
		DeviceProperties->limits.maxPerStageDescriptorStorageBuffers, DeviceProperties->limits.maxPerStageDescriptorSampledImages,
		DeviceProperties->limits.maxPerStageDescriptorStorageImages, DeviceProperties->limits.maxPerStageDescriptorInputAttachments,
		DeviceProperties->limits.maxPerStageResources, DeviceProperties->limits.maxDescriptorSetSamplers,
		DeviceProperties->limits.maxDescriptorSetUniformBuffers, DeviceProperties->limits.maxDescriptorSetUniformBuffersDynamic,
		DeviceProperties->limits.maxDescriptorSetStorageBuffers, DeviceProperties->limits.maxDescriptorSetStorageBuffersDynamic,
		DeviceProperties->limits.maxDescriptorSetSampledImages, DeviceProperties->limits.maxDescriptorSetStorageImages,
		DeviceProperties->limits.maxDescriptorSetInputAttachments, DeviceProperties->limits.maxVertexInputAttributes,
		DeviceProperties->limits.maxVertexInputBindings, DeviceProperties->limits.maxVertexInputAttributeOffset,
		DeviceProperties->limits.maxVertexInputBindingStride, DeviceProperties->limits.maxVertexOutputComponents,
		DeviceProperties->limits.maxTessellationGenerationLevel, DeviceProperties->limits.maxTessellationPatchSize,
		DeviceProperties->limits.maxTessellationControlPerVertexInputComponents,
		DeviceProperties->limits.maxTessellationControlPerVertexOutputComponents,
		DeviceProperties->limits.maxTessellationControlPerPatchOutputComponents,
		DeviceProperties->limits.maxTessellationControlTotalOutputComponents,
		DeviceProperties->limits.maxTessellationEvaluationInputComponents,
		DeviceProperties->limits.maxTessellationEvaluationOutputComponents,
		DeviceProperties->limits.maxGeometryShaderInvocations, DeviceProperties->limits.maxGeometryInputComponents,
		DeviceProperties->limits.maxGeometryOutputComponents, DeviceProperties->limits.maxGeometryOutputVertices,
		DeviceProperties->limits.maxGeometryTotalOutputComponents, DeviceProperties->limits.maxFragmentInputComponents,
		DeviceProperties->limits.maxFragmentOutputAttachments, DeviceProperties->limits.maxFragmentDualSrcAttachments,
		DeviceProperties->limits.maxFragmentCombinedOutputResources, DeviceProperties->limits.maxComputeSharedMemorySize,
		DeviceProperties->limits.maxComputeWorkGroupCount[0], DeviceProperties->limits.maxComputeWorkGroupCount[1],
		DeviceProperties->limits.maxComputeWorkGroupCount[2], DeviceProperties->limits.maxComputeWorkGroupInvocations,
		DeviceProperties->limits.maxComputeWorkGroupSize[0], DeviceProperties->limits.maxComputeWorkGroupSize[1],
		DeviceProperties->limits.maxComputeWorkGroupSize[2], DeviceProperties->limits.subPixelPrecisionBits,
		DeviceProperties->limits.subTexelPrecisionBits, DeviceProperties->limits.mipmapPrecisionBits,
		DeviceProperties->limits.maxDrawIndexedIndexValue, DeviceProperties->limits.maxDrawIndirectCount,
		DeviceProperties->limits.maxSamplerLodBias, DeviceProperties->limits.maxSamplerAnisotropy,
		DeviceProperties->limits.maxViewports, DeviceProperties->limits.maxViewportDimensions[0],
		DeviceProperties->limits.maxViewportDimensions[1], DeviceProperties->limits.viewportBoundsRange[0],
		DeviceProperties->limits.viewportBoundsRange[1], DeviceProperties->limits.viewportSubPixelBits,
		DeviceProperties->limits.minMemoryMapAlignment, DeviceProperties->limits.minTexelBufferOffsetAlignment,
		DeviceProperties->limits.minUniformBufferOffsetAlignment, DeviceProperties->limits.minStorageBufferOffsetAlignment,
		DeviceProperties->limits.minTexelOffset, DeviceProperties->limits.maxTexelOffset,
		DeviceProperties->limits.minTexelGatherOffset, DeviceProperties->limits.maxTexelGatherOffset,
		DeviceProperties->limits.minInterpolationOffset, DeviceProperties->limits.maxInterpolationOffset,
		DeviceProperties->limits.subPixelInterpolationOffsetBits, DeviceProperties->limits.maxFramebufferWidth,
		DeviceProperties->limits.maxFramebufferHeight, DeviceProperties->limits.maxFramebufferLayers,
		DeviceProperties->limits.framebufferColorSampleCounts, DeviceProperties->limits.framebufferDepthSampleCounts,
		DeviceProperties->limits.framebufferStencilSampleCounts, DeviceProperties->limits.framebufferNoAttachmentsSampleCounts,
		DeviceProperties->limits.maxColorAttachments, DeviceProperties->limits.sampledImageColorSampleCounts,
		DeviceProperties->limits.sampledImageIntegerSampleCounts, DeviceProperties->limits.sampledImageDepthSampleCounts,
		DeviceProperties->limits.sampledImageStencilSampleCounts, DeviceProperties->limits.storageImageSampleCounts,
		DeviceProperties->limits.maxSampleMaskWords, DeviceProperties->limits.timestampComputeAndGraphics,
		DeviceProperties->limits.timestampPeriod, DeviceProperties->limits.maxClipDistances,
		DeviceProperties->limits.maxCullDistances, DeviceProperties->limits.maxCombinedClipAndCullDistances,
		DeviceProperties->limits.discreteQueuePriorities, DeviceProperties->limits.pointSizeRange[0],
		DeviceProperties->limits.pointSizeRange[1], DeviceProperties->limits.lineWidthRange[0],
		DeviceProperties->limits.lineWidthRange[1], DeviceProperties->limits.pointSizeGranularity,
		DeviceProperties->limits.lineWidthGranularity, DeviceProperties->limits.strictLines,
		DeviceProperties->limits.standardSampleLocations, DeviceProperties->limits.optimalBufferCopyOffsetAlignment,
		DeviceProperties->limits.optimalBufferCopyRowPitchAlignment, DeviceProperties->limits.nonCoherentAtomSize
	);
}

VkExtent2D GetBestSwapExtent(VkPhysicalDevice PhysicalDevice, GLFWwindow* WindowHandler, VkSurfaceKHR Surface)
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

	VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities));

	VkExtent2D SwapExtent;

	if (SurfaceCapabilities.currentExtent.width != UINT32_MAX)
	{
		SwapExtent = SurfaceCapabilities.currentExtent;
	}
	else
	{
		s32 Width;
		s32 Height;
		glfwGetFramebufferSize(WindowHandler, &Width, &Height);

		Width = glm::clamp(static_cast<u32>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
		Height = glm::clamp(static_cast<u32>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

		SwapExtent = { static_cast<u32>(Width), static_cast<u32>(Height) };
	}

	return SwapExtent;
}

VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
	u32 PresentModeCount;
	VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, nullptr));

	auto PresentModes = (VkPresentModeKHR*)Memory_LinearAllocator_Alloc(GetFrameMemory(), PresentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, PresentModes);

	for (u32 i = 0; i < PresentModeCount; ++i)
	{
		RenderLog(LogType::Info, "Present mode %d is available", PresentModes[i]);
	}

	VkPresentModeKHR Mode = VK_PRESENT_MODE_FIFO_KHR;
	for (u32 i = 0; i < PresentModeCount; ++i)
	{
		if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Mode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	// Has to be present by spec
	if (Mode != VK_PRESENT_MODE_MAILBOX_KHR)
	{
		RenderLog(LogType::Warning, "Using default VK_PRESENT_MODE_FIFO_KHR");
	}

	return Mode;
}

bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
	const char** RequiredExtensions, u32 RequiredExtensionsCount)
{
	for (u32 i = 0; i < RequiredExtensionsCount; ++i)
	{
		bool IsExtensionSupported = false;
		for (u32 j = 0; j < AvailableExtensionsCount; ++j)
		{
			if (std::strcmp(RequiredExtensions[i], AvailableExtensions[j].extensionName) == 0)
			{
				IsExtensionSupported = true;
				break;
			}
		}

		if (!IsExtensionSupported)
		{
			RenderLog(LogType::Error, "Extension %s unsupported", RequiredExtensions[i]);
			return false;
		}
	}

	return true;
}

bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
	const char** ValidationLayersToCheck, u32 ValidationLayersToCheckSize)
{
	for (u32 i = 0; i < ValidationLayersToCheckSize; ++i)
	{
		bool IsLayerAvailable = false;
		for (u32 j = 0; j < PropertiesSize; ++j)
		{
			if (std::strcmp(ValidationLayersToCheck[i], Properties[j].layerName) == 0)
			{
				IsLayerAvailable = true;
				break;
			}
		}

		if (!IsLayerAvailable)
		{
			RenderLog(LogType::Error, "Validation layer %s unsupported", ValidationLayersToCheck[i]);
			return false;
		}
	}

	return true;
}

bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
	const char** ExtensionsToCheck, u32 ExtensionsToCheckSize)
{
	for (u32 i = 0; i < ExtensionsToCheckSize; ++i)
	{
		bool IsDeviceExtensionSupported = false;
		for (u32 j = 0; j < ExtensionPropertiesCount; ++j)
		{
			if (std::strcmp(ExtensionsToCheck[i], ExtensionProperties[j].extensionName) == 0)
			{
				IsDeviceExtensionSupported = true;
				break;
			}
		}

		if (!IsDeviceExtensionSupported)
		{
			RenderLog(LogType::Error, "extension %s unsupported", ExtensionsToCheck[i]);
			return false;
		}
	}

	return true;
}

bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
	const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger)
{
	auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
	if (CreateMessengerFunc != nullptr)
	{
		VULKAN_CHECK_RESULT(CreateMessengerFunc(Instance, CreateInfo, Allocator, InDebugMessenger));
	}
	else
	{
		RenderLog(LogType::Error, "CreateMessengerFunc is nullptr");
		return false;
	}

	return true;
}

bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
	const VkAllocationCallbacks* Allocator)
{
	auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (DestroyMessengerFunc != nullptr)
	{
		DestroyMessengerFunc(Instance, InDebugMessenger, Allocator);
		return true;
	}
	else
	{
		RenderLog(LogType::Error, "DestroyMessengerFunc is nullptr");
		return false;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	[[maybe_unused]] void* UserData)
{
	if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		RenderLog(LogType::Error, CallbackData->pMessage);
	}
	else if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		RenderLog(LogType::Warning, CallbackData->pMessage);
	}
	else
	{
		RenderLog(LogType::Info, CallbackData->pMessage);
	}

	return VK_FALSE;
}

VkShaderStageFlags DescriptorShaderStageToVkShaderStage(BmRender_DescriptorShaderStage stage)
{
	VkShaderStageFlags flags = 0;
	if ((u64)stage & (u64)BmRender_DescriptorShaderStage::Vertex)   flags |= VK_SHADER_STAGE_VERTEX_BIT;
	if ((u64)stage & (u64)BmRender_DescriptorShaderStage::Fragment) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if ((u64)stage & (u64)BmRender_DescriptorShaderStage::Compute)  flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	return flags;
}

VkShaderStageFlagBits PipelineShaderStageToVkShaderStage(BmRender_PipelineShaderStage stage)
{
	switch (stage)
	{
		case BmRender_PipelineShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
		case BmRender_PipelineShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case BmRender_PipelineShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case BmRender_PipelineShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case BmRender_PipelineShaderStage::TessEval:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case BmRender_PipelineShaderStage::Compute:     return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	return VK_SHADER_STAGE_VERTEX_BIT;
}

VkPipelineStageFlags PipelineSyncToVkPipelineStage(BmRender_PipelineSyncStage stage)
{
	VkPipelineStageFlags flags = 0;
	if ((u64)stage & (u64)BmRender_PipelineSyncStage::VertexShader)          flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	if ((u64)stage & (u64)BmRender_PipelineSyncStage::FragmentShader)        flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	if ((u64)stage & (u64)BmRender_PipelineSyncStage::ColorAttachmentOutput) flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	if ((u64)stage & (u64)BmRender_PipelineSyncStage::ComputeShader)         flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	return flags;
}

VkPipelineBindPoint PipelineTypeToVkPipelineBindPoint(BmRender_PipelineType Type)
{
	switch (Type)
	{
		case BmRender_PipelineType::Graphics:
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case BmRender_PipelineType::Compute:
			return VK_PIPELINE_BIND_POINT_COMPUTE;
		default:
			assert(false);
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
	}
}

VkImageAspectFlags ImageTypeToVkImageAspectFlags(BmRender_ImageType Type)
{
	switch (Type)
	{
		case BmRender_ImageType::TransferSampled:
		case BmRender_ImageType::ColorAttachmentSampled:
		case BmRender_ImageType::MultiSampledColorAttachment:
			return VK_IMAGE_ASPECT_COLOR_BIT;

		case BmRender_ImageType::DepthSamplad:
		case BmRender_ImageType::MultiSampledDepthAttachment:
			return VK_IMAGE_ASPECT_DEPTH_BIT;

		default:
			assert(false);
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VkFilter FilterToVkFilter(BmRender_Filter Filter)
{
	switch (Filter)
	{
		case BmRender_Filter::Nearest:
			return VK_FILTER_NEAREST;
		case BmRender_Filter::Linear:
			return VK_FILTER_LINEAR;
		default:
			assert(false);
			return VK_FILTER_NEAREST;
	}
}

VkSamplerMipmapMode SamplerMipmapModeToVk(BmRender_SamplerMipmapMode Mode)
{
	switch (Mode)
	{
		case BmRender_SamplerMipmapMode::Nearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case BmRender_SamplerMipmapMode::Linear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
			assert(false);
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
}

VkSamplerAddressMode SamplerAddressModeToVk(BmRender_SamplerAddressMode Mode)
{
	switch (Mode)
	{
		case BmRender_SamplerAddressMode::Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case BmRender_SamplerAddressMode::MirroredRepeat:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case BmRender_SamplerAddressMode::ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case BmRender_SamplerAddressMode::ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case BmRender_SamplerAddressMode::MirrorClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
			assert(false);
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
}

VkCompareOp CompareOpToVk(BmRender_CompareOp Op)
{
	switch (Op)
	{
		case BmRender_CompareOp::Never:
			return VK_COMPARE_OP_NEVER;
		case BmRender_CompareOp::Less:
			return VK_COMPARE_OP_LESS;
		case BmRender_CompareOp::Equal:
			return VK_COMPARE_OP_EQUAL;
		case BmRender_CompareOp::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case BmRender_CompareOp::Greater:
			return VK_COMPARE_OP_GREATER;
		case BmRender_CompareOp::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;
		case BmRender_CompareOp::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case BmRender_CompareOp::Always:
			return VK_COMPARE_OP_ALWAYS;
		default:
			assert(false);
			return VK_COMPARE_OP_NEVER;
	}
}

VkBorderColor BorderColorToVk(BmRender_BorderColor Color)
{
	switch (Color)
	{
		case BmRender_BorderColor::FloatTransparentBlack:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case BmRender_BorderColor::IntTransparentBlack:
			return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		case BmRender_BorderColor::FloatOpaqueBlack:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case BmRender_BorderColor::IntOpaqueBlack:
			return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		case BmRender_BorderColor::FloatOpaqueWhite:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case BmRender_BorderColor::IntOpaqueWhite:
			return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		default:
			assert(false);
			return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	}
}

VkImageLayout ImageLayoutToVk(BmRender_ImageLayout Layout)
{
	switch (Layout)
	{
		case BmRender_ImageLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case BmRender_ImageLayout::General:
			return VK_IMAGE_LAYOUT_GENERAL;
		case BmRender_ImageLayout::ColorAttachmentOptimal:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case BmRender_ImageLayout::DepthStencilAttachmentOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case BmRender_ImageLayout::DepthStencilReadOnlyOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case BmRender_ImageLayout::ShaderReadOnlyOptimal:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case BmRender_ImageLayout::TransferSrcOptimal:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case BmRender_ImageLayout::TransferDstOptimal:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case BmRender_ImageLayout::Preinitialized:
			return VK_IMAGE_LAYOUT_PREINITIALIZED;
		case BmRender_ImageLayout::PresentSrcKHR:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			assert(false);
			return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

VkAttachmentLoadOp AttachmentLoadOpToVk(BmRender_AttachmentLoadOp Op)
{
	switch (Op)
	{
		case BmRender_AttachmentLoadOp::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case BmRender_AttachmentLoadOp::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case BmRender_AttachmentLoadOp::DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:
			assert(false);
			return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

VkAttachmentStoreOp AttachmentStoreOpToVk(BmRender_AttachmentStoreOp Op)
{
	switch (Op)
	{
		case BmRender_AttachmentStoreOp::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case BmRender_AttachmentStoreOp::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default:
			assert(false);
			return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

VkDescriptorType DescriptorTypeToVk(BmRender_DescriptorType Type)
{
	switch (Type)
	{
		case BmRender_DescriptorType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case BmRender_DescriptorType::CombinedImageSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case BmRender_DescriptorType::SampledImage:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BmRender_DescriptorType::StorageImage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case BmRender_DescriptorType::UniformTexelBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case BmRender_DescriptorType::StorageTexelBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case BmRender_DescriptorType::UniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case BmRender_DescriptorType::StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case BmRender_DescriptorType::UniformBufferDynamic:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case BmRender_DescriptorType::StorageBufferDynamic:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case BmRender_DescriptorType::InputAttachment:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		default:
			assert(false);
			return VK_DESCRIPTOR_TYPE_SAMPLER;
	}
}

BmRender_DescriptorType VkDescriptorTypeToBmRender(VkDescriptorType Type)
{
	switch (Type)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			return BmRender_DescriptorType::Sampler;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return BmRender_DescriptorType::CombinedImageSampler;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return BmRender_DescriptorType::SampledImage;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return BmRender_DescriptorType::StorageImage;
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return BmRender_DescriptorType::UniformTexelBuffer;
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return BmRender_DescriptorType::StorageTexelBuffer;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return BmRender_DescriptorType::UniformBuffer;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return BmRender_DescriptorType::StorageBuffer;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return BmRender_DescriptorType::UniformBufferDynamic;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return BmRender_DescriptorType::StorageBufferDynamic;
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			return BmRender_DescriptorType::InputAttachment;
		default:
			assert(false);
			return BmRender_DescriptorType::Sampler;
	}
}

VkIndexType IndexTypeToVk(BmRender_IndexType Type)
{
	switch (Type)
	{
		case BmRender_IndexType::Uint16:
			return VK_INDEX_TYPE_UINT16;
		case BmRender_IndexType::Uint32:
			return VK_INDEX_TYPE_UINT32;
		default:
			assert(false);
			return VK_INDEX_TYPE_UINT16;
	}
}

VkFormat BmRender_FormatToVk(BmRender_Format Format)
{
	switch (Format)
	{
		case BmRender_Format::Undefined:
			return VK_FORMAT_UNDEFINED;
		// 8-bit formats
		case BmRender_Format::R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case BmRender_Format::R8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case BmRender_Format::R8_USCALED:
			return VK_FORMAT_R8_USCALED;
		case BmRender_Format::R8_SSCALED:
			return VK_FORMAT_R8_SSCALED;
		case BmRender_Format::R8_UINT:
			return VK_FORMAT_R8_UINT;
		case BmRender_Format::R8_SINT:
			return VK_FORMAT_R8_SINT;
		case BmRender_Format::R8_SRGB:
			return VK_FORMAT_R8_SRGB;
		// 16-bit formats
		case BmRender_Format::R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case BmRender_Format::R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
		case BmRender_Format::R8G8_USCALED:
			return VK_FORMAT_R8G8_USCALED;
		case BmRender_Format::R8G8_SSCALED:
			return VK_FORMAT_R8G8_SSCALED;
		case BmRender_Format::R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case BmRender_Format::R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;
		case BmRender_Format::R8G8_SRGB:
			return VK_FORMAT_R8G8_SRGB;
		case BmRender_Format::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case BmRender_Format::R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case BmRender_Format::R16_USCALED:
			return VK_FORMAT_R16_USCALED;
		case BmRender_Format::R16_SSCALED:
			return VK_FORMAT_R16_SSCALED;
		case BmRender_Format::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case BmRender_Format::R16_SINT:
			return VK_FORMAT_R16_SINT;
		case BmRender_Format::R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;
		// 24-bit formats
		case BmRender_Format::R8G8B8_UNORM:
			return VK_FORMAT_R8G8B8_UNORM;
		case BmRender_Format::R8G8B8_SNORM:
			return VK_FORMAT_R8G8B8_SNORM;
		case BmRender_Format::R8G8B8_USCALED:
			return VK_FORMAT_R8G8B8_USCALED;
		case BmRender_Format::R8G8B8_SSCALED:
			return VK_FORMAT_R8G8B8_SSCALED;
		case BmRender_Format::R8G8B8_UINT:
			return VK_FORMAT_R8G8B8_UINT;
		case BmRender_Format::R8G8B8_SINT:
			return VK_FORMAT_R8G8B8_SINT;
		case BmRender_Format::R8G8B8_SRGB:
			return VK_FORMAT_R8G8B8_SRGB;
		case BmRender_Format::B8G8R8_UNORM:
			return VK_FORMAT_B8G8R8_UNORM;
		case BmRender_Format::B8G8R8_SNORM:
			return VK_FORMAT_B8G8R8_SNORM;
		case BmRender_Format::B8G8R8_USCALED:
			return VK_FORMAT_B8G8R8_USCALED;
		case BmRender_Format::B8G8R8_SSCALED:
			return VK_FORMAT_B8G8R8_SSCALED;
		case BmRender_Format::B8G8R8_UINT:
			return VK_FORMAT_B8G8R8_UINT;
		case BmRender_Format::B8G8R8_SINT:
			return VK_FORMAT_B8G8R8_SINT;
		case BmRender_Format::B8G8R8_SRGB:
			return VK_FORMAT_B8G8R8_SRGB;
		// 32-bit formats
		case BmRender_Format::R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case BmRender_Format::R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case BmRender_Format::R8G8B8A8_USCALED:
			return VK_FORMAT_R8G8B8A8_USCALED;
		case BmRender_Format::R8G8B8A8_SSCALED:
			return VK_FORMAT_R8G8B8A8_SSCALED;
		case BmRender_Format::R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case BmRender_Format::R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		case BmRender_Format::R8G8B8A8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case BmRender_Format::B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case BmRender_Format::B8G8R8A8_SNORM:
			return VK_FORMAT_B8G8R8A8_SNORM;
		case BmRender_Format::B8G8R8A8_USCALED:
			return VK_FORMAT_B8G8R8A8_USCALED;
		case BmRender_Format::B8G8R8A8_SSCALED:
			return VK_FORMAT_B8G8R8A8_SSCALED;
		case BmRender_Format::B8G8R8A8_UINT:
			return VK_FORMAT_B8G8R8A8_UINT;
		case BmRender_Format::B8G8R8A8_SINT:
			return VK_FORMAT_B8G8R8A8_SINT;
		case BmRender_Format::B8G8R8A8_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case BmRender_Format::R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case BmRender_Format::R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
		case BmRender_Format::R16G16_USCALED:
			return VK_FORMAT_R16G16_USCALED;
		case BmRender_Format::R16G16_SSCALED:
			return VK_FORMAT_R16G16_SSCALED;
		case BmRender_Format::R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case BmRender_Format::R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;
		case BmRender_Format::R16G16_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case BmRender_Format::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case BmRender_Format::R32_SINT:
			return VK_FORMAT_R32_SINT;
		case BmRender_Format::R32_SFLOAT:
			return VK_FORMAT_R32_SFLOAT;
		// 48-bit formats
		case BmRender_Format::R16G16B16_UNORM:
			return VK_FORMAT_R16G16B16_UNORM;
		case BmRender_Format::R16G16B16_SNORM:
			return VK_FORMAT_R16G16B16_SNORM;
		case BmRender_Format::R16G16B16_USCALED:
			return VK_FORMAT_R16G16B16_USCALED;
		case BmRender_Format::R16G16B16_SSCALED:
			return VK_FORMAT_R16G16B16_SSCALED;
		case BmRender_Format::R16G16B16_UINT:
			return VK_FORMAT_R16G16B16_UINT;
		case BmRender_Format::R16G16B16_SINT:
			return VK_FORMAT_R16G16B16_SINT;
		case BmRender_Format::R16G16B16_SFLOAT:
			return VK_FORMAT_R16G16B16_SFLOAT;
		// 64-bit formats
		case BmRender_Format::R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case BmRender_Format::R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case BmRender_Format::R16G16B16A16_USCALED:
			return VK_FORMAT_R16G16B16A16_USCALED;
		case BmRender_Format::R16G16B16A16_SSCALED:
			return VK_FORMAT_R16G16B16A16_SSCALED;
		case BmRender_Format::R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case BmRender_Format::R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
		case BmRender_Format::R16G16B16A16_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case BmRender_Format::R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case BmRender_Format::R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;
		case BmRender_Format::R32G32_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		// 96-bit formats
		case BmRender_Format::R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case BmRender_Format::R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;
		case BmRender_Format::R32G32B32_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		// 128-bit formats
		case BmRender_Format::R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case BmRender_Format::R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
		case BmRender_Format::R32G32B32A32_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		// Special formats
		case BmRender_Format::A2R10G10B10_UNORM_PACK32:
			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case BmRender_Format::A2R10G10B10_SNORM_PACK32:
			return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
		case BmRender_Format::A2R10G10B10_USCALED_PACK32:
			return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
		case BmRender_Format::A2R10G10B10_SSCALED_PACK32:
			return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
		case BmRender_Format::A2R10G10B10_UINT_PACK32:
			return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case BmRender_Format::A2R10G10B10_SINT_PACK32:
			return VK_FORMAT_A2R10G10B10_SINT_PACK32;
		case BmRender_Format::A2B10G10R10_UNORM_PACK32:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case BmRender_Format::A2B10G10R10_SNORM_PACK32:
			return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
		case BmRender_Format::A2B10G10R10_USCALED_PACK32:
			return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
		case BmRender_Format::A2B10G10R10_SSCALED_PACK32:
			return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
		case BmRender_Format::A2B10G10R10_UINT_PACK32:
			return VK_FORMAT_A2B10G10R10_UINT_PACK32;
		case BmRender_Format::A2B10G10R10_SINT_PACK32:
			return VK_FORMAT_A2B10G10R10_SINT_PACK32;
		// Depth formats
		case BmRender_Format::D16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case BmRender_Format::D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case BmRender_Format::D32_SFLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case BmRender_Format::S8_UINT:
			return VK_FORMAT_S8_UINT;
		case BmRender_Format::D16_UNORM_S8_UINT:
			return VK_FORMAT_D16_UNORM_S8_UINT;
		case BmRender_Format::D32_SFLOAT_S8_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		// Compressed formats - BC1/BC2/BC3
		case BmRender_Format::BC1_RGB_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case BmRender_Format::BC1_RGB_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case BmRender_Format::BC1_RGBA_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case BmRender_Format::BC1_RGBA_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case BmRender_Format::BC2_UNORM_BLOCK:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case BmRender_Format::BC2_SRGB_BLOCK:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case BmRender_Format::BC3_UNORM_BLOCK:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case BmRender_Format::BC3_SRGB_BLOCK:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		// Compressed formats - BC7
		case BmRender_Format::BC7_UNORM_BLOCK:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case BmRender_Format::BC7_SRGB_BLOCK:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		// Compressed formats - ETC2
		case BmRender_Format::ETC2_R8G8B8_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case BmRender_Format::ETC2_R8G8B8_SRGB_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case BmRender_Format::ETC2_R8G8B8A1_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		case BmRender_Format::ETC2_R8G8B8A1_SRGB_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
		case BmRender_Format::ETC2_R8G8B8A8_UNORM_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case BmRender_Format::ETC2_R8G8B8A8_SRGB_BLOCK:
			return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
		case BmRender_Format::B10G11R11_UFLOAT_PACK32:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case BmRender_Format::E5B9G9R9_UFLOAT_PACK32:
			return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		default:
			assert(false);
			return VK_FORMAT_UNDEFINED;
	}
}

VkAllocationCallbacks* BmRender_GetVulkanAllocator()
{
	return GetVulkanAllocator();
}

BmRender_Format VkFormatToBmRender(VkFormat Format)
{
	switch (Format)
	{
		case VK_FORMAT_UNDEFINED:
			return BmRender_Format::Undefined;
		// 8-bit formats
		case VK_FORMAT_R8_UNORM:
			return BmRender_Format::R8_UNORM;
		case VK_FORMAT_R8_SNORM:
			return BmRender_Format::R8_SNORM;
		case VK_FORMAT_R8_USCALED:
			return BmRender_Format::R8_USCALED;
		case VK_FORMAT_R8_SSCALED:
			return BmRender_Format::R8_SSCALED;
		case VK_FORMAT_R8_UINT:
			return BmRender_Format::R8_UINT;
		case VK_FORMAT_R8_SINT:
			return BmRender_Format::R8_SINT;
		case VK_FORMAT_R8_SRGB:
			return BmRender_Format::R8_SRGB;
		// 16-bit formats
		case VK_FORMAT_R8G8_UNORM:
			return BmRender_Format::R8G8_UNORM;
		case VK_FORMAT_R8G8_SNORM:
			return BmRender_Format::R8G8_SNORM;
		case VK_FORMAT_R8G8_USCALED:
			return BmRender_Format::R8G8_USCALED;
		case VK_FORMAT_R8G8_SSCALED:
			return BmRender_Format::R8G8_SSCALED;
		case VK_FORMAT_R8G8_UINT:
			return BmRender_Format::R8G8_UINT;
		case VK_FORMAT_R8G8_SINT:
			return BmRender_Format::R8G8_SINT;
		case VK_FORMAT_R8G8_SRGB:
			return BmRender_Format::R8G8_SRGB;
		case VK_FORMAT_R16_UNORM:
			return BmRender_Format::R16_UNORM;
		case VK_FORMAT_R16_SNORM:
			return BmRender_Format::R16_SNORM;
		case VK_FORMAT_R16_USCALED:
			return BmRender_Format::R16_USCALED;
		case VK_FORMAT_R16_SSCALED:
			return BmRender_Format::R16_SSCALED;
		case VK_FORMAT_R16_UINT:
			return BmRender_Format::R16_UINT;
		case VK_FORMAT_R16_SINT:
			return BmRender_Format::R16_SINT;
		case VK_FORMAT_R16_SFLOAT:
			return BmRender_Format::R16_SFLOAT;
		// 24-bit formats
		case VK_FORMAT_R8G8B8_UNORM:
			return BmRender_Format::R8G8B8_UNORM;
		case VK_FORMAT_R8G8B8_SNORM:
			return BmRender_Format::R8G8B8_SNORM;
		case VK_FORMAT_R8G8B8_USCALED:
			return BmRender_Format::R8G8B8_USCALED;
		case VK_FORMAT_R8G8B8_SSCALED:
			return BmRender_Format::R8G8B8_SSCALED;
		case VK_FORMAT_R8G8B8_UINT:
			return BmRender_Format::R8G8B8_UINT;
		case VK_FORMAT_R8G8B8_SINT:
			return BmRender_Format::R8G8B8_SINT;
		case VK_FORMAT_R8G8B8_SRGB:
			return BmRender_Format::R8G8B8_SRGB;
		case VK_FORMAT_B8G8R8_UNORM:
			return BmRender_Format::B8G8R8_UNORM;
		case VK_FORMAT_B8G8R8_SNORM:
			return BmRender_Format::B8G8R8_SNORM;
		case VK_FORMAT_B8G8R8_USCALED:
			return BmRender_Format::B8G8R8_USCALED;
		case VK_FORMAT_B8G8R8_SSCALED:
			return BmRender_Format::B8G8R8_SSCALED;
		case VK_FORMAT_B8G8R8_UINT:
			return BmRender_Format::B8G8R8_UINT;
		case VK_FORMAT_B8G8R8_SINT:
			return BmRender_Format::B8G8R8_SINT;
		case VK_FORMAT_B8G8R8_SRGB:
			return BmRender_Format::B8G8R8_SRGB;
		// 32-bit formats
		case VK_FORMAT_R8G8B8A8_UNORM:
			return BmRender_Format::R8G8B8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SNORM:
			return BmRender_Format::R8G8B8A8_SNORM;
		case VK_FORMAT_R8G8B8A8_USCALED:
			return BmRender_Format::R8G8B8A8_USCALED;
		case VK_FORMAT_R8G8B8A8_SSCALED:
			return BmRender_Format::R8G8B8A8_SSCALED;
		case VK_FORMAT_R8G8B8A8_UINT:
			return BmRender_Format::R8G8B8A8_UINT;
		case VK_FORMAT_R8G8B8A8_SINT:
			return BmRender_Format::R8G8B8A8_SINT;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return BmRender_Format::R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return BmRender_Format::B8G8R8A8_UNORM;
		case VK_FORMAT_B8G8R8A8_SNORM:
			return BmRender_Format::B8G8R8A8_SNORM;
		case VK_FORMAT_B8G8R8A8_USCALED:
			return BmRender_Format::B8G8R8A8_USCALED;
		case VK_FORMAT_B8G8R8A8_SSCALED:
			return BmRender_Format::B8G8R8A8_SSCALED;
		case VK_FORMAT_B8G8R8A8_UINT:
			return BmRender_Format::B8G8R8A8_UINT;
		case VK_FORMAT_B8G8R8A8_SINT:
			return BmRender_Format::B8G8R8A8_SINT;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return BmRender_Format::B8G8R8A8_SRGB;
		case VK_FORMAT_R16G16_UNORM:
			return BmRender_Format::R16G16_UNORM;
		case VK_FORMAT_R16G16_SNORM:
			return BmRender_Format::R16G16_SNORM;
		case VK_FORMAT_R16G16_USCALED:
			return BmRender_Format::R16G16_USCALED;
		case VK_FORMAT_R16G16_SSCALED:
			return BmRender_Format::R16G16_SSCALED;
		case VK_FORMAT_R16G16_UINT:
			return BmRender_Format::R16G16_UINT;
		case VK_FORMAT_R16G16_SINT:
			return BmRender_Format::R16G16_SINT;
		case VK_FORMAT_R16G16_SFLOAT:
			return BmRender_Format::R16G16_SFLOAT;
		case VK_FORMAT_R32_UINT:
			return BmRender_Format::R32_UINT;
		case VK_FORMAT_R32_SINT:
			return BmRender_Format::R32_SINT;
		case VK_FORMAT_R32_SFLOAT:
			return BmRender_Format::R32_SFLOAT;
		// 48-bit formats
		case VK_FORMAT_R16G16B16_UNORM:
			return BmRender_Format::R16G16B16_UNORM;
		case VK_FORMAT_R16G16B16_SNORM:
			return BmRender_Format::R16G16B16_SNORM;
		case VK_FORMAT_R16G16B16_USCALED:
			return BmRender_Format::R16G16B16_USCALED;
		case VK_FORMAT_R16G16B16_SSCALED:
			return BmRender_Format::R16G16B16_SSCALED;
		case VK_FORMAT_R16G16B16_UINT:
			return BmRender_Format::R16G16B16_UINT;
		case VK_FORMAT_R16G16B16_SINT:
			return BmRender_Format::R16G16B16_SINT;
		case VK_FORMAT_R16G16B16_SFLOAT:
			return BmRender_Format::R16G16B16_SFLOAT;
		// 64-bit formats
		case VK_FORMAT_R16G16B16A16_UNORM:
			return BmRender_Format::R16G16B16A16_UNORM;
		case VK_FORMAT_R16G16B16A16_SNORM:
			return BmRender_Format::R16G16B16A16_SNORM;
		case VK_FORMAT_R16G16B16A16_USCALED:
			return BmRender_Format::R16G16B16A16_USCALED;
		case VK_FORMAT_R16G16B16A16_SSCALED:
			return BmRender_Format::R16G16B16A16_SSCALED;
		case VK_FORMAT_R16G16B16A16_UINT:
			return BmRender_Format::R16G16B16A16_UINT;
		case VK_FORMAT_R16G16B16A16_SINT:
			return BmRender_Format::R16G16B16A16_SINT;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return BmRender_Format::R16G16B16A16_SFLOAT;
		case VK_FORMAT_R32G32_UINT:
			return BmRender_Format::R32G32_UINT;
		case VK_FORMAT_R32G32_SINT:
			return BmRender_Format::R32G32_SINT;
		case VK_FORMAT_R32G32_SFLOAT:
			return BmRender_Format::R32G32_SFLOAT;
		// 96-bit formats
		case VK_FORMAT_R32G32B32_UINT:
			return BmRender_Format::R32G32B32_UINT;
		case VK_FORMAT_R32G32B32_SINT:
			return BmRender_Format::R32G32B32_SINT;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return BmRender_Format::R32G32B32_SFLOAT;
		// 128-bit formats
		case VK_FORMAT_R32G32B32A32_UINT:
			return BmRender_Format::R32G32B32A32_UINT;
		case VK_FORMAT_R32G32B32A32_SINT:
			return BmRender_Format::R32G32B32A32_SINT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return BmRender_Format::R32G32B32A32_SFLOAT;
		// Special formats
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return BmRender_Format::A2R10G10B10_UNORM_PACK32;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
			return BmRender_Format::A2R10G10B10_SNORM_PACK32;
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
			return BmRender_Format::A2R10G10B10_USCALED_PACK32;
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
			return BmRender_Format::A2R10G10B10_SSCALED_PACK32;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			return BmRender_Format::A2R10G10B10_UINT_PACK32;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
			return BmRender_Format::A2R10G10B10_SINT_PACK32;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return BmRender_Format::A2B10G10R10_UNORM_PACK32;
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
			return BmRender_Format::A2B10G10R10_SNORM_PACK32;
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
			return BmRender_Format::A2B10G10R10_USCALED_PACK32;
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
			return BmRender_Format::A2B10G10R10_SSCALED_PACK32;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			return BmRender_Format::A2B10G10R10_UINT_PACK32;
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return BmRender_Format::A2B10G10R10_SINT_PACK32;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return BmRender_Format::B10G11R11_UFLOAT_PACK32;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return BmRender_Format::E5B9G9R9_UFLOAT_PACK32;
		// Depth formats
		case VK_FORMAT_D16_UNORM:
			return BmRender_Format::D16_UNORM;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return BmRender_Format::D24_UNORM_S8_UINT;
		case VK_FORMAT_D32_SFLOAT:
			return BmRender_Format::D32_SFLOAT;
		case VK_FORMAT_S8_UINT:
			return BmRender_Format::S8_UINT;
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return BmRender_Format::D16_UNORM_S8_UINT;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return BmRender_Format::D32_SFLOAT_S8_UINT;
		// Compressed formats - BC1/BC2/BC3
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			return BmRender_Format::BC1_RGB_UNORM_BLOCK;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			return BmRender_Format::BC1_RGB_SRGB_BLOCK;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return BmRender_Format::BC1_RGBA_UNORM_BLOCK;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			return BmRender_Format::BC1_RGBA_SRGB_BLOCK;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			return BmRender_Format::BC2_UNORM_BLOCK;
		case VK_FORMAT_BC2_SRGB_BLOCK:
			return BmRender_Format::BC2_SRGB_BLOCK;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return BmRender_Format::BC3_UNORM_BLOCK;
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return BmRender_Format::BC3_SRGB_BLOCK;
		// Compressed formats - BC7
		case VK_FORMAT_BC7_UNORM_BLOCK:
			return BmRender_Format::BC7_UNORM_BLOCK;
		case VK_FORMAT_BC7_SRGB_BLOCK:
			return BmRender_Format::BC7_SRGB_BLOCK;
		// Compressed formats - ETC2
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			return BmRender_Format::ETC2_R8G8B8_UNORM_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
			return BmRender_Format::ETC2_R8G8B8_SRGB_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			return BmRender_Format::ETC2_R8G8B8A1_UNORM_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			return BmRender_Format::ETC2_R8G8B8A1_SRGB_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			return BmRender_Format::ETC2_R8G8B8A8_UNORM_BLOCK;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			return BmRender_Format::ETC2_R8G8B8A8_SRGB_BLOCK;
		default:
			return BmRender_Format::Undefined;
	}
}

VkSurfaceFormatKHR SurfaceFormatToVk(BmRender_SurfaceFormat SurfaceFormat)
{
	VkSurfaceFormatKHR VkSurfaceFormat;
	VkSurfaceFormat.format = BmRender_FormatToVk(SurfaceFormat.Format);
	//VkSurfaceFormat.colorSpace = SurfaceFormat.ColorSpace;
	return VkSurfaceFormat;
}

BmRender_SurfaceFormat VkSurfaceFormatToBmRender(VkSurfaceFormatKHR SurfaceFormat)
{
	BmRender_SurfaceFormat BmSurfaceFormat;
	BmSurfaceFormat.Format = VkFormatToBmRender(SurfaceFormat.format);
	//BmSurfaceFormat.ColorSpace = SurfaceFormat.colorSpace;
	return BmSurfaceFormat;
}

// 2D types conversions
VkOffset2D Offset2DToVk(BmRender_Offset2D Offset)
{
	VkOffset2D VkOffset;
	VkOffset.x = Offset.X;
	VkOffset.y = Offset.Y;
	return VkOffset;
}

BmRender_Offset2D VkOffset2DToBmRender(VkOffset2D Offset)
{
	BmRender_Offset2D BmOffset;
	BmOffset.X = Offset.x;
	BmOffset.Y = Offset.y;
	return BmOffset;
}

VkExtent2D Extent2DToVk(BmRender_Dimensions Extent)
{
	VkExtent2D VkExtent;
	VkExtent.width = Extent.Width;
	VkExtent.height = Extent.Height;
	return VkExtent;
}

BmRender_Dimensions VkExtent2DToBmRender(VkExtent2D Extent)
{
	BmRender_Dimensions BmExtent;
	BmExtent.Width = Extent.width;
	BmExtent.Height = Extent.height;
	return BmExtent;
}

VkViewport ViewportToVk(BmRender_Viewport Viewport)
{
	VkViewport VkViewport;
	VkViewport.x = Viewport.X;
	VkViewport.y = Viewport.Y;
	VkViewport.width = Viewport.Width;
	VkViewport.height = Viewport.Height;
	VkViewport.minDepth = Viewport.MinDepth;
	VkViewport.maxDepth = Viewport.MaxDepth;
	return VkViewport;
}

BmRender_Viewport VkViewportToBmRender(VkViewport Viewport)
{
	BmRender_Viewport BmViewport;
	BmViewport.X = Viewport.x;
	BmViewport.Y = Viewport.y;
	BmViewport.Width = Viewport.width;
	BmViewport.Height = Viewport.height;
	BmViewport.MinDepth = Viewport.minDepth;
	BmViewport.MaxDepth = Viewport.maxDepth;
	return BmViewport;
}

VkRect2D Rect2DToVk(BmRender_Rect2D Rect)
{
	VkRect2D VkRect;
	VkRect.offset = Offset2DToVk(Rect.Offset);
	VkRect.extent = Extent2DToVk(Rect.Extent);
	return VkRect;
}

BmRender_Rect2D VkRect2DToBmRender(VkRect2D Rect)
{
	BmRender_Rect2D BmRect;
	BmRect.Offset = VkOffset2DToBmRender(Rect.offset);
	BmRect.Extent = VkExtent2DToBmRender(Rect.extent);
	return BmRect;
}

// Clear value conversions
VkClearColorValue ClearColorValueToVk(BmRender_ClearColorValue ClearValue)
{
	VkClearColorValue VkClearValue;
	VkClearValue.float32[0] = ClearValue.Float32[0];
	VkClearValue.float32[1] = ClearValue.Float32[1];
	VkClearValue.float32[2] = ClearValue.Float32[2];
	VkClearValue.float32[3] = ClearValue.Float32[3];
	return VkClearValue;
}

BmRender_ClearColorValue VkClearColorValueToBmRender(VkClearColorValue ClearValue)
{
	BmRender_ClearColorValue BmClearValue;
	BmClearValue.Float32[0] = ClearValue.float32[0];
	BmClearValue.Float32[1] = ClearValue.float32[1];
	BmClearValue.Float32[2] = ClearValue.float32[2];
	BmClearValue.Float32[3] = ClearValue.float32[3];
	return BmClearValue;
}

VkClearDepthStencilValue ClearDepthStencilValueToVk(BmRender_ClearDepthStencilValue ClearValue)
{
	VkClearDepthStencilValue VkClearValue;
	VkClearValue.depth = ClearValue.Depth;
	VkClearValue.stencil = ClearValue.Stencil;
	return VkClearValue;
}

BmRender_ClearDepthStencilValue VkClearDepthStencilValueToBmRender(VkClearDepthStencilValue ClearValue)
{
	BmRender_ClearDepthStencilValue BmClearValue;
	BmClearValue.Depth = ClearValue.depth;
	BmClearValue.Stencil = ClearValue.stencil;
	return BmClearValue;
}

// Descriptor pool size conversion
VkDescriptorPoolSize DescriptorPoolSizeToVk(BmRender_DescriptorPoolSize PoolSize)
{
	VkDescriptorPoolSize VkPoolSize;
	VkPoolSize.type = DescriptorTypeToVk(PoolSize.Type);
	VkPoolSize.descriptorCount = PoolSize.DescriptorCount;
	return VkPoolSize;
}

BmRender_DescriptorPoolSize VkDescriptorPoolSizeToBmRender(VkDescriptorPoolSize PoolSize)
{
	BmRender_DescriptorPoolSize BmPoolSize;
	BmPoolSize.Type = VkDescriptorTypeToBmRender(PoolSize.type);
	BmPoolSize.DescriptorCount = PoolSize.descriptorCount;
	return BmPoolSize;
}

// Shader stage flags conversion
VkShaderStageFlags ShaderStageFlagsToVk(BmRender_DescriptorShaderStage StageFlags)
{
	VkShaderStageFlags VkFlags = 0;
	u64 FlagsValue = static_cast<u64>(StageFlags);
	if ((FlagsValue & static_cast<u64>(BmRender_DescriptorShaderStage::Vertex)) != 0)
		VkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_DescriptorShaderStage::Fragment)) != 0)
		VkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_DescriptorShaderStage::Compute)) != 0)
		VkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
	return VkFlags;
}

BmRender_DescriptorShaderStage VkShaderStageFlagsToBmRender(VkShaderStageFlags StageFlags)
{
	BmRender_DescriptorShaderStage BmFlags = BmRender_DescriptorShaderStage::None;
	if (StageFlags & VK_SHADER_STAGE_VERTEX_BIT)
		BmFlags = static_cast<BmRender_DescriptorShaderStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_DescriptorShaderStage::Vertex));
	if (StageFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
		BmFlags = static_cast<BmRender_DescriptorShaderStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_DescriptorShaderStage::Fragment));
	if (StageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
		BmFlags = static_cast<BmRender_DescriptorShaderStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_DescriptorShaderStage::Compute));
	return BmFlags;
}

VkPipelineStageFlags PipelineStageFlagsToVk(BmRender_PipelineSyncStage StageFlags)
{
	VkPipelineStageFlags VkFlags = 0;
	u64 FlagsValue = static_cast<u64>(StageFlags);
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::TopOfPipe)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::VertexShader)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::FragmentShader)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::ColorAttachmentOutput)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::ComputeShader)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if ((FlagsValue & static_cast<u64>(BmRender_PipelineSyncStage::BottomOfPipe)) != 0)
		VkFlags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	return VkFlags;
}

BmRender_PipelineSyncStage VkPipelineStageFlagsToBmRender(VkPipelineStageFlags StageFlags)
{
	BmRender_PipelineSyncStage BmFlags = BmRender_PipelineSyncStage::None;
	if (StageFlags & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::TopOfPipe));
	if (StageFlags & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::VertexShader));
	if (StageFlags & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::FragmentShader));
	if (StageFlags & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::ColorAttachmentOutput));
	if (StageFlags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::ComputeShader));
	if (StageFlags & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
		BmFlags = static_cast<BmRender_PipelineSyncStage>(static_cast<u64>(BmFlags) | static_cast<u64>(BmRender_PipelineSyncStage::BottomOfPipe));
	return BmFlags;
}

VkPolygonMode PolygonModeToVk(BmRender_PolygonMode Mode)
{
	switch (Mode)
	{
		case BmRender_PolygonMode::Fill:
			return VK_POLYGON_MODE_FILL;
		case BmRender_PolygonMode::Line:
			return VK_POLYGON_MODE_LINE;
		case BmRender_PolygonMode::Point:
			return VK_POLYGON_MODE_POINT;
		default:
			assert(false);
			return VK_POLYGON_MODE_FILL;
	}
}

VkCullModeFlags CullModeFlagsToVk(BmRender_CullModeFlags Flags)
{
	if (Flags == BmRender_CullModeFlags::None)
		return VK_CULL_MODE_NONE;
	
	VkCullModeFlags VkFlags = 0;
	u32 FlagsValue = static_cast<u32>(Flags);
	if ((FlagsValue & static_cast<u32>(BmRender_CullModeFlags::Front)) != 0)
		VkFlags |= VK_CULL_MODE_FRONT_BIT;
	if ((FlagsValue & static_cast<u32>(BmRender_CullModeFlags::Back)) != 0)
		VkFlags |= VK_CULL_MODE_BACK_BIT;
	return VkFlags;
}

VkFrontFace FrontFaceToVk(BmRender_FrontFace Face)
{
	switch (Face)
	{
		case BmRender_FrontFace::CounterClockwise:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		case BmRender_FrontFace::Clockwise:
			return VK_FRONT_FACE_CLOCKWISE;
		default:
			assert(false);
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

VkColorComponentFlags ColorComponentFlagsToVk(BmRender_ColorComponentFlags Flags)
{
	VkColorComponentFlags VkFlags = 0;
	u32 FlagsValue = static_cast<u32>(Flags);
	if ((FlagsValue & static_cast<u32>(BmRender_ColorComponentFlags::R)) != 0)
		VkFlags |= VK_COLOR_COMPONENT_R_BIT;
	if ((FlagsValue & static_cast<u32>(BmRender_ColorComponentFlags::G)) != 0)
		VkFlags |= VK_COLOR_COMPONENT_G_BIT;
	if ((FlagsValue & static_cast<u32>(BmRender_ColorComponentFlags::B)) != 0)
		VkFlags |= VK_COLOR_COMPONENT_B_BIT;
	if ((FlagsValue & static_cast<u32>(BmRender_ColorComponentFlags::A)) != 0)
		VkFlags |= VK_COLOR_COMPONENT_A_BIT;
	return VkFlags;
}

VkBlendFactor BlendFactorToVk(BmRender_BlendFactor Factor)
{
	switch (Factor)
	{
		case BmRender_BlendFactor::Zero:
			return VK_BLEND_FACTOR_ZERO;
		case BmRender_BlendFactor::One:
			return VK_BLEND_FACTOR_ONE;
		case BmRender_BlendFactor::SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case BmRender_BlendFactor::DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case BmRender_BlendFactor::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case BmRender_BlendFactor::DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case BmRender_BlendFactor::OneMinusSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BmRender_BlendFactor::OneMinusDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BmRender_BlendFactor::OneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BmRender_BlendFactor::OneMinusDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		default:
			assert(false);
			return VK_BLEND_FACTOR_SRC_ALPHA;
	}
}

VkBlendOp BlendOpToVk(BmRender_BlendOp Op)
{
	switch (Op)
	{
		case BmRender_BlendOp::Add:
			return VK_BLEND_OP_ADD;
		case BmRender_BlendOp::Subtract:
			return VK_BLEND_OP_SUBTRACT;
		case BmRender_BlendOp::ReverseSubtract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BmRender_BlendOp::Min:
			return VK_BLEND_OP_MIN;
		case BmRender_BlendOp::Max:
			return VK_BLEND_OP_MAX;
		default:
			assert(false);
			return VK_BLEND_OP_ADD;
	}
}

VkPrimitiveTopology PrimitiveTopologyToVk(BmRender_PrimitiveTopology Topology)
{
	switch (Topology)
	{
		case BmRender_PrimitiveTopology::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case BmRender_PrimitiveTopology::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case BmRender_PrimitiveTopology::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case BmRender_PrimitiveTopology::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case BmRender_PrimitiveTopology::TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case BmRender_PrimitiveTopology::TriangleFan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case BmRender_PrimitiveTopology::LineListWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		case BmRender_PrimitiveTopology::LineStripWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		case BmRender_PrimitiveTopology::TriangleListWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		case BmRender_PrimitiveTopology::TriangleStripWithAdjacency:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		case BmRender_PrimitiveTopology::PatchList:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		default:
			assert(false);
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkSampleCountFlagBits SampleCountToVk(BmRender_SampleCount Count)
{
	switch (Count)
	{
		case BmRender_SampleCount::Count1:
			return VK_SAMPLE_COUNT_1_BIT;
		case BmRender_SampleCount::Count2:
			return VK_SAMPLE_COUNT_2_BIT;
		case BmRender_SampleCount::Count4:
			return VK_SAMPLE_COUNT_4_BIT;
		case BmRender_SampleCount::Count8:
			return VK_SAMPLE_COUNT_8_BIT;
		case BmRender_SampleCount::Count16:
			return VK_SAMPLE_COUNT_16_BIT;
		case BmRender_SampleCount::Count32:
			return VK_SAMPLE_COUNT_32_BIT;
		case BmRender_SampleCount::Count64:
			return VK_SAMPLE_COUNT_64_BIT;
		default:
			assert(false);
			return VK_SAMPLE_COUNT_1_BIT;
	}
}

VkPipelineRasterizationStateCreateInfo RasterizationStateToVk(const BmRender_RasterizationState& State)
{
	VkPipelineRasterizationStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	VkState.depthClampEnable = State.DepthClampEnable ? VK_TRUE : VK_FALSE;
	VkState.rasterizerDiscardEnable = State.RasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
	VkState.polygonMode = PolygonModeToVk(State.PolygonMode);
	VkState.lineWidth = State.LineWidth;
	VkState.cullMode = CullModeFlagsToVk(State.CullMode);
	VkState.frontFace = FrontFaceToVk(State.FrontFace);
	VkState.depthBiasEnable = State.DepthBiasEnable ? VK_TRUE : VK_FALSE;
	return VkState;
}

VkPipelineColorBlendAttachmentState ColorBlendAttachmentToVk(const BmRender_ColorBlendAttachment& Attachment)
{
	VkPipelineColorBlendAttachmentState VkAttachment = {};
	VkAttachment.colorWriteMask = ColorComponentFlagsToVk(Attachment.ColorWriteMask);
	VkAttachment.blendEnable = Attachment.BlendEnable ? VK_TRUE : VK_FALSE;
	VkAttachment.srcColorBlendFactor = BlendFactorToVk(Attachment.SrcColorBlendFactor);
	VkAttachment.dstColorBlendFactor = BlendFactorToVk(Attachment.DstColorBlendFactor);
	VkAttachment.colorBlendOp = BlendOpToVk(Attachment.ColorBlendOp);
	VkAttachment.srcAlphaBlendFactor = BlendFactorToVk(Attachment.SrcAlphaBlendFactor);
	VkAttachment.dstAlphaBlendFactor = BlendFactorToVk(Attachment.DstAlphaBlendFactor);
	VkAttachment.alphaBlendOp = BlendOpToVk(Attachment.AlphaBlendOp);
	return VkAttachment;
}

VkPipelineColorBlendStateCreateInfo ColorBlendStateToVk(const BmRender_ColorBlendState& State)
{
	VkPipelineColorBlendStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkState.logicOpEnable = State.LogicOpEnable ? VK_TRUE : VK_FALSE;
	VkState.attachmentCount = State.AttachmentCount;
	return VkState;
}

VkPipelineDepthStencilStateCreateInfo DepthStencilStateToVk(const BmRender_DepthStencilState& State)
{
	VkPipelineDepthStencilStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	VkState.depthTestEnable = State.DepthTestEnable ? VK_TRUE : VK_FALSE;
	VkState.depthWriteEnable = State.DepthWriteEnable ? VK_TRUE : VK_FALSE;
	VkState.depthCompareOp = CompareOpToVk(State.DepthCompareOp);
	VkState.depthBoundsTestEnable = State.DepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
	VkState.stencilTestEnable = State.StencilTestEnable ? VK_TRUE : VK_FALSE;
	return VkState;
}

VkPipelineMultisampleStateCreateInfo MultisampleStateToVk(const BmRender_MultisampleState& State)
{
	VkPipelineMultisampleStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	VkState.sampleShadingEnable = State.SampleShadingEnable ? VK_TRUE : VK_FALSE;
	return VkState;
}

VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateToVk(const BmRender_InputAssemblyState& State)
{
	VkPipelineInputAssemblyStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	VkState.topology = PrimitiveTopologyToVk(State.Topology);
	VkState.primitiveRestartEnable = State.PrimitiveRestartEnable ? VK_TRUE : VK_FALSE;
	return VkState;
}

VkPipelineViewportStateCreateInfo ViewportStateToVk(const BmRender_ViewportState& State)
{
	VkPipelineViewportStateCreateInfo VkState = {};
	VkState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	VkState.viewportCount = State.ViewportCount;
	VkState.scissorCount = State.ScissorCount;
	return VkState;
}

bool CheckFormats(VkPhysicalDevice PhDevice)
{
	const u32 FormatPrioritySize = 3;
	BmRender_Format FormatPriority[FormatPrioritySize] = { BmRender_Format::D32_SFLOAT_S8_UINT, BmRender_Format::D32_SFLOAT, BmRender_Format::D24_UNORM_S8_UINT };

	bool IsSupportedFormatFound = false;
	for (u32 i = 0; i < FormatPrioritySize; ++i)
	{
		VkFormat FormatToCheck = BmRender_FormatToVk(FormatPriority[i]);
		if (CheckFormatSupport(PhDevice, FormatToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			IsSupportedFormatFound = true;
			break;
		}

		RenderLog(LogType::Warning, "Format %d is not supported", FormatToCheck);
	}

	if (!IsSupportedFormatFound)
	{
		RenderLog(LogType::Error, "No supported format found");
		return false;
	}

	return true;
}


u32 BmRender_GetFormatAlignment(BmRender_Format Format)
{
	VkFormat VkFormatValue = BmRender_FormatToVk(Format);
	switch (VkFormatValue)
	{
		// 8-bit formats - 1 byte alignment
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_S8_UINT:
			return 1;

			// 16-bit formats - 2 byte alignment
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_D16_UNORM:
			return 2;

			// 24-bit formats - 4 byte alignment (3 bytes don't align well)
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 4;

			// 32-bit formats - 4 byte alignment
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return 4;

			// 64-bit formats - 8 byte alignment
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;

			// 96-bit formats - 16 byte alignment
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 16;

			// 128-bit formats - 16 byte alignment
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return 16;

			// Compressed formats - Block alignment
			// BC1/BC2/BC3 - 8 byte blocks
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return 8;

			// BC7 - 16 byte blocks
		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
			return 16;

			// ETC2 - 8 byte blocks
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			return 8;

			// ASTC - 16 byte blocks
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return 16;

			// PVRTC - 32 byte blocks
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			return 32;

			// Default case - assume 4 byte alignment for safety
		default:
			return 4;
	}
}

u32 CalculateFormatSize(VkFormat Format)
{
	switch (Format)
	{
		case VK_FORMAT_R32_SFLOAT: return 4;
		case VK_FORMAT_R32G32_SFLOAT: return 8;
		case VK_FORMAT_R32G32B32_SFLOAT: return 12;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
		case VK_FORMAT_R32_UINT: return 4;
		default:
			assert(false);
			return 4;
	}
}

VkDescriptorPoolCreateFlags DescriptorPoolTypeToVkFlags(BmRender_DescriptorPoolType Type)
{
	VkDescriptorPoolCreateFlags flags = 0;
	if ((u32)Type & (u32)BmRender_DescriptorPoolType::UpdateAfterBind) flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	if ((u32)Type & (u32)BmRender_DescriptorPoolType::CreateFree) flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	return flags;
}

VkMemoryPropertyFlags MemoryPropertyFlagToVkFlags(MemoryPropertyFlag Flag)
{
	switch (Flag)
	{
		case MemoryPropertyFlag::GPULocal:
			return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		case MemoryPropertyFlag::HostCompatible:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		default:
			assert(false);
			return 0;
	}
}

s32 GetQueueFamilyIndexFromQueueType(BmRender_QueueType QueueType, const PhysicalDeviceIndices& Indices)
{
	bool NeedsGraphics = ((u32)QueueType & (u32)BmRender_QueueType::Graphic) != 0;
	bool NeedsTransfer = ((u32)QueueType & (u32)BmRender_QueueType::Transfer) != 0;

	if (NeedsTransfer && !NeedsGraphics)
	{
		return Indices.TransferFamily;
	}
	else if (NeedsGraphics)
	{
		return Indices.GraphicsFamily;
	}

	return -1;
}

void RenderLog(LogType logType, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	RenderLog(logType, format, args);
	va_end(args);
}

void RenderLog(LogType LogType, const char* Format, va_list Args)
{
	switch (LogType)
	{
		case LogType::Error:
		{
			vprintf("\033[31;5mError: ", Args);
			va_list ArgsCopy;
			va_copy(ArgsCopy, Args);
			vprintf(Format, ArgsCopy);
			va_end(ArgsCopy);
			vprintf("\n\033[m", Args);
			//assert(false);
			break;
		}
		case LogType::Warning:
		{
			vprintf("\033[33;5mWarning: ", Args);
			va_list ArgsCopy;
			va_copy(ArgsCopy, Args);
			vprintf(Format, ArgsCopy);
			va_end(ArgsCopy);
			vprintf("\n\033[m", Args);
			break;
		}
		case LogType::Info:
		{
			vprintf("Info: ", Args);
			va_list ArgsCopy;
			va_copy(ArgsCopy, Args);
			vprintf(Format, ArgsCopy);
			va_end(ArgsCopy);
			vprintf("\n", Args);
			break;
		}
	}
}

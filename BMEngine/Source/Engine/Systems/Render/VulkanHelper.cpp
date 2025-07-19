#include "VulkanHelper.h"

#include <cassert>

#include <glm/glm.hpp>

#include "Util/Util.h"

namespace VulkanHelper
{
	Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		u32 Count;
		VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Count, nullptr));

		auto Data = Memory::FrameArray<VkSurfaceFormatKHR>::Create(Count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		return Data;
	}

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

	u64 CalculateBufferAlignedSize(VkDevice Device, VkBuffer Buffer, u64 BufferSize)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

		u32 Padding = 0;
		if (BufferSize % MemoryRequirements.alignment != 0)
		{
			Padding = MemoryRequirements.alignment - (BufferSize % MemoryRequirements.alignment);
		}

		return BufferSize + Padding;
	}

	u64 CalculateImageAlignedSize(VkDevice Device, VkImage Image, u64 ImageSize)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

		u32 Padding = 0;
		if (ImageSize % MemoryRequirements.alignment != 0)
		{
			Padding = IMAGE_ALIGNMENT - (ImageSize % MemoryRequirements.alignment);
		}

		return ImageSize + Padding;
	}

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag)
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = Size;
		BufferInfo.usage = Flag;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VULKAN_CHECK_RESULT(vkCreateBuffer(Device, &BufferInfo, nullptr, &Buffer));
		return Buffer;
	}

	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer, MemoryPropertyFlag Properties)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		DeviceMemoryAllocResult Result;
		VULKAN_CHECK_RESULT(vkAllocateMemory(Device, &MemoryAllocInfo, nullptr, &Result.Memory));
		Result.Alignment = MemoryRequirements.alignment;
		Result.Size = MemoryRequirements.size;

		return Result;
	}

	DeviceMemoryAllocResult AllocateDeviceMemory(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkImage Image, MemoryPropertyFlag Properties)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		DeviceMemoryAllocResult Result;
		VULKAN_CHECK_RESULT(vkAllocateMemory(Device, &MemoryAllocInfo, nullptr, &Result.Memory));
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

	Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties()
	{
		u32 Count;
		VULKAN_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &Count, nullptr));

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateInstanceExtensionProperties(nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties()
	{
		u32 Count;
		VULKAN_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&Count, nullptr));

		auto Data = Memory::FrameArray<VkLayerProperties>::Create(Count);
		vkEnumerateInstanceLayerProperties(&Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** RequiredInstanceExtensions, u32 RequiredExtensionsCount,
		const char** ValidationExtensions, u32 ValidationExtensionsCount)
	{
		auto Data = Memory::FrameArray<const char*>::Create(RequiredExtensionsCount + ValidationExtensionsCount);

		for (u32 i = 0; i < RequiredExtensionsCount; ++i)
		{
			Data[i] = RequiredInstanceExtensions[i];
			Util::RenderLog(Util::BMRVkLogType_Info, "Requested %s extension", Data[i]);
		}

		for (u32 i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data[i + RequiredExtensionsCount] = ValidationExtensions[i];
			Util::RenderLog(Util::BMRVkLogType_Info, "Requested %s extension", Data[i + RequiredExtensionsCount]);
		}

		return Data;
	}

	Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance)
	{
		u32 Count;
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, nullptr);

		auto Data = Memory::FrameArray<VkPhysicalDevice>::Create(Count);
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		VULKAN_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, nullptr));

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, nullptr);

		auto Data = Memory::FrameArray<VkQueueFamilyProperties>::Create(Count);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, Data.Pointer.Data);

		return Data;
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
	}

	void PrintDeviceData(VkPhysicalDeviceProperties* DeviceProperties, VkPhysicalDeviceFeatures* AvailableFeatures)
	{
		Util::RenderLog(Util::BMRVkLogType_Info,
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

		Util::RenderLog(Util::BMRVkLogType_Info,
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

		Util::RenderLog(Util::BMRVkLogType_Info,
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
		Memory::FrameArray<VkPresentModeKHR> PresentModes = GetAvailablePresentModes(PhysicalDevice, Surface);

		VkPresentModeKHR Mode = VK_PRESENT_MODE_FIFO_KHR;
		for (u32 i = 0; i < PresentModes.Count; ++i)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				Mode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		// Has to be present by spec
		if (Mode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Using default VK_PRESENT_MODE_FIFO_KHR");
		}

		return Mode;
	}

	Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface)
	{
		u32 Count;
		VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, nullptr));

		auto Data = Memory::FrameArray<VkPresentModeKHR>::Create(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		for (u32 i = 0; i < Count; ++i)
		{
			Util::RenderLog(Util::BMRVkLogType_Info, "Present mode %d is available", Data[i]);
		}

		return Data;
	}

	Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice,
		VkSwapchainKHR VulkanSwapchain)
	{
		u32 Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		auto Data = Memory::FrameArray<VkImage>::Create(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Data.Pointer.Data);

		return Data;
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
				Util::RenderLog(Util::BMRVkLogType_Error, "Extension %s unsupported", RequiredExtensions[i]);
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
				Util::RenderLog(Util::BMRVkLogType_Error, "Validation layer %s unsupported", ValidationLayersToCheck[i]);
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
				Util::RenderLog(Util::BMRVkLogType_Error, "extension %s unsupported", ExtensionsToCheck[i]);
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
			Util::RenderLog(Util::BMRVkLogType_Error, "CreateMessengerFunc is nullptr");
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
			Util::RenderLog(Util::BMRVkLogType_Error, "DestroyMessengerFunc is nullptr");
			return false;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		[[maybe_unused]] void* UserData)
	{
		if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, CallbackData->pMessage);
		}
		else if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, CallbackData->pMessage);
		}
		else
		{
			Util::RenderLog(Util::BMRVkLogType_Info, CallbackData->pMessage);
		}

		return VK_FALSE;
	}

	VkPipeline BatchPipelineCreation(VkDevice Device, const Shader* Shaders, u32 ShadersCount,
		const BMRVertexInputBinding* VertexInputBinding, u32 VertexInputBindingCount,
		const PipelineSettings* Settings, const PipelineResourceInfo* ResourceInfo)
	{
		VkViewport Viewport = Settings->Viewport;
		Viewport.width = Settings->Extent.width;
		Viewport.height = Settings->Extent.height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		VkRect2D Scissor = Settings->Scissor;
		Scissor.extent.width = Settings->Extent.width;
		Scissor.extent.height = Settings->Extent.height;
		Scissor.offset = {};

		VkPipelineViewportStateCreateInfo ViewportState = Settings->ViewportState;
		ViewportState.pViewports = &Viewport;
		ViewportState.pScissors = &Scissor;

		Util::RenderLog(Util::BMRVkLogType_Info,
			"CREATING PIPELINE "
			"Extent - Width: %d, Height: %d\n"
			"DepthClampEnable: %d, RasterizerDiscardEnable: %d\n"
			"PolygonMode: %d, LineWidth: %f, CullMode: %d, FrontFace: %d, DepthBiasEnable: %d\n"
			"LogicOpEnable: %d, AttachmentCount: %d, ColorWriteMask: %d\n"
			"BlendEnable: %d, SrcColorBlendFactor: %d, DstColorBlendFactor: %d, ColorBlendOp: %d\n"
			"SrcAlphaBlendFactor: %d, DstAlphaBlendFactor: %d, AlphaBlendOp: %d\n"
			"DepthTestEnable: %d, DepthWriteEnable: %d, DepthCompareOp: %d\n"
			"DepthBoundsTestEnable: %d, StencilTestEnable: %d",
			Settings->Extent.width, Settings->Extent.height,
			Settings->RasterizationState.depthClampEnable, Settings->RasterizationState.rasterizerDiscardEnable,
			Settings->RasterizationState.polygonMode, Settings->RasterizationState.lineWidth, Settings->RasterizationState.cullMode,
			Settings->RasterizationState.frontFace, Settings->RasterizationState.depthBiasEnable,
			Settings->ColorBlendState.logicOpEnable, Settings->ColorBlendState.attachmentCount, Settings->ColorBlendAttachment.colorWriteMask,
			Settings->ColorBlendAttachment.blendEnable, Settings->ColorBlendAttachment.srcColorBlendFactor,
			Settings->ColorBlendAttachment.dstColorBlendFactor, Settings->ColorBlendAttachment.colorBlendOp,
			Settings->ColorBlendAttachment.srcAlphaBlendFactor, Settings->ColorBlendAttachment.dstAlphaBlendFactor,
			Settings->ColorBlendAttachment.alphaBlendOp,
			Settings->DepthStencilState.depthTestEnable, Settings->DepthStencilState.depthWriteEnable,
			Settings->DepthStencilState.depthCompareOp, Settings->DepthStencilState.depthBoundsTestEnable,
			Settings->DepthStencilState.stencilTestEnable
		);

		Util::RenderLog(Util::BMRVkLogType_Info, "Creating VkPipelineShaderStageCreateInfo, ShadersCount: %u", ShadersCount);

		auto ShaderStageCreateInfos = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineShaderStageCreateInfo>(ShadersCount);
		for (u32 i = 0; i < ShadersCount; ++i)
		{
			Util::RenderLog(Util::BMRVkLogType_Info, "Shader #%d, Stage: %d", i, Shaders[i].Stage);

			VkPipelineShaderStageCreateInfo* ShaderStageCreateInfo = ShaderStageCreateInfos + i;
			*ShaderStageCreateInfo = { };
			ShaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			ShaderStageCreateInfo->stage = Shaders[i].Stage;
			ShaderStageCreateInfo->pName = "main";

			VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
			ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			ShaderModuleCreateInfo.codeSize = Shaders[i].CodeSize;
			ShaderModuleCreateInfo.pCode = (const u32*)Shaders[i].Code;
			VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShaderStageCreateInfo->module));
		}

		auto VertexInputBindings = Memory::MemoryManagementSystem::FrameAlloc<VkVertexInputBindingDescription>(VertexInputBindingCount);
		auto VertexInputAttributes = Memory::MemoryManagementSystem::FrameAlloc<VkVertexInputAttributeDescription>(VertexInputBindingCount * MAX_VERTEX_INPUTS_ATTRIBUTES);
		u32 VertexInputAttributesIndex = 0;

		Util::RenderLog(Util::BMRVkLogType_Info, "Creating vertex input, VertexInputBindingCount: %d", VertexInputBindingCount);

		for (u32 BindingIndex = 0; BindingIndex < VertexInputBindingCount; ++BindingIndex)
		{
			const BMRVertexInputBinding* BMRBinding = VertexInputBinding + BindingIndex;
			VkVertexInputBindingDescription* VertexInputBinding = VertexInputBindings + BindingIndex;

			VertexInputBinding->binding = BindingIndex;
			VertexInputBinding->inputRate = BMRBinding->InputRate;
			VertexInputBinding->stride = BMRBinding->Stride;

			Util::RenderLog(Util::BMRVkLogType_Info, "Initialized VkVertexInputBindingDescription, BindingName: %s, "
				"BindingIndex: %d, VkInputRate: %d, Stride: %d, InputAttributesCount: %d",
				BMRBinding->VertexInputBindingName, VertexInputBinding->binding, VertexInputBinding->inputRate,
				VertexInputBinding->stride, BMRBinding->InputAttributesCount);

			for (u32 CurrentAttributeIndex = 0; CurrentAttributeIndex < BMRBinding->InputAttributesCount; ++CurrentAttributeIndex)
			{
				const VertexInputAttribute* BMRAttribute = BMRBinding->InputAttributes + CurrentAttributeIndex;
				VkVertexInputAttributeDescription* VertexInputAttribute = VertexInputAttributes + VertexInputAttributesIndex;

				VertexInputAttribute->binding = BindingIndex;
				VertexInputAttribute->location = VertexInputAttributesIndex;
				VertexInputAttribute->format = BMRAttribute->Format;
				VertexInputAttribute->offset = BMRAttribute->AttributeOffset;

				Util::RenderLog(Util::BMRVkLogType_Info, "Initialized VkVertexInputAttributeDescription, "
					"AttributeName: %s, BindingIndex: %d, Location: %d, VkFormat: %d, Offset: %d, Index in creation array: %d",
					BMRAttribute->VertexInputAttributeName, BindingIndex, CurrentAttributeIndex,
					VertexInputAttribute->format, VertexInputAttribute->offset, VertexInputAttributesIndex);

				++VertexInputAttributesIndex;
			}
		}
		auto VertexInputInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineVertexInputStateCreateInfo>();
		*VertexInputInfo = { };
		VertexInputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputInfo->vertexBindingDescriptionCount = VertexInputBindingCount;
		VertexInputInfo->pVertexBindingDescriptions = VertexInputBindings;
		VertexInputInfo->vertexAttributeDescriptionCount = VertexInputAttributesIndex;
		VertexInputInfo->pVertexAttributeDescriptions = VertexInputAttributes;

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = ResourceInfo->PipelineAttachmentData.ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = ResourceInfo->PipelineAttachmentData.ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;

		auto PipelineCreateInfo = Memory::MemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>();
		PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfo->stageCount = ShadersCount;
		PipelineCreateInfo->pStages = ShaderStageCreateInfos;
		PipelineCreateInfo->pVertexInputState = VertexInputInfo;
		PipelineCreateInfo->pInputAssemblyState = &Settings->InputAssemblyState;
		PipelineCreateInfo->pViewportState = &ViewportState;
		PipelineCreateInfo->pDynamicState = nullptr;
		PipelineCreateInfo->pRasterizationState = &Settings->RasterizationState;
		PipelineCreateInfo->pMultisampleState = &Settings->MultisampleState;
		PipelineCreateInfo->pColorBlendState = &Settings->ColorBlendState;
		PipelineCreateInfo->pDepthStencilState = &Settings->DepthStencilState;
		PipelineCreateInfo->layout = ResourceInfo->PipelineLayout;
		PipelineCreateInfo->renderPass = nullptr;
		PipelineCreateInfo->subpass = 0;
		PipelineCreateInfo->pNext = &RenderingInfo;

		PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
		PipelineCreateInfo->basePipelineIndex = -1;

		VkPipeline Pipeline;
		VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

		for (u32 j = 0; j < ShadersCount; ++j)
		{
			vkDestroyShaderModule(Device, ShaderStageCreateInfos[j].module, nullptr);
		}

		return Pipeline;
	}

	bool CheckFormats(VkPhysicalDevice PhDevice)
	{
		const u32 FormatPrioritySize = 3;
		VkFormat FormatPriority[FormatPrioritySize] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

		bool IsSupportedFormatFound = false;
		for (u32 i = 0; i < FormatPrioritySize; ++i)
		{
			VkFormat FormatToCheck = FormatPriority[i];
			if (VulkanHelper::CheckFormatSupport(PhDevice, FormatToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			{
				IsSupportedFormatFound = true;
				break;
			}

			Util::RenderLog(Util::BMRVkLogType_Warning, "Format %d is not supported", FormatToCheck);
		}

		if (!IsSupportedFormatFound)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "No supported format found");
			return false;
		}

		return true;
	}

	VkBool32 ParseBool(const char* Value, u32 Length)
	{
		if (Length == 0) return VK_FALSE;
		
		if (strncmp(Value, "true", 4) == 0) return VK_TRUE;
		if (Length == 1 && Value[0] == '1') return VK_TRUE;
		if (strncmp(Value, "yes", 3) == 0) return VK_TRUE;
		if (strncmp(Value, "on", 2) == 0) return VK_TRUE;
		
		if (strncmp(Value, "false", 5) == 0) return VK_FALSE;
		if (Length == 1 && Value[0] == '0') return VK_FALSE;
		if (strncmp(Value, "no", 2) == 0) return VK_FALSE;
		if (strncmp(Value, "off", 3) == 0) return VK_FALSE;
		
		return VK_FALSE;
	}

	VkPolygonMode ParsePolygonMode(const char* Value, u32 Length)
	{
		if (strncmp(Value, "fill", 4) == 0 || strncmp(Value, "FILL", 4) == 0) return VK_POLYGON_MODE_FILL;
		if (strncmp(Value, "line", 4) == 0 || strncmp(Value, "LINE", 4) == 0) return VK_POLYGON_MODE_LINE;
		if (strncmp(Value, "point", 5) == 0 || strncmp(Value, "POINT", 5) == 0) return VK_POLYGON_MODE_POINT;
		
		return VK_POLYGON_MODE_FILL;
	}

	VkCullModeFlags ParseCullMode(const char* Value, u32 Length)
	{
		if (strncmp(Value, "none", 4) == 0 || strncmp(Value, "NONE", 4) == 0) return VK_CULL_MODE_NONE;
		if (strncmp(Value, "back", 4) == 0 || strncmp(Value, "BACK", 4) == 0) return VK_CULL_MODE_BACK_BIT;
		if (strncmp(Value, "front", 5) == 0 || strncmp(Value, "FRONT", 5) == 0) return VK_CULL_MODE_FRONT_BIT;
		if (strncmp(Value, "front_back", 9) == 0 || strncmp(Value, "FRONT_BACK", 9) == 0) return VK_CULL_MODE_FRONT_AND_BACK;
		
		return VK_CULL_MODE_BACK_BIT;
	}

	VkFrontFace ParseFrontFace(const char* Value, u32 Length)
	{
		if (strncmp(Value, "counter_clockwise", 15) == 0 || strncmp(Value, "COUNTER_CLOCKWISE", 15) == 0) return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		if (strncmp(Value, "clockwise", 10) == 0 || strncmp(Value, "CLOCKWISE", 10) == 0) return VK_FRONT_FACE_CLOCKWISE;
		
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	VkColorComponentFlags ParseColorWriteMask(const char* Value, u32 Length)
	{
		VkColorComponentFlags Flags = 0;
		
		for (u32 i = 0; i < Length; ++i)
		{
			if (Value[i] == 'R' || Value[i] == 'r') Flags |= VK_COLOR_COMPONENT_R_BIT;
			if (Value[i] == 'G' || Value[i] == 'g') Flags |= VK_COLOR_COMPONENT_G_BIT;
			if (Value[i] == 'B' || Value[i] == 'b') Flags |= VK_COLOR_COMPONENT_B_BIT;
			if (Value[i] == 'A' || Value[i] == 'a') Flags |= VK_COLOR_COMPONENT_A_BIT;
		}
		
		if (Flags == 0) Flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		
		return Flags;
	}

	VkBlendFactor ParseBlendFactor(const char* Value, u32 Length)
	{
		if (strncmp(Value, "one_minus_src_alpha", 18) == 0 || strncmp(Value, "ONE_MINUS_SRC_ALPHA", 18) == 0) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		if (strncmp(Value, "one_minus_dst_alpha", 18) == 0 || strncmp(Value, "ONE_MINUS_DST_ALPHA", 18) == 0) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		if (strncmp(Value, "one_minus_src_color", 17) == 0 || strncmp(Value, "ONE_MINUS_SRC_COLOR", 17) == 0) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		if (strncmp(Value, "one_minus_dst_color", 17) == 0 || strncmp(Value, "ONE_MINUS_DST_COLOR", 17) == 0) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		if (strncmp(Value, "src_color", 9) == 0 || strncmp(Value, "SRC_COLOR", 9) == 0) return VK_BLEND_FACTOR_SRC_COLOR;
		if (strncmp(Value, "dst_color", 9) == 0 || strncmp(Value, "DST_COLOR", 9) == 0) return VK_BLEND_FACTOR_DST_COLOR;
		if (strncmp(Value, "src_alpha", 9) == 0 || strncmp(Value, "SRC_ALPHA", 9) == 0) return VK_BLEND_FACTOR_SRC_ALPHA;
		if (strncmp(Value, "dst_alpha", 9) == 0 || strncmp(Value, "DST_ALPHA", 9) == 0) return VK_BLEND_FACTOR_DST_ALPHA;
		if (strncmp(Value, "zero", 4) == 0 || strncmp(Value, "ZERO", 4) == 0) return VK_BLEND_FACTOR_ZERO;
		if (strncmp(Value, "one", 3) == 0 || strncmp(Value, "ONE", 3) == 0) return VK_BLEND_FACTOR_ONE;
		
		return VK_BLEND_FACTOR_SRC_ALPHA;
	}

	VkBlendOp ParseBlendOp(const char* Value, u32 Length)
	{
		if (strncmp(Value, "reverse_subtract", 15) == 0 || strncmp(Value, "REVERSE_SUBTRACT", 15) == 0) return VK_BLEND_OP_REVERSE_SUBTRACT;
		if (strncmp(Value, "subtract", 8) == 0 || strncmp(Value, "SUBTRACT", 8) == 0) return VK_BLEND_OP_SUBTRACT;
		if (strncmp(Value, "add", 3) == 0 || strncmp(Value, "ADD", 3) == 0) return VK_BLEND_OP_ADD;
		if (strncmp(Value, "min", 3) == 0 || strncmp(Value, "MIN", 3) == 0) return VK_BLEND_OP_MIN;
		if (strncmp(Value, "max", 3) == 0 || strncmp(Value, "MAX", 3) == 0) return VK_BLEND_OP_MAX;
		
		return VK_BLEND_OP_ADD;
	}

	VkCompareOp ParseCompareOp(const char* Value, u32 Length)
	{
		if (strncmp(Value, "never", 5) == 0 || strncmp(Value, "NEVER", 5) == 0) return VK_COMPARE_OP_NEVER;
		if (strncmp(Value, "less", 4) == 0 || strncmp(Value, "LESS", 4) == 0) return VK_COMPARE_OP_LESS;
		if (strncmp(Value, "equal", 5) == 0 || strncmp(Value, "EQUAL", 5) == 0) return VK_COMPARE_OP_EQUAL;
		if (strncmp(Value, "less_or_equal", 12) == 0 || strncmp(Value, "LESS_OR_EQUAL", 12) == 0) return VK_COMPARE_OP_LESS_OR_EQUAL;
		if (strncmp(Value, "greater", 7) == 0 || strncmp(Value, "GREATER", 7) == 0) return VK_COMPARE_OP_GREATER;
		if (strncmp(Value, "not_equal", 9) == 0 || strncmp(Value, "NOT_EQUAL", 9) == 0) return VK_COMPARE_OP_NOT_EQUAL;
		if (strncmp(Value, "greater_or_equal", 15) == 0 || strncmp(Value, "GREATER_OR_EQUAL", 15) == 0) return VK_COMPARE_OP_GREATER_OR_EQUAL;
		if (strncmp(Value, "always", 6) == 0 || strncmp(Value, "ALWAYS", 6) == 0) return VK_COMPARE_OP_ALWAYS;
		
		return VK_COMPARE_OP_LESS;
	}

	VkSampleCountFlagBits ParseSampleCount(const char* Value, u32 Length)
	{
		if (strncmp(Value, "1", 1) == 0) return VK_SAMPLE_COUNT_1_BIT;
		if (strncmp(Value, "2", 1) == 0) return VK_SAMPLE_COUNT_2_BIT;
		if (strncmp(Value, "4", 1) == 0) return VK_SAMPLE_COUNT_4_BIT;
		if (strncmp(Value, "8", 1) == 0) return VK_SAMPLE_COUNT_8_BIT;
		if (strncmp(Value, "16", 2) == 0) return VK_SAMPLE_COUNT_16_BIT;
		if (strncmp(Value, "32", 2) == 0) return VK_SAMPLE_COUNT_32_BIT;
		if (strncmp(Value, "64", 2) == 0) return VK_SAMPLE_COUNT_64_BIT;
		
		return VK_SAMPLE_COUNT_1_BIT;
	}

	VkPrimitiveTopology ParseTopology(const char* Value, u32 Length)
	{
		if (strncmp(Value, "point_list", 10) == 0 || strncmp(Value, "POINT_LIST", 10) == 0) return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		if (strncmp(Value, "line_list", 9) == 0 || strncmp(Value, "LINE_LIST", 9) == 0) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		if (strncmp(Value, "line_strip", 10) == 0 || strncmp(Value, "LINE_STRIP", 10) == 0) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		if (strncmp(Value, "triangle_list", 13) == 0 || strncmp(Value, "TRIANGLE_LIST", 13) == 0) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		if (strncmp(Value, "triangle_strip", 14) == 0 || strncmp(Value, "TRIANGLE_STRIP", 14) == 0) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		if (strncmp(Value, "triangle_fan", 12) == 0 || strncmp(Value, "TRIANGLE_FAN", 12) == 0) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		if (strncmp(Value, "line_list_with_adjacency", 23) == 0 || strncmp(Value, "LINE_LIST_WITH_ADJACENCY", 23) == 0) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		if (strncmp(Value, "line_strip_with_adjacency", 24) == 0 || strncmp(Value, "LINE_STRIP_WITH_ADJACENCY", 24) == 0) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		if (strncmp(Value, "triangle_list_with_adjacency", 27) == 0 || strncmp(Value, "TRIANGLE_LIST_WITH_ADJACENCY", 27) == 0) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		if (strncmp(Value, "triangle_strip_with_adjacency", 28) == 0 || strncmp(Value, "TRIANGLE_STRIP_WITH_ADJACENCY", 28) == 0) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		if (strncmp(Value, "patch_list", 10) == 0 || strncmp(Value, "PATCH_LIST", 10) == 0) return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

}

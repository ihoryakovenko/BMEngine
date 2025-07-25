#include "VulkanHelper.h"

#include <cassert>

#include <glm/glm.hpp>

#include "Util/Util.h"

namespace VulkanHelper
{
	// Helper function to check if a string matches any of the strings in an array
	template<size_t N>
	bool StringMatches(const char* Value, u32 Length, const char* const (&Strings)[N])
	{
		for (size_t i = 0; i < N; ++i)
		{
			if (strncmp(Value, Strings[i], Length) == 0)
			{
				return true;
			}
		}
		return false;
	}

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
		
		if (StringMatches(Value, Length, ParseStrings::TRUE_STRINGS)) return VK_TRUE;
		if (StringMatches(Value, Length, ParseStrings::FALSE_STRINGS)) return VK_FALSE;
		
		return VK_FALSE;
	}

	VkPolygonMode ParsePolygonMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FILL_STRINGS)) return VK_POLYGON_MODE_FILL;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRINGS)) return VK_POLYGON_MODE_LINE;
		if (StringMatches(Value, Length, ParseStrings::POINT_STRINGS)) return VK_POLYGON_MODE_POINT;
		
		return VK_POLYGON_MODE_FILL;
	}

	VkCullModeFlags ParseCullMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NONE_STRINGS)) return VK_CULL_MODE_NONE;
		if (StringMatches(Value, Length, ParseStrings::BACK_STRINGS)) return VK_CULL_MODE_BACK_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRONT_STRINGS)) return VK_CULL_MODE_FRONT_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRONT_BACK_STRINGS)) return VK_CULL_MODE_FRONT_AND_BACK;
		
		return VK_CULL_MODE_BACK_BIT;
	}

	VkFrontFace ParseFrontFace(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::COUNTER_CLOCKWISE_STRINGS)) return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		if (StringMatches(Value, Length, ParseStrings::CLOCKWISE_STRINGS)) return VK_FRONT_FACE_CLOCKWISE;
		
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
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_ALPHA_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_ALPHA_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_SRC_COLOR_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		if (StringMatches(Value, Length, ParseStrings::ONE_MINUS_DST_COLOR_STRINGS)) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		if (StringMatches(Value, Length, ParseStrings::SRC_COLOR_STRINGS)) return VK_BLEND_FACTOR_SRC_COLOR;
		if (StringMatches(Value, Length, ParseStrings::DST_COLOR_STRINGS)) return VK_BLEND_FACTOR_DST_COLOR;
		if (StringMatches(Value, Length, ParseStrings::SRC_ALPHA_STRINGS)) return VK_BLEND_FACTOR_SRC_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::DST_ALPHA_STRINGS)) return VK_BLEND_FACTOR_DST_ALPHA;
		if (StringMatches(Value, Length, ParseStrings::ZERO_STRINGS)) return VK_BLEND_FACTOR_ZERO;
		if (StringMatches(Value, Length, ParseStrings::ONE_STRINGS)) return VK_BLEND_FACTOR_ONE;
		
		return VK_BLEND_FACTOR_SRC_ALPHA;
	}

	VkBlendOp ParseBlendOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REVERSE_SUBTRACT_STRINGS)) return VK_BLEND_OP_REVERSE_SUBTRACT;
		if (StringMatches(Value, Length, ParseStrings::SUBTRACT_STRINGS)) return VK_BLEND_OP_SUBTRACT;
		if (StringMatches(Value, Length, ParseStrings::ADD_STRINGS)) return VK_BLEND_OP_ADD;
		if (StringMatches(Value, Length, ParseStrings::MIN_STRINGS)) return VK_BLEND_OP_MIN;
		if (StringMatches(Value, Length, ParseStrings::MAX_STRINGS)) return VK_BLEND_OP_MAX;
		
		return VK_BLEND_OP_ADD;
	}

	VkCompareOp ParseCompareOp(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEVER_STRINGS)) return VK_COMPARE_OP_NEVER;
		if (StringMatches(Value, Length, ParseStrings::LESS_STRINGS)) return VK_COMPARE_OP_LESS;
		if (StringMatches(Value, Length, ParseStrings::EQUAL_STRINGS)) return VK_COMPARE_OP_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::LESS_OR_EQUAL_STRINGS)) return VK_COMPARE_OP_LESS_OR_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::GREATER_STRINGS)) return VK_COMPARE_OP_GREATER;
		if (StringMatches(Value, Length, ParseStrings::NOT_EQUAL_STRINGS)) return VK_COMPARE_OP_NOT_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::GREATER_OR_EQUAL_STRINGS)) return VK_COMPARE_OP_GREATER_OR_EQUAL;
		if (StringMatches(Value, Length, ParseStrings::ALWAYS_STRINGS)) return VK_COMPARE_OP_ALWAYS;
		
		return VK_COMPARE_OP_LESS;
	}

	VkSampleCountFlagBits ParseSampleCount(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_1_STRINGS)) return VK_SAMPLE_COUNT_1_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_2_STRINGS)) return VK_SAMPLE_COUNT_2_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_4_STRINGS)) return VK_SAMPLE_COUNT_4_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_8_STRINGS)) return VK_SAMPLE_COUNT_8_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_16_STRINGS)) return VK_SAMPLE_COUNT_16_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_32_STRINGS)) return VK_SAMPLE_COUNT_32_BIT;
		if (StringMatches(Value, Length, ParseStrings::SAMPLE_64_STRINGS)) return VK_SAMPLE_COUNT_64_BIT;
		
		return VK_SAMPLE_COUNT_1_BIT;
	}

	VkPrimitiveTopology ParseTopology(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::POINT_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_FAN_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		if (StringMatches(Value, Length, ParseStrings::LINE_LIST_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::LINE_STRIP_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_LIST_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::TRIANGLE_STRIP_WITH_ADJACENCY_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		if (StringMatches(Value, Length, ParseStrings::PATCH_LIST_STRINGS)) return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	VkShaderStageFlagBits ParseShaderStage(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_SHADER_STRINGS))
			return VK_SHADER_STAGE_VERTEX_BIT;
		if (StringMatches(Value, Length, ParseStrings::FRAGMENT_STRINGS))
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		if (StringMatches(Value, Length, ParseStrings::GEOMETRY_STRINGS))
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		if (StringMatches(Value, Length, ParseStrings::COMPUTE_STRINGS))
			return VK_SHADER_STAGE_COMPUTE_BIT;
		if (StringMatches(Value, Length, ParseStrings::TESS_CONTROL_STRINGS))
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (StringMatches(Value, Length, ParseStrings::TESS_EVAL_STRINGS))
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (StringMatches(Value, Length, ParseStrings::TASK_STRINGS))
			return VK_SHADER_STAGE_TASK_BIT_EXT;
		if (StringMatches(Value, Length, ParseStrings::MESH_STRINGS))
			return VK_SHADER_STAGE_MESH_BIT_EXT;
		if (StringMatches(Value, Length, ParseStrings::RAYGEN_STRINGS))
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::CLOSEST_HIT_STRINGS))
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::ANY_HIT_STRINGS))
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::MISS_STRINGS))
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::INTERSECTION_STRINGS))
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (StringMatches(Value, Length, ParseStrings::CALLABLE_STRINGS))
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;

		assert(false);
		return VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkFilter ParseFilter(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::NEAREST_STRINGS)) return VK_FILTER_NEAREST;
		if (StringMatches(Value, Length, ParseStrings::LINEAR_STRINGS)) return VK_FILTER_LINEAR;
		return VK_FILTER_LINEAR;
	}

	VkSamplerAddressMode ParseAddressMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::REPEAT_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		if (StringMatches(Value, Length, ParseStrings::MIRRORED_REPEAT_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_EDGE_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		if (StringMatches(Value, Length, ParseStrings::CLAMP_TO_BORDER_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		if (StringMatches(Value, Length, ParseStrings::MIRROR_CLAMP_TO_EDGE_STRINGS)) return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}

	VkBorderColor ParseBorderColor(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::FLOAT_TRANSPARENT_BLACK_STRINGS)) return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		if (StringMatches(Value, Length, ParseStrings::INT_TRANSPARENT_BLACK_STRINGS)) return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_BLACK_STRINGS)) return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_BLACK_STRINGS)) return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		if (StringMatches(Value, Length, ParseStrings::FLOAT_OPAQUE_WHITE_STRINGS)) return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (StringMatches(Value, Length, ParseStrings::INT_OPAQUE_WHITE_STRINGS)) return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	}

	VkSamplerMipmapMode ParseMipmapMode(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_NEAREST_STRINGS)) return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		if (StringMatches(Value, Length, ParseStrings::MIPMAP_LINEAR_STRINGS)) return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}

	VkDescriptorType ParseDescriptorType(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::SAMPLER_STRINGS)) return VK_DESCRIPTOR_TYPE_SAMPLER;
		if (StringMatches(Value, Length, ParseStrings::COMBINED_IMAGE_SAMPLER_STRINGS)) return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		if (StringMatches(Value, Length, ParseStrings::SAMPLED_IMAGE_STRINGS)) return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_IMAGE_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_TEXEL_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_TEXEL_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		if (StringMatches(Value, Length, ParseStrings::UNIFORM_BUFFER_DYNAMIC_STRINGS)) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		if (StringMatches(Value, Length, ParseStrings::STORAGE_BUFFER_DYNAMIC_STRINGS)) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		if (StringMatches(Value, Length, ParseStrings::INPUT_ATTACHMENT_STRINGS)) return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	VkShaderStageFlags ParseShaderStageFlags(const char* Value, u32 Length)
	{
		VkShaderStageFlags flags = 0;
		
		const char* token = Value;
		const char* end = Value + Length;
		
		while (token < end)
		{
			while (token < end && (*token == ' ' || *token == '|' || *token == '\t')) token++;
			if (token >= end) break;
			
			const char* tokenStart = token;
			while (token < end && *token != ' ' && *token != '|' && *token != '\t') token++;
			
			u32 tokenLength = static_cast<u32>(token - tokenStart);
			
			if (StringMatches(tokenStart, tokenLength, ParseStrings::VERTEX_BIT_STRINGS)) flags |= VK_SHADER_STAGE_VERTEX_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::FRAGMENT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::GEOMETRY_BIT_STRINGS)) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::COMPUTE_BIT_STRINGS)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_CONTROL_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TESSELLATION_EVALUATION_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::TASK_BIT_STRINGS)) flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MESH_BIT_STRINGS)) flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::RAYGEN_BIT_STRINGS)) flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CLOSEST_HIT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::ANY_HIT_BIT_STRINGS)) flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::MISS_BIT_STRINGS)) flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::INTERSECTION_BIT_STRINGS)) flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			else if (StringMatches(tokenStart, tokenLength, ParseStrings::CALLABLE_BIT_STRINGS)) flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		}
		
		return flags;
	}

	VkFormat ParseFormat(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::R32_SFLOAT_STRINGS)) return VK_FORMAT_R32_SFLOAT;
		if (StringMatches(Value, Length, ParseStrings::R32G32_SFLOAT_STRINGS)) return VK_FORMAT_R32G32_SFLOAT;
		if (StringMatches(Value, Length, ParseStrings::R32G32B32_SFLOAT_STRINGS)) return VK_FORMAT_R32G32B32_SFLOAT;
		if (StringMatches(Value, Length, ParseStrings::R32G32B32A32_SFLOAT_STRINGS)) return VK_FORMAT_R32G32B32A32_SFLOAT;
		if (StringMatches(Value, Length, ParseStrings::R32_UINT_STRINGS)) return VK_FORMAT_R32_UINT;
		
		assert(false);
		return VK_FORMAT_R32_SFLOAT;
	}

	VkVertexInputRate ParseVertexInputRate(const char* Value, u32 Length)
	{
		if (StringMatches(Value, Length, ParseStrings::VERTEX_STRINGS)) return VK_VERTEX_INPUT_RATE_VERTEX;
		if (StringMatches(Value, Length, ParseStrings::INSTANCE_STRINGS)) return VK_VERTEX_INPUT_RATE_INSTANCE;
		
		assert(false);
		return VK_VERTEX_INPUT_RATE_VERTEX;
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

	u32 CalculateFormatSizeFromString(const char* FormatString, u32 FormatLength)
	{
		VkFormat Format = ParseFormat(FormatString, FormatLength);
		return CalculateFormatSize(Format);
	}
}

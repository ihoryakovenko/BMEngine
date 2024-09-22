#pragma once

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init(GLFWwindow* Window, GLFWwindow* Window2);
		void DeInit();

		// VulkanRenderInstance
		void CreateImageBuffer(stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize);
		bool InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport, VkExtent2D SwapExtent);
		void DeinitViewport(ViewportInstance* Viewport);
		bool InitVulkanRenderInstance(VkSurfaceKHR Surface, GLFWwindow* Window, VkSurfaceKHR Surface2, GLFWwindow* Window2, VkExtent2D Extent1, VkExtent2D Extent2);
		bool LoadMesh(Mesh Mesh);
		bool Draw();
		void DeinitVulkanRenderInstance();
		void RemoveViewport(GLFWwindow* Window);

	private:
		ExtensionPropertiesData CreateDeviceExtensionPropertiesData();
		void DestroyExtensionPropertiesData(ExtensionPropertiesData& Data);

		QueueFamilyPropertiesData CreateQueueFamilyPropertiesData();
		void DestroyQueueFamilyPropertiesData(QueueFamilyPropertiesData& Data);

		SurfaceFormatsData CreateSurfaceFormatsData(VkSurfaceKHR Surface);
		void DestroySurfaceFormatsData(SurfaceFormatsData& Data);

		PresentModeData CreatePresentModeData(VkSurfaceKHR Surface);
		void DestroyPresentModeData(PresentModeData& Data);

		PhysicalDevicesData CreatePhysicalDevicesData();
		void DestroyPhysicalDevicesData(PhysicalDevicesData& Data);

		bool CheckDeviceSuitability(const char* DeviceExtensions[], uint32_t DeviceExtensionsSize,
			ExtensionPropertiesData DeviceExtensionsData, VkSurfaceKHR Surface);





		uint32_t FindMemoryTypeIndex(uint32_t AllowedTypes, VkMemoryPropertyFlags Properties);

		GPUBuffer CreateVertexBuffer(Vertex* Vertices, uint32_t VerticesCount);

		GPUBuffer CreateIndexBuffer(uint32_t* Indices, uint32_t IndicesCount);

		GPUBuffer CreateUniformBuffer(VkDeviceSize bufferSize);

		GPUBuffer CreateIndirectBuffer(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands);

		GPUBuffer CreateGPUBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties);

		void DestroyGPUBuffer(GPUBuffer& buffer);

		void CopyGPUBuffer(const GPUBuffer& srcBuffer, GPUBuffer& dstBuffer);

		void CopyBufferToImage(VkBuffer SourceBuffer, VkImage Image, uint32_t Width, uint32_t Height);


		VkImageView CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);

		VkImage CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling,
			VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags, VkDeviceMemory* OutImageMemory);

		VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
			uint32_t DeviceExtensionsSize);

		VkSwapchainKHR CreateSwapchain(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, VkSurfaceKHR Surface,
			VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
			PhysicalDeviceIndices DeviceIndices);

		
		VkDescriptorPool CreateSamplerDescriptorPool(uint32_t Count);

		VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);
		VkPresentModeKHR GetBestPresentationMode(VkSurfaceKHR Surface);

		

		VkSampler CreateTextureSampler();

		bool SetupQueues();

		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);



	public:
		MainInstance Instance;
		VulkanRenderInstance RenderInstance;
		VkDevice LogicalDevice = nullptr;
		DeviceInstance Device;
		MainRenderPass MainPass;

	private:
		ExtensionPropertiesData AvailableExtensions;
		RequiredInstanceExtensionsData RequiredExtensions;
		AvailableInstanceLayerPropertiesData LayerPropertiesData;
		PhysicalDevicesData DevicesData;
		ExtensionPropertiesData DeviceExtensionsData;
		QueueFamilyPropertiesData FamilyPropertiesData;

		
	};
}
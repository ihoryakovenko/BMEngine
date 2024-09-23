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
		void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport, VkExtent2D SwapExtent);
		void DeinitViewport(ViewportInstance* Viewport);
		bool LoadMesh(Mesh Mesh);
		bool Draw();
		void RemoveViewport(GLFWwindow* Window);

	private:
		void GetAvailableExtensionProperties(std::vector<VkExtensionProperties>& Data);
		void GetAvailableInstanceLayerProperties(std::vector<VkLayerProperties>& Data);
		void GetRequiredInstanceExtensions(const char** ValidationExtensions,
			uint32_t ValidationExtensionsCount, std::vector<const char*>& Data);
		void GetSurfaceFormats(VkSurfaceKHR Surface, std::vector<VkSurfaceFormatKHR>& SurfaceFormats);
		void GetAvailablePresentModes(VkSurfaceKHR Surface, std::vector<VkPresentModeKHR>& PresentModes);
		
		void GetSwapchainImages(VkSwapchainKHR VulkanSwapchain, std::vector<VkImage>& Images);
		VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window);
		uint32_t GetMemoryTypeIndex(uint32_t AllowedTypes, VkMemoryPropertyFlags Properties);

		bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, uint32_t AvailableExtensionsCount,
			const char** RequiredExtensions, uint32_t RequiredExtensionsCount);
		bool CheckValidationLayersSupport(VkLayerProperties* Properties, uint32_t PropertiesSize,
			const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize);
		VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);
		VkPresentModeKHR GetBestPresentationMode(VkSurfaceKHR Surface);

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
		VkSampler CreateTextureSampler();

		bool SetupQueues();
		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

		void InitImageView(ViewportInstance* OutViewport, const std::vector<VkImage>& Images);

	public:
		MainInstance Instance;
		VkDevice LogicalDevice = nullptr;
		DeviceInstance Device;
		MainRenderPass MainPass;

		uint32_t ViewportsCount = 0;
		ViewportInstance* Viewports[2];
		ViewportInstance MainViewport;

		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentationQueue = nullptr;

		// Todo: pass as AddViewport params? 
		VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
		VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkPresentModeKHR PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };

		VkSampler TextureSampler = nullptr;
		VkDescriptorPool SamplerDescriptorPool = nullptr;
		VkDescriptorSet* SamplerDescriptorSets = nullptr;

		//GenericBuffer* ModelDynamicUniformBuffers = nullptr;

		//uint32_t ModelUniformAlignment = 0;
		//UboModel* ModelTransferSpace = nullptr;

		const uint32_t MaxObjects = 528;
		uint32_t DrawableObjectsCount = 0;
		DrawableObject DrawableObjects[528];
		const int MaxFrameDraws = 2;
		int CurrentFrame = 0;

		// Todo: put all textures in atlases or texture layers
		const uint32_t MaxTextures = 64;
		uint32_t TextureImagesCount = 0;
		// Todo: have single TextureImagesMemory and VkImage offset references in it
		ImageBuffer TextureImageBuffer[64];
	};
}
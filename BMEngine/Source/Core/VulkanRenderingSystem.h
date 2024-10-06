#pragma once

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init(GLFWwindow* Window);
		void DeInit();

		// VulkanRenderInstance
		void CreateImageBuffer(stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize);
		void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
			VkDescriptorPool DescriptorPool, SwapchainInstance SwapInstance, ImageBuffer* ColorBuffers, ImageBuffer* DepthBuffers);
		void DeinitViewport(ViewportInstance* Viewport);
		bool LoadMesh(Mesh Mesh);
		bool LoadTerrain();
		bool Draw();

	private:
		void GetAvailableExtensionProperties(std::vector<VkExtensionProperties>& Data);
		void GetAvailableInstanceLayerProperties(std::vector<VkLayerProperties>& Data);
		void GetRequiredInstanceExtensions(const char** ValidationExtensions,
			uint32_t ValidationExtensionsCount, std::vector<const char*>& Data);
		void GetSurfaceFormats(VkSurfaceKHR Surface, std::vector<VkSurfaceFormatKHR>& SurfaceFormats);
		
		VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window);

		bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, uint32_t AvailableExtensionsCount,
			const char** RequiredExtensions, uint32_t RequiredExtensionsCount);
		bool CheckValidationLayersSupport(VkLayerProperties* Properties, uint32_t PropertiesSize,
			const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize);
		VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);
		
		void CopyBufferToImage(VkBuffer SourceBuffer, VkImage Image, uint32_t Width, uint32_t Height);

		VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
			uint32_t DeviceExtensionsSize);
		VkDescriptorPool CreateSamplerDescriptorPool(uint32_t Count);
		VkSampler CreateTextureSampler();

		bool SetupQueues();
		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

		void CreateSynchronisation();
		void DestroySynchronisation();

	public:
		MainInstance Instance;
		VkDevice LogicalDevice = nullptr;
		DeviceInstance Device;
		MainRenderPass MainPass;

		ViewportInstance MainViewport;

		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentationQueue = nullptr;

		// Todo: pass as AddViewport params? 
		VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
		VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

		VkSampler TextureSampler = nullptr;
		VkDescriptorPool SamplerDescriptorPool = nullptr;
		VkDescriptorSet* SamplerDescriptorSets = nullptr;

		VkDescriptorPool DescriptorPool = nullptr;

		VkSemaphore* ImageAvailable = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

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


		uint32_t TerrainVerticesCount = 0;
		GPUBuffer TerrainVertexBuffer;

		uint32_t TerrainIndicesCount = 0;
		GPUBuffer TerrainIndexBuffer;
	};
}
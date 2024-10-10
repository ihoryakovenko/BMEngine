#pragma once

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init(GLFWwindow* Window, const RenderConfig& InConfig);
		void DeInit();

		void LoadTextures(TextureInfo* Infos, uint32_t TexturesCount);

		void CreateDrawEntity(Mesh Mesh, DrawEntity& OutEntity);
		void DestroyDrawEntity(DrawEntity& Entity);

		void CreateTerrainIndices(uint32_t* Indices, uint32_t IndicesCount);
		void CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, uint32_t TerrainVerticesCount, DrawTerrainEntity& OutTerrain);
		void DestroyTerrainDrawEntity(DrawTerrainEntity& Entity);

		void Draw(const DrawScene& Scene);

		static inline RenderConfig Config;

	private:
		static void SetConfig(const RenderConfig& InConfig) { Config = InConfig; };

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
		
		void CopyBufferToImage(VkBuffer SourceBuffer, VkDeviceSize bufferOffset, VkImage Image, uint32_t Width, uint32_t Height);

		VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
			uint32_t DeviceExtensionsSize);
		VkSampler CreateTextureSampler();

		bool SetupQueues();
		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

		void CreateSynchronisation();
		void DestroySynchronisation();

		void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
			VkDescriptorPool DescriptorPool, SwapchainInstance SwapInstance, ImageBuffer* ColorBuffers, ImageBuffer* DepthBuffers);
		void DeinitViewport(ViewportInstance* Viewport);

	private:
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

		VkDescriptorPool StaticPool = nullptr;

		VkSemaphore* ImageAvailable = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		const int MaxFrameDraws = 2;
		int CurrentFrame = 0;

		// Todo: put all textures in atlases or texture layers?
		struct
		{
			VkSampler TextureSampler = nullptr;
			VkDescriptorPool SamplerDescriptorPool = nullptr;
			VkDescriptorSet* SamplerDescriptorSets = nullptr;

			VkDeviceMemory TextureImagesMemory = nullptr;
			VkImage* Images = nullptr;
			VkImageView* ImageViews = nullptr;
			uint32_t ImagesCount = 0;
		} TextureUnit;

		uint32_t TerrainIndicesCount = 0;
		GPUBuffer TerrainIndexBuffer;
	};
}
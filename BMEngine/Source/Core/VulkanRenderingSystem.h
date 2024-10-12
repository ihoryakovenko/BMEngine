#pragma once

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"
#include "VulkanMemoryManagementSystem.h"
#include "Util/EngineTypes.h"

#include "Memory/MemoryManagmentSystem.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init(GLFWwindow* Window, const RenderConfig& InConfig);
		void DeInit();

		void LoadTextures(TextureInfo* Infos, u32 TexturesCount);

		void CreateDrawEntity(Mesh Mesh, DrawEntity& OutEntity);
		void DestroyDrawEntity(DrawEntity& Entity);

		void CreateTerrainIndices(u32* Indices, u32 IndicesCount);
		void CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, u32 TerrainVerticesCount, DrawTerrainEntity& OutTerrain);
		void DestroyTerrainDrawEntity(DrawTerrainEntity& Entity);

		void Draw(const DrawScene& Scene);

		static inline RenderConfig Config;

	private:
		static void SetConfig(const RenderConfig& InConfig) { Config = InConfig; };

		Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties();
		Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties();
		Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions, u32 ValidationExtensionsCount);
		Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface);
		
		VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window);

		bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
			const char** RequiredExtensions, u32 RequiredExtensionsCount);
		bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
			const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
		VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);
		
		void CopyBufferToImage(VkBuffer SourceBuffer, VkDeviceSize bufferOffset, VkImage Image, u32 Width, u32 Height);

		VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
			u32 DeviceExtensionsSize);
		VkSampler CreateTextureSampler();

		bool SetupQueues();
		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

		void CreateSynchronisation();
		void DestroySynchronisation();

		void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
			SwapchainInstance SwapInstance, ImageBuffer* ColorBuffers, ImageBuffer* DepthBuffers);
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

		VkSemaphore* ImageAvailable = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		const int MaxFrameDraws = 2;
		int CurrentFrame = 0;

		// Todo: put all textures in atlases or texture layers?
		struct
		{
			VkSampler TextureSampler = nullptr;
			VkDescriptorSet* SamplerDescriptorSets = nullptr;

			VkImage* Images = nullptr;
			VkImageView* ImageViews = nullptr;
			u32 ImagesCount = 0;
		} TextureUnit;

		u32 TerrainIndicesCount = 0;
		GPUBuffer TerrainIndexBuffer;
	};
}
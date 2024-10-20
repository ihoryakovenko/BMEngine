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

		void LoadTexture(TextureArrayInfo DiffuseTexture, TextureArrayInfo SpecularTexture);

		void CreateDrawEntities(Mesh* Meshes, u32 MeshesCount, DrawEntity* OutEntities);

		void CreateTerrainIndices(u32* Indices, u32 IndicesCount);
		void CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, u32 TerrainVerticesCount, DrawTerrainEntity& OutTerrain);

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

		VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
			u32 DeviceExtensionsSize);
		VkSampler CreateTextureSampler();

		bool SetupQueues();
		bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

		void CreateSynchronisation();
		void DestroySynchronisation();

		void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
			SwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers);
		void DeinitViewport(ViewportInstance* Viewport);

		void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex);

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

		static const u32 MAX_DRAW_FRAMES = 2;
		int CurrentFrame = 0;

		VkSemaphore ImageAvailable[MAX_DRAW_FRAMES];
		VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
		VkFence DrawFences[MAX_DRAW_FRAMES];

		VkCommandPool GraphicsCommandPool = nullptr;

		GPUBuffer VertexBuffer;
		u32 VertexBufferOffset = 0;
		GPUBuffer IndexBuffer;
		u32 IndexBufferOffset = 0;

		VkDescriptorPool MainPool = nullptr;

		struct
		{
			VkSampler DiffuseTextureSampler = nullptr;
			VkSampler SpecularTextureSampler = nullptr;
			static inline ImageBuffer DiffuseTextureBuffers[MAX_IMAGES];
			static inline ImageBuffer SpecularTextureBuffers[MAX_IMAGES];
			u32 ImageArraysCount = 0;
			static inline VkImageView DiffuseTextureImageViews[MAX_IMAGES];
			static inline VkImageView SpecularTextureImageViews[MAX_IMAGES];
			
		} TextureUnit;

		u32 TerrainIndicesCount = 0;
		VkDeviceSize TerrainIndexOffset = 0;
	};
}
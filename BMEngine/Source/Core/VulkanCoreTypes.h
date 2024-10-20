#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stb_image.h>

#include <vector>

#include "Util/EngineTypes.h"
#include <Memory/MemoryManagmentSystem.h>

namespace Core
{
	static const u32 MAX_SWAPCHAIN_IMAGES_COUNT = 3;
	static const u32 MAX_IMAGES = 1024;
	static const u32 Mb64 = 1024 * 1024 * 64;
	static const u32 BufferAlignment = 64;
	static const u32 ImageAlignment = 4096;

	using ShaderName = const char*;
	struct ShaderNames
	{
		static inline ShaderName TerrainVertex = "TerrainVertex";
		static inline ShaderName TerrainFragment = "TerrainFragment";
		static inline ShaderName EntityVertex = "EntityVertex";
		static inline ShaderName EntityFragment = "EntityFragment";
		static inline ShaderName DeferredVertex = "DeferredVertex";
		static inline ShaderName DeferredFragment = "DeferredFragment";

		static inline const u32 ShadersCount = 6;
	};

	struct ShaderCodeDescription
	{
		u32* Code = nullptr;
		u32 CodeSize = 0;
		ShaderName Name = nullptr;
	};

	struct RenderConfig
	{
		ShaderCodeDescription* RenderShaders = nullptr;
		u32 ShadersCount = 0;

		u32 MaxTextures = 0;
	};

	using Model = glm::mat4;

	struct UboViewProjection
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct PhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};

	struct EntityVertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct TerrainVertex
	{
		float Altitude;
	};

	struct Mesh
	{
		u32 MeshVerticesCount = 0;
		EntityVertex* MeshVertices = nullptr;

		u32 MeshIndicesCount = 0;
		u32* MeshIndices = nullptr;

		Model Model = glm::mat4(1.0f);
	};

	struct TerrainMesh
	{
		u32 VerticesCount = 0;
		TerrainVertex* Vertices = nullptr;
	};

	struct LoadScene
	{
		Mesh* Meshes = nullptr;
		u32 MeshesCount = 0;

		TerrainMesh* TerrainMeshes = nullptr;
		u32 TerrainMeshesCount = 0;
	};

	struct ImageBuffer
	{
		VkImage Image = nullptr;
		VkDeviceMemory Memory = nullptr;
	};

	struct GPUBuffer
	{
		VkBuffer Buffer = nullptr;
		VkDeviceMemory Memory = nullptr;
	};

	struct DrawEntity
	{
		VkDeviceSize VertexOffset = 0;
		VkDeviceSize IndexOffset = 0;
		u32 IndicesCount = 0;
		Model Model;
		u32 TextureId = 0;
	};

	struct DrawTerrainEntity
	{
		VkDeviceSize VertexOffset = 0;
	};

	struct DrawPointLightEntity
	{
		alignas(16) glm::vec3 Position;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct Material
	{
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		glm::vec3 Specular;
		float Shininess;
	};

	struct DrawScene
	{
		UboViewProjection ViewProjection;

		DrawEntity* DrawEntities;
		u32 DrawEntitiesCount;

		DrawTerrainEntity* DrawTerrainEntities;
		u32 DrawTerrainEntitiesCount;
	};

	struct TextureArrayInfo
	{
		u32 Width = 0;
		u32 Height = 0;
		u32 Format = 0;
		u32 LayersCount = 0;
		stbi_uc* Data = nullptr;
	};
	
	struct MainInstance
	{
		static MainInstance CreateMainInstance(const char** RequiredExtensions, u32 RequiredExtensionsCount,
			bool IsValidationLayersEnabled, const char* ValidationLayers[], u32 ValidationLayersSize);
		static void DestroyMainInstance(MainInstance& Instance);

		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct SwapchainInstance
	{
		static SwapchainInstance CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
			PhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
			VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent);

		static void DestroySwapchainInstance(VkDevice LogicalDevice, SwapchainInstance& Instance);

		static VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
			VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
			PhysicalDeviceIndices DeviceIndices);
		static VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
			VkSurfaceKHR Surface);
		static Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);

		VkSwapchainKHR VulkanSwapchain = nullptr;
		u32 ImagesCount = 0;
		VkImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent = { };
	};

	struct DeviceInstance
	{
		void Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
			u32 DeviceExtensionsSize);

		static Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance);
		static Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice);
		static Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice);
		static PhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
			VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
			VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, PhysicalDeviceIndices Indices,
			VkPhysicalDeviceFeatures AvailableFeatures);
		static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
			const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);

		VkPhysicalDevice PhysicalDevice = nullptr;
		PhysicalDeviceIndices Indices;
		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures AvailableFeatures;
	};

	struct ViewportInstance
	{
		GLFWwindow* Window = nullptr;
		VkSurfaceKHR Surface = nullptr;

		SwapchainInstance ViewportSwapchain;

		VkFramebuffer SwapchainFramebuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	};
}

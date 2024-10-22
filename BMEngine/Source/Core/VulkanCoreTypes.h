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
	static const u32 MB64 = 1024 * 1024 * 64;
	static const u32 BUFFER_ALIGNMENT = 64;
	static const u32 IMAGE_ALIGNMENT = 4096;
	static const u32 MAX_DRAW_FRAMES = 3;

	using ShaderName = const char*;
	struct BrShaderNames
	{
		static inline ShaderName TerrainVertex = "TerrainVertex";
		static inline ShaderName TerrainFragment = "TerrainFragment";
		static inline ShaderName EntityVertex = "EntityVertex";
		static inline ShaderName EntityFragment = "EntityFragment";
		static inline ShaderName DeferredVertex = "DeferredVertex";
		static inline ShaderName DeferredFragment = "DeferredFragment";

		static inline const u32 ShadersCount = 6;
	};

	struct BrShaderCodeDescription
	{
		u32* Code = nullptr;
		u32 CodeSize = 0;
		ShaderName Name = nullptr;
	};

	struct BrConfig
	{
		BrShaderCodeDescription* RenderShaders = nullptr;
		u32 ShadersCount = 0;

		u32 MaxTextures = 0;
	};

	using BrModel = glm::mat4;

	struct BrUboViewProjection
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct BrPhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};

	struct BrEntityVertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoords;
		glm::vec3 Normal;
	};

	struct BrTerrainVertex
	{
		float Altitude;
	};

	struct BrMesh
	{
		u32 MeshVerticesCount = 0;
		BrEntityVertex* MeshVertices = nullptr;

		u32 MeshIndicesCount = 0;
		u32* MeshIndices = nullptr;

		u32 MaterialIndex = -1;

		BrModel Model = glm::mat4(1.0f);
	};

	struct BrTerrainMesh
	{
		u32 VerticesCount = 0;
		BrTerrainVertex* Vertices = nullptr;
	};

	struct BrLoadScene
	{
		BrMesh* Meshes = nullptr;
		u32 MeshesCount = 0;

		BrTerrainMesh* TerrainMeshes = nullptr;
		u32 TerrainMeshesCount = 0;
	};

	struct BrImageBuffer
	{
		VkImage Image = nullptr;
		VkDeviceMemory Memory = nullptr;
	};

	struct BrGPUBuffer
	{
		VkBuffer Buffer = nullptr;
		VkDeviceMemory Memory = nullptr;
	};

	struct BrDrawEntity
	{
		VkDeviceSize VertexOffset = 0;
		VkDeviceSize IndexOffset = 0;
		u32 IndicesCount = 0;
		u32 MaterialIndex = -1;
		BrModel Model;
	};

	struct BrDrawTerrainEntity
	{
		VkDeviceSize VertexOffset = 0;
	};

	struct BrPointLight
	{
		glm::vec4 Position;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
	};

	struct BrDirectionLight
	{
		alignas(16) glm::vec3 Direction;
		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct BrSpotLight
	{
		glm::vec3 Position;
		f32 CutOff;
		glm::vec3 Direction;
		f32 OuterCutOff;
		glm::vec3 Ambient;
		f32 Constant;
		glm::vec3 Diffuse;
		f32 Linear;
		glm::vec3 Specular;
		f32 Quadratic;
	};

	struct BrLightBuffer
	{
		BrPointLight PointLight;
		BrDirectionLight DirectionLight;
		BrSpotLight SpotLight;
	};

	struct BrMaterial
	{
		f32 Shininess;
	};

	struct BrDrawScene
	{
		BrUboViewProjection ViewProjection;

		BrDrawEntity* DrawEntities;
		u32 DrawEntitiesCount;

		BrDrawTerrainEntity* DrawTerrainEntities;
		u32 DrawTerrainEntitiesCount;
	};

	struct BrTextureArrayInfo
	{
		u32 Width = 0;
		u32 Height = 0;
		u32 Format = 0;
		u32 LayersCount = 0;
		stbi_uc* Data = nullptr;
	};
	
	struct BrMainInstance
	{
		static BrMainInstance CreateMainInstance(const char** RequiredExtensions, u32 RequiredExtensionsCount,
			bool IsValidationLayersEnabled, const char* ValidationLayers[], u32 ValidationLayersSize);
		static void DestroyMainInstance(BrMainInstance& Instance);

		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct BrSwapchainInstance
	{
		static BrSwapchainInstance CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
			BrPhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
			VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent);

		static void DestroySwapchainInstance(VkDevice LogicalDevice, BrSwapchainInstance& Instance);

		static VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
			VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
			BrPhysicalDeviceIndices DeviceIndices);
		static VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
			VkSurfaceKHR Surface);
		static Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);

		VkSwapchainKHR VulkanSwapchain = nullptr;
		u32 ImagesCount = 0;
		VkImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent = { };
	};

	struct BrDeviceInstance
	{
		void Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
			u32 DeviceExtensionsSize);

		static Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance);
		static Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice);
		static Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice);
		static BrPhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
			VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
			VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BrPhysicalDeviceIndices Indices,
			VkPhysicalDeviceFeatures AvailableFeatures);
		static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
			const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);

		VkPhysicalDevice PhysicalDevice = nullptr;
		BrPhysicalDeviceIndices Indices;
		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures AvailableFeatures;
	};

	struct BrViewportInstance
	{
		GLFWwindow* Window = nullptr;
		VkSurfaceKHR Surface = nullptr;

		BrSwapchainInstance ViewportSwapchain;

		VkFramebuffer SwapchainFramebuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	};
}

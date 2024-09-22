#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stb_image.h>

#include <vector>

#include "MainRenderPass.h"

namespace Core
{
	struct RequiredInstanceExtensionsData
	{
		uint32_t Count = 0;
		const char** Extensions = nullptr;
		uint32_t ValidationExtensionsCount = 0;
	};

	struct ExtensionPropertiesData
	{
		uint32_t Count = 0;
		VkExtensionProperties* ExtensionProperties = nullptr;
	};

	struct AvailableInstanceLayerPropertiesData
	{
		uint32_t Count = 0;
		VkLayerProperties* Properties = nullptr;
	};

	struct QueueFamilyPropertiesData
	{
		uint32_t Count = 0;
		VkQueueFamilyProperties* Properties = nullptr;
	};

	struct SurfaceFormatsData
	{
		uint32_t Count = 0;
		VkSurfaceFormatKHR* SurfaceFormats = nullptr;
	};

	struct PresentModeData
	{
		uint32_t Count = 0;
		VkPresentModeKHR* PresentModes = nullptr;
	};

	struct PhysicalDevicesData
	{
		uint32_t Count = 0;
		VkPhysicalDevice* DeviceList = nullptr;
	};

	struct PhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};

	struct GPUBuffer
	{
		VkBuffer Buffer = 0;
		VkDeviceMemory Memory = 0;
		VkDeviceSize Size = 0;
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoords;
	};

	struct Mesh
	{
		uint32_t MeshVerticesCount = 0;
		Vertex* MeshVertices = nullptr;

		uint32_t MeshIndicesCount = 0;
		uint32_t* MeshIndices = nullptr;
	};

	struct ImageBuffer
	{
		VkImage Image = nullptr;
		VkDeviceMemory Memory = nullptr;
		VkImageView ImageView = nullptr;
	};

	typedef glm::mat4 Model;

	struct DrawableObject
	{
		uint32_t VerticesCount = 0;
		GPUBuffer VertexBuffer;

		uint32_t IndicesCount = 0;
		GPUBuffer IndexBuffer;

		Model Model;

		uint32_t TextureId = 0;
	};

	struct MainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	// TODO: Remove Indices Properties AvailableFeatures?
	struct DeviceInstance
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		PhysicalDeviceIndices Indices;
		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures AvailableFeatures;
	};

	struct ViewportInstance
	{
		GLFWwindow* Window = nullptr;
		VkSurfaceKHR Surface = nullptr;

		VkExtent2D SwapExtent = {};
		VkSwapchainKHR VulkanSwapchain = nullptr;

		uint32_t SwapchainImagesCount = 0;
		VkImageView* ImageViews = nullptr;

		VkFramebuffer* SwapchainFramebuffers = nullptr;
		VkCommandBuffer* CommandBuffers = nullptr;
		ImageBuffer* ColorBuffers = nullptr;
		ImageBuffer* DepthBuffers = nullptr;

		VkDescriptorPool DescriptorPool = nullptr;
		VkDescriptorPool InputDescriptorPool = nullptr;
		VkDescriptorSet* DescriptorSets = nullptr;
		VkDescriptorSet* InputDescriptorSets = nullptr;



		GPUBuffer* VpUniformBuffers = nullptr;

		struct UboViewProjection
		{
			glm::mat4 View;
			glm::mat4 Projection;
		} ViewProjection;
	};

	struct VulkanRenderInstance
	{
		VkInstance VulkanInstance = nullptr;

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

	bool CheckRequiredInstanceExtensionsSupport(ExtensionPropertiesData AvailableExtensions,
		RequiredInstanceExtensionsData RequiredExtensions);

	bool CheckValidationLayersSupport(AvailableInstanceLayerPropertiesData& Data,
		const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize);

	bool CheckDeviceExtensionsSupport(ExtensionPropertiesData Data, const char** ExtensionsToCheck,
		uint32_t ExtensionsToCheckSize);




	RequiredInstanceExtensionsData CreateRequiredInstanceExtensionsData(const char** ValidationExtensions, uint32_t ValidationExtensionsCount);
	void DestroyRequiredInstanceExtensionsData(RequiredInstanceExtensionsData& Data);

	ExtensionPropertiesData CreateAvailableExtensionPropertiesData();


	AvailableInstanceLayerPropertiesData CreateAvailableInstanceLayerPropertiesData();
	void DestroyAvailableInstanceLayerPropertiesData(AvailableInstanceLayerPropertiesData& Data);


	PhysicalDeviceIndices GetPhysicalDeviceIndices(QueueFamilyPropertiesData Data, VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);

	MainInstance CreateMainInstance(RequiredInstanceExtensionsData RequiredExtensions,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize);
	void DestroyMainInstance(MainInstance& Instance);


	VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window);
}

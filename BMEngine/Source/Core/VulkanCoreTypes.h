#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Core
{
	// Deinit if InitVkInstanceCreateInfoSetupData returns true
	struct VkInstanceCreateInfoSetupData
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = nullptr;

		const char** RequiredAndValidationExtensions = nullptr;
		uint32_t RequiredAndValidationExtensionsCount = 0;

		uint32_t AvalibleExtensionsCount = 0;
		VkExtensionProperties* AvalibleExtensions = nullptr;

		uint32_t AvalibleValidationLayersCount = 0;
		VkLayerProperties* AvalibleValidationLayers = nullptr;
	};

	bool InitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data,
		const char** ValidationExtensions, uint32_t ValidationExtensionsSize, bool EnumerateInstanceLayerProperties);

	void DeinitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data);

	bool CheckRequiredInstanceExtensionsSupport(const VkInstanceCreateInfoSetupData& Data);
	bool CheckValidationLayersSupport(const VkInstanceCreateInfoSetupData& Data, const char** ValidationLeyersToCheck,
		uint32_t ValidationLeyersToCheckSize);

	// Deinit if InitVkPhysicalDeviceSetupData returns true
	struct VkPhysicalDeviceSetupData
	{
		uint32_t AvalibleDeviceExtensionsCount = 0;
		VkExtensionProperties* AvalibleDeviceExtensions = nullptr;

		uint32_t FamilyPropertiesCount = 0;
		VkQueueFamilyProperties* FamilyProperties = nullptr;

		uint32_t SurfaceFormatsCount = 0;
		VkSurfaceFormatKHR* SurfaceFormats = nullptr;

		uint32_t PresentModesCount = 0;
		VkPresentModeKHR* PresentModes = nullptr;
	};

	bool InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	void DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data);

	bool CheckDeviceExtensionsSupport(const VkPhysicalDeviceSetupData& Data, const char** ExtensionsToCheck,
		uint32_t ExtensionsToCheckSize);

	struct PhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
	};

	PhysicalDeviceIndices GetPhysicalDeviceIndices(const VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
	};

	struct Mesh
	{
		uint32_t MeshVerticesCount = 0;
		Vertex* MeshVertices = nullptr;

		uint32_t MeshIndicesCount = 0;
		uint32_t* MeshIndices = nullptr;
	};

	struct VulkanBuffer
	{
		VkBuffer Buffer = nullptr;
		VkDeviceMemory BufferMemory = nullptr;
	};

	struct DrawableObject
	{
		uint32_t VerticesCount = 0;
		VulkanBuffer VertexBuffer;

		uint32_t IndicesCount = 0;
		VulkanBuffer IndexBuffer;
	};

	struct MainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	bool InitMainInstance(MainInstance& Instance, bool IsValidationLayersEnabled);
	void DeinitMainInstance(MainInstance& Instance);

	struct VulkanRenderInstance
	{
		VkInstance VulkanInstance = nullptr;

		GLFWwindow* Window = nullptr;
		VkSurfaceKHR Surface = nullptr;

		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;

		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentationQueue = nullptr;

		VkExtent2D SwapExtent;
		VkSwapchainKHR VulkanSwapchain = nullptr;

		uint32_t SwapchainImagesCount = 0;
		VkImageView* ImageViews = nullptr;

		VkFramebuffer* SwapchainFramebuffers = nullptr;
		VkCommandBuffer* CommandBuffers = nullptr;
		VkPipeline GraphicsPipeline = nullptr;
		VkCommandPool GraphicsCommandPool = nullptr;

		VkPipelineLayout PipelineLayout = nullptr;

		VkRenderPass RenderPass = nullptr;

		DrawableObject DrawableObject;

		int MaxFrameDraws = 0;
		int CurrentFrame = 0;

		VkSemaphore* ImageAvalible = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;
	};

	bool InitVulkanRenderInstance(VulkanRenderInstance& RenderInstance, VkInstance VulkanInstance, GLFWwindow* Window);
	bool LoadMesh(VulkanRenderInstance& RenderInstance, Mesh Mesh);
	bool RecordCommands(VulkanRenderInstance& RenderInstance);
	bool Draw(VulkanRenderInstance& RenderInstance);
	void DeinitVulkanRenderInstance(VulkanRenderInstance& RenderInstance);

	bool CreateVulkanBuffer(const VulkanRenderInstance& RenderInstance, VkDeviceSize BufferSize,
		VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags BufferProperties, VulkanBuffer& OutBuffer);

	void DestroyVulkanBuffer(const VulkanRenderInstance& RenderInstance, VulkanBuffer& Buffer);

	void CopyBuffer(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkBuffer DstinationBuffer, VkDeviceSize BufferSize);
}

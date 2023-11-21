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

	struct GenericBuffer
	{
		VkBuffer Buffer = nullptr;
		VkDeviceMemory BufferMemory = nullptr;
	};

	struct ImageBuffer
	{
		VkImage TextureImage = nullptr;
		VkDeviceMemory TextureImagesMemory = nullptr;
	};

	typedef glm::mat4 Model;

	struct DrawableObject
	{
		uint32_t VerticesCount = 0;
		GenericBuffer VertexBuffer;

		uint32_t IndicesCount = 0;
		GenericBuffer IndexBuffer;

		Model Model;
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
		GLFWwindow* Window = nullptr;

		VkInstance VulkanInstance = nullptr;

		VkPhysicalDeviceProperties PhysicalDeviceProperties;
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;

		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentationQueue = nullptr;
		VkSurfaceKHR Surface = nullptr;
		VkExtent2D SwapExtent;
		VkSwapchainKHR VulkanSwapchain = nullptr;

		uint32_t SwapchainImagesCount = 0;
		VkImageView* ImageViews = nullptr;
		VkFramebuffer* SwapchainFramebuffers = nullptr;
		VkCommandBuffer* CommandBuffers = nullptr;

		// Todo struct depth buffer?
		VkImage DepthBufferImage = nullptr;
		VkDeviceMemory DepthBufferImageMemory = nullptr;
		VkImageView DepthBufferImageView = nullptr;

		VkDescriptorSetLayout DescriptorSetLayout = nullptr;
		VkPushConstantRange PushConstantRange;

		VkDescriptorPool DescriptorPool = nullptr;
		VkDescriptorSet* DescriptorSets = nullptr;	

		GenericBuffer* VpUniformBuffers = nullptr;
		//GenericBuffer* ModelDynamicUniformBuffers = nullptr;

		//uint32_t ModelUniformAlignment = 0;
		//UboModel* ModelTransferSpace = nullptr;

		VkPipeline GraphicsPipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
		VkRenderPass RenderPass = nullptr;

		struct UboViewProjection
		{
			glm::mat4 View;
			glm::mat4 Projection;
		} ViewProjection;

		const uint32_t MaxObjects = 2;
		uint32_t DrawableObjectsCount = 0;
		DrawableObject DrawableObjects[2];
		const int MaxFrameDraws = 2;
		int CurrentFrame = 0;
		
		VkCommandPool GraphicsCommandPool = nullptr;

		VkSemaphore* ImageAvalible = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		const uint32_t MaxTextures = 2;
		uint32_t TextureImagesCount = 0;
		// Todo: have single TextureImagesMemory and VkImage offset references in it
		ImageBuffer TextureImageBuffer[2];
	};

	bool InitVulkanRenderInstance(VulkanRenderInstance& RenderInstance, VkInstance VulkanInstance, GLFWwindow* Window);
	bool LoadMesh(VulkanRenderInstance& RenderInstance, Mesh Mesh);
	bool Draw(VulkanRenderInstance& RenderInstance);
	void DeinitVulkanRenderInstance(VulkanRenderInstance& RenderInstance);
	void CreateTexture(VulkanRenderInstance& RenderInstance, struct stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize);

	bool CreateGenericBuffer(const VulkanRenderInstance& RenderInstance, VkDeviceSize BufferSize,
		VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags BufferProperties, GenericBuffer& OutBuffer);

	void DestroyGenericBuffer(const VulkanRenderInstance& RenderInstance, GenericBuffer& Buffer);

	// Todo: Refactor CopyBuffer and CopyBufferToImage?
	void CopyBuffer(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkBuffer DstinationBuffer, VkDeviceSize BufferSize);

	void CopyBufferToImage(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkImage Image, uint32_t Width, uint32_t Height);

	VkImage CreateImage(const VulkanRenderInstance& RenderInstance, uint32_t Width, uint32_t Height,
		VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags,
		VkDeviceMemory* OutImageMemory);

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, uint32_t AllowedTypes, VkMemoryPropertyFlags Properties);

	VkImageView CreateImageView(const VulkanRenderInstance& RenderInstance, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);
}

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stb_image.h>

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


	struct PhysicalDeviceIndices
	{
		int GraphicsFamily = -1;
		int PresentationFamily = -1;
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

	struct GenericBuffer
	{
		VkBuffer Buffer = nullptr;
		VkDeviceMemory BufferMemory = nullptr;
	};

	struct ImageBuffer
	{
		VkImage TextureImage = nullptr;
		VkDeviceMemory TextureImagesMemory = nullptr;
		VkImageView TextureImageView = nullptr;
	};

	typedef glm::mat4 Model;

	struct DrawableObject
	{
		uint32_t VerticesCount = 0;
		GenericBuffer VertexBuffer;

		uint32_t IndicesCount = 0;
		GenericBuffer IndexBuffer;

		Model Model;

		uint32_t TextureId = 0;
	};

	struct GenericImageBuffer
	{
		VkImage Image = nullptr;
		VkDeviceMemory ImageMemory = nullptr;
		VkImageView ImageView = nullptr;
	};

	struct MainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct ViewportInstence
	{
		GLFWwindow* Window = nullptr;
		VkSurfaceKHR Surface = nullptr;

		VkExtent2D SwapExtent = {};
		VkSwapchainKHR VulkanSwapchain = nullptr;

		uint32_t SwapchainImagesCount = 0;
		VkImageView* ImageViews = nullptr;

		VkFramebuffer* SwapchainFramebuffers = nullptr;
		VkCommandBuffer* CommandBuffers = nullptr;
		GenericImageBuffer* ColorBuffers = nullptr;
		GenericImageBuffer* DepthBuffers = nullptr;

		VkDescriptorPool DescriptorPool = nullptr;
		VkDescriptorPool InputDescriptorPool = nullptr;
		VkDescriptorSet* DescriptorSets = nullptr;
		VkDescriptorSet* InputDescriptorSets = nullptr;

		VkPipeline GraphicsPipeline = nullptr;
		VkPipeline SecondPipeline = nullptr;

		GenericBuffer* VpUniformBuffers = nullptr;

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
		ViewportInstence* Viewports[2];
		ViewportInstence MainViewport;

		VkPhysicalDeviceProperties PhysicalDeviceProperties;
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;

		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentationQueue = nullptr;

		// Todo: pass as AddViewport params? 
		VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
		VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkPresentModeKHR PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
		PhysicalDeviceIndices PhysicalDeviceIndices = { };

		// Todo: put textures in DescriptorSets to use only one descriptor set and pool?
		VkDescriptorPool SamplerDescriptorPool = nullptr;
		VkDescriptorSetLayout SamplerSetLayout = nullptr;
		VkDescriptorSet* SamplerDescriptorSets = nullptr;

		VkPushConstantRange PushConstantRange;

		VkDescriptorSetLayout DescriptorSetLayout = nullptr;
		VkDescriptorSetLayout InputSetLayout = nullptr; // Set for diferent pipeline, goes to diferent shader

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipelineLayout SecondPipelineLayout = nullptr;

		//GenericBuffer* ModelDynamicUniformBuffers = nullptr;

		//uint32_t ModelUniformAlignment = 0;
		//UboModel* ModelTransferSpace = nullptr;

		VkRenderPass RenderPass = nullptr;

		const uint32_t MaxObjects = 528;
		uint32_t DrawableObjectsCount = 0;
		DrawableObject DrawableObjects[528];
		const int MaxFrameDraws = 2;
		int CurrentFrame = 0;

		VkCommandPool GraphicsCommandPool = nullptr;

		VkSemaphore* ImageAvalible = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		VkSampler TextureSampler = nullptr;
		// Todo: put all textures in atlases or texture layers
		const uint32_t MaxTextures = 64;
		uint32_t TextureImagesCount = 0;
		// Todo: have single TextureImagesMemory and VkImage offset references in it
		ImageBuffer TextureImageBuffer[64];
	};

	RequiredInstanceExtensionsData CreateRequiredInstanceExtensionsData(const char** ValidationExtensions, uint32_t ValidationExtensionsCount);
	void DestroyRequiredInstanceExtensionsData(RequiredInstanceExtensionsData& Data);

	ExtensionPropertiesData CreateAvailableExtensionPropertiesData();
	ExtensionPropertiesData CreateDeviceExtensionPropertiesData(VkPhysicalDevice PhysicalDevice);
	void DestroyExtensionPropertiesData(ExtensionPropertiesData& Data);

	AvailableInstanceLayerPropertiesData CreateAvailableInstanceLayerPropertiesData();
	void DestroyAvailableInstanceLayerPropertiesData(AvailableInstanceLayerPropertiesData& Data);

	QueueFamilyPropertiesData CreateQueueFamilyPropertiesData(VkPhysicalDevice PhysicalDevice);
	void DestroyQueueFamilyPropertiesData(QueueFamilyPropertiesData& Data);

	SurfaceFormatsData CreateSurfaceFormatsData(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	void DestroySurfaceFormatsData(SurfaceFormatsData& Data);

	PresentModeData CreatePresentModeData(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	void DestroyPresentModeData(PresentModeData& Data);



	MainInstance CreateMainInstance(RequiredInstanceExtensionsData RequiredExtensions,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize);
	void DestroyMainInstance(MainInstance& Instance);




	PhysicalDeviceIndices GetPhysicalDeviceIndices(QueueFamilyPropertiesData Data, VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);


	bool CheckRequiredInstanceExtensionsSupport(ExtensionPropertiesData AvailableExtensions,
		RequiredInstanceExtensionsData RequiredExtensions);
	bool CheckValidationLayersSupport(AvailableInstanceLayerPropertiesData& Data,
		const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize);
	bool CheckDeviceExtensionsSupport(ExtensionPropertiesData Data, const char** ExtensionsToCheck,
		uint32_t ExtensionsToCheckSize);








	// VulkanRenderInstance
	bool InitVulkanRenderInstance(VulkanRenderInstance& RenderInstance, VkInstance VulkanInstance, GLFWwindow* Window);
	bool LoadMesh(VulkanRenderInstance& RenderInstance, Mesh Mesh);
	bool Draw(VulkanRenderInstance& RenderInstance);
	void DeinitVulkanRenderInstance(VulkanRenderInstance& RenderInstance);
	void CreateTexture(VulkanRenderInstance& RenderInstance, stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize);
	bool AddViewport(VulkanRenderInstance& RenderInstance, GLFWwindow* Window);
	void RemoveViewport(VulkanRenderInstance& RenderInstance, GLFWwindow* Window);
}

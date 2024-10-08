#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stb_image.h>

#include <vector>

namespace Core
{
	typedef glm::mat4 Model;

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

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoords;
	};

	struct TerrainVertex
	{
		float Altitude;
	};

	struct GPUBuffer
	{
		static void DestroyGPUBuffer(VkDevice LogicalDevice, GPUBuffer& buffer);
		static GPUBuffer CreateIndexBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			VkCommandPool TransferCommandPool, VkQueue TransferQueue, uint32_t* Indices, uint32_t IndicesCount);
		static GPUBuffer CreateUniformBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkDeviceSize bufferSize);
		static GPUBuffer CreateIndirectBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			VkCommandPool TransferCommandPool, VkQueue TransferQueue, const std::vector<VkDrawIndexedIndirectCommand>& DrawCommands);
		static GPUBuffer CreateGPUBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties);

		static void CopyGPUBuffer(VkDevice LogicalDevice, VkCommandPool TransferCommandPool, VkQueue TransferQueue,
			const GPUBuffer& srcBuffer, GPUBuffer& dstBuffer);

		template <typename T>
		static GPUBuffer CreateVertexBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			VkCommandPool TransferCommandPool, VkQueue TransferQueue, T* Vertices, uint32_t VerticesCount)
		{
			const VkDeviceSize BufferSize = sizeof(T) * VerticesCount;

			GPUBuffer StagingBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			void* Data;
			vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
			memcpy(Data, Vertices, (size_t)BufferSize);
			vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

			GPUBuffer vertexBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Copy staging buffer to vertex buffer on GPU;
			GPUBuffer::CopyGPUBuffer(LogicalDevice, TransferCommandPool, TransferQueue, StagingBuffer, vertexBuffer);
			GPUBuffer::DestroyGPUBuffer(LogicalDevice, StagingBuffer);

			return vertexBuffer;
		}

		VkBuffer Buffer = 0;
		VkDeviceMemory Memory = 0;
		VkDeviceSize Size = 0;
	};

	struct Mesh
	{
		uint32_t MeshVerticesCount = 0;
		Vertex* MeshVertices = nullptr;

		uint32_t MeshIndicesCount = 0;
		uint32_t* MeshIndices = nullptr;

		Model Model = glm::mat4(1.0f);
	};

	struct ImageBuffer
	{
		VkImage Image = nullptr;
		VkDeviceMemory Memory = nullptr;
		VkImageView ImageView = nullptr;
	};

	struct DrawEntity
	{
		uint32_t VerticesCount = 0;
		GPUBuffer VertexBuffer;

		uint32_t IndicesCount = 0;
		GPUBuffer IndexBuffer;

		Model Model;

		uint32_t TextureId = 0;
	};

	struct DrawTerrainEntity
	{
		uint32_t VerticesCount = 0;
		GPUBuffer VertexBuffer;
	};

	struct TextureInfo
	{
		int Width = 0;
		int Height = 0;
		int Format = 0;
		stbi_uc* Data = nullptr;
	};

	struct DrawScene
	{
		UboViewProjection ViewProjection;

		DrawEntity* DrawEntities;
		uint32_t DrawEntitiesCount;

		DrawTerrainEntity* DrawTerrainEntities;
		uint32_t DrawTerrainEntitiesCount;
	};
	
	struct MainInstance
	{
		static MainInstance CreateMainInstance(const char** RequiredExtensions, uint32_t RequiredExtensionsCount,
			bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize);
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
		static void GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, std::vector<VkPresentModeKHR>& PresentModes);
		static void GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain, std::vector<VkImage>& Images);

		VkSwapchainKHR VulkanSwapchain = nullptr;
		uint32_t ImagesCount = 0;
		VkImageView* ImageViews = nullptr;
		VkExtent2D SwapExtent = { };
	};

	struct DeviceInstance
	{
		void Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
			uint32_t DeviceExtensionsSize);

		static void GetPhysicalDeviceList(VkInstance VulkanInstance, std::vector<VkPhysicalDevice>& DeviceList);
		static void GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice, std::vector<VkExtensionProperties>& Data);
		static void GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice, std::vector<VkQueueFamilyProperties>& Data);
		static PhysicalDeviceIndices GetPhysicalDeviceIndices(const std::vector<VkQueueFamilyProperties>& Properties,
			VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
		static bool CheckDeviceSuitability(const char* DeviceExtensions[], uint32_t DeviceExtensionsSize,
			VkExtensionProperties* ExtensionProperties, uint32_t ExtensionPropertiesCount, PhysicalDeviceIndices Indices,
			VkPhysicalDeviceFeatures AvailableFeatures);
		static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, uint32_t ExtensionPropertiesCount,
			const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize);

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

		VkFramebuffer* SwapchainFramebuffers = nullptr;
		VkCommandBuffer* CommandBuffers = nullptr;
	};

	struct TerrainSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout TerrainSetLayout = nullptr;
		VkDescriptorSet* TerrainSets = nullptr;
	};

	struct EntitySubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout SamplerSetLayout = nullptr;
		VkDescriptorSetLayout EntitySetLayout = nullptr;
		VkDescriptorSet* EntitySets = nullptr;
	};

	struct DeferredSubpass
	{
		void ClearResources(VkDevice LogicalDevice);

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipeline Pipeline = nullptr;

		VkDescriptorSetLayout DeferredLayout = nullptr;
		VkDescriptorSet* DeferredSets = nullptr;
	};

	struct MainRenderPass
	{
		static void GetPoolSizes(uint32_t TotalImagesCount, std::vector<VkDescriptorPoolSize>& TotalPassPoolSizes,
			uint32_t& TotalDescriptorCount);

		void ClearResources(VkDevice LogicalDevice, uint32_t ImagesCount);

		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void SetupPushConstants();
		void CreateSamplerSetLayout(VkDevice LogicalDevice);
		void CreateCommandPool(VkDevice LogicalDevice, uint32_t FamilyIndex);
		void CreateTerrainSetLayout(VkDevice LogicalDevice);
		void CreateEntitySetLayout(VkDevice LogicalDevice);
		void CreateDeferredSetLayout(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice);
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent);
		void CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, uint32_t ImagesCount, VkExtent2D SwapExtent,
			VkFormat DepthFormat, VkFormat ColorFormat);
		void CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
			uint32_t ImagesCount);
		void CreateSets(VkDevice LogicalDevice, VkDescriptorPool DescriptorPool, uint32_t ImagesCount);

		VkRenderPass RenderPass = nullptr;

		TerrainSubpass TerrainPass;
		EntitySubpass EntityPass;
		DeferredSubpass DeferredPass;

		VkCommandPool GraphicsCommandPool = nullptr;

		ImageBuffer* ColorBuffers = nullptr;
		ImageBuffer* DepthBuffers = nullptr;
		GPUBuffer* VpUniformBuffers = nullptr;
	};
}

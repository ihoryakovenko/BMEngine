#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stb_image.h>

#include <vector>

#include "MainRenderPass.h"

namespace Core
{
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
		static MainInstance CreateMainInstance(const char** RequiredExtensions, uint32_t RequiredExtensionsCount,
			bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize);
		static void DestroyMainInstance(MainInstance& Instance);

		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
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

		UboViewProjection ViewProjection;
	};

	struct MainRenderPass
	{
		void Init(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat, int GraphicsFamily, uint32_t MaxDrawFrames,
			VkExtent2D* SwapExtents, uint32_t SwapExtentsCount);

		void DeInit(VkDevice LogicalDevice);

		VkRenderPass RenderPass = nullptr;

		uint32_t MaxFrameDrawsCounter = 0;
		VkSemaphore* ImageAvailable = nullptr;
		VkSemaphore* RenderFinished = nullptr;
		VkFence* DrawFences = nullptr;

		VkPushConstantRange PushConstantRange;
		VkDescriptorSetLayout SamplerSetLayout = nullptr;
		VkDescriptorSetLayout DescriptorSetLayout = nullptr;
		VkDescriptorSetLayout InputSetLayout = nullptr; // Set for different pipeline, goes to different shader

		VkPipelineLayout PipelineLayout = nullptr;
		VkPipelineLayout SecondPipelineLayout = nullptr;

		VkPipeline GraphicsPipeline = nullptr;
		VkPipeline SecondPipeline = nullptr;

		VkCommandPool GraphicsCommandPool = nullptr;

	private:
		void CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
			VkSurfaceFormatKHR SurfaceFormat);
		void CreateSamplerSetLayout(VkDevice LogicalDevice);
		void CreateCommandPool(VkDevice LogicalDevice, uint32_t FamilyIndex);
		void CreateSynchronisation(VkDevice LogicalDevice, uint32_t MaxFrameDraws);
		void DestroySynchronisation(VkDevice LogicalDevice);
		void CreateDescriptorSetLayout(VkDevice LogicalDevice);
		void CreateInputSetLayout(VkDevice LogicalDevice);
		void CreatePipelineLayouts(VkDevice LogicalDevice);
		void CreatePipelines(VkDevice LogicalDevice, VkExtent2D* SwapExtents, uint32_t SwapExtentsCount);
	};
}

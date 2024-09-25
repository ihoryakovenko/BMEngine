#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	bool CreateShader(VkDevice LogicalDevice, const uint32_t* Code, uint32_t CodeSize,
		VkShaderModule& VertexShaderModule);

	VkImageView CreateImageView(VkDevice LogicalDevice, VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags);

	VkDescriptorPool CreateDescriptorPool(VkDevice LogicalDevice, VkDescriptorPoolSize* PoolSizes,
		uint32_t PoolSizeCount, uint32_t MaxDescriptorCount);

	uint32_t GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, uint32_t AllowedTypes, VkMemoryPropertyFlags Properties);

	VkImage CreateImage(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, uint32_t Width, uint32_t Height,
		VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags,
		VkDeviceMemory* OutImageMemory);

	void CreateDescriptorSets(VkDevice LogicalDevice, VkDescriptorPool DescriptorPool, VkDescriptorSetLayout* Layouts,
		uint32_t DescriptorSetCount, VkDescriptorSet* OutSets);
}

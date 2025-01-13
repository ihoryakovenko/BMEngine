#pragma once

#include "BMR/BMRInterface.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct EntityVertex
{
	glm::vec3 Position;
	glm::vec3 Color;
	glm::vec2 TextureCoords;
	glm::vec3 Normal;
};

struct TerrainVertex
{
	f32 Altitude;
};

struct SkyBoxVertex
{
	glm::vec3 Position;
};

struct QuadSphereVertex
{
	glm::vec3 Position;
	glm::vec2 TextureCoords;
};

extern BMRVulkan::BMRVertexInput EntityVertexInput;
extern BMRVulkan::BMRVertexInput TerrainVertexInput;
extern BMRVulkan::BMRVertexInput SkyBoxVertexInput;
extern BMRVulkan::BMRVertexInput DepthVertexInput;
extern BMRVulkan::BMRVertexInput QuadSphereVertexInput;

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

extern BMRVulkan::BMRPipelineSettings EntityPipelineSettings;
extern BMRVulkan::BMRPipelineSettings TerrainPipelineSettings;
extern BMRVulkan::BMRPipelineSettings DeferredPipelineSettings;
extern BMRVulkan::BMRPipelineSettings SkyBoxPipelineSettings;
extern BMRVulkan::BMRPipelineSettings DepthPipelineSettings;
extern BMRVulkan::BMRPipelineSettings MapPipelineSettings;

static const u32 MainPathPipelinesCount = 5;

extern BMRVulkan::BMRRenderPassSettings MainRenderPassSettings;
extern BMRVulkan::BMRRenderPassSettings DepthRenderPassSettings;

static VkDescriptorType VpDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
static VkDescriptorType EntityLightDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
static VkDescriptorType LightSpaceMatrixDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
static VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
static VkDescriptorType DeferredInputDescriptorType[2] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT };
static VkDescriptorType ShadowMapArrayDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
static VkDescriptorType EntitySamplerDescriptorType[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
static VkDescriptorType TerrainSkyBoxSamplerDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

static VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
static VkShaderStageFlags EntityLightStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
static VkShaderStageFlags LightSpaceMatrixStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
static VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
static VkShaderStageFlags DeferredInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
static VkShaderStageFlags ShadowMapArrayFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
static VkShaderStageFlags EntitySamplerInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
static VkShaderStageFlags TerrainSkyBoxArrayFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

extern VkClearValue MainPassClearValues[3];
extern VkClearValue DepthPassClearValues;

extern VkImageCreateInfo DeferredInputDepthUniformCreateInfo;
extern VkImageCreateInfo DeferredInputColorUniformCreateInfo;
extern VkImageCreateInfo ShadowMapArrayCreateInfo;

extern BMRVulkan::BMRUniformImageInterfaceCreateInfo DeferredInputDepthUniformInterfaceCreateInfo;
extern BMRVulkan::BMRUniformImageInterfaceCreateInfo DeferredInputUniformColorInterfaceCreateInfo;
extern BMRVulkan::BMRUniformImageInterfaceCreateInfo ShadowMapArrayInterfaceCreateInfo;
extern BMRVulkan::BMRUniformImageInterfaceCreateInfo ShadowMapElement1InterfaceCreateInfo;
extern BMRVulkan::BMRUniformImageInterfaceCreateInfo ShadowMapElement2InterfaceCreateInfo;

extern VkSamplerCreateInfo ShadowMapSamplerCreateInfo;
extern VkSamplerCreateInfo DiffuseSamplerCreateInfo;
extern VkSamplerCreateInfo SpecularSamplerCreateInfo;

extern VkBufferCreateInfo VpBufferInfo;

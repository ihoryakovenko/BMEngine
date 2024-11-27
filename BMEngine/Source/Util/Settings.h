#pragma once

#include "BMR/BMRInterfaceTypes.h"

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

extern BMR::BMRVertexInput EntityVertexInput;
extern BMR::BMRVertexInput TerrainVertexInput;
extern BMR::BMRVertexInput SkyBoxVertexInput;
extern BMR::BMRVertexInput DepthVertexInput;

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

extern BMR::BMRPipelineSettings EntityPipelineSettings;
extern BMR::BMRPipelineSettings TerrainPipelineSettings;
extern BMR::BMRPipelineSettings DeferredPipelineSettings;
extern BMR::BMRPipelineSettings SkyBoxPipelineSettings;
extern BMR::BMRPipelineSettings DepthPipelineSettings;

extern BMR::BMRRenderPassSettings MainRenderPassSettings;
extern BMR::BMRRenderPassSettings DepthRenderPassSettings;

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

extern BMR::BMRUniformImageInterfaceCreateInfo DeferredInputDepthUniformInterfaceCreateInfo;
extern BMR::BMRUniformImageInterfaceCreateInfo DeferredInputUniformColorInterfaceCreateInfo;
extern BMR::BMRUniformImageInterfaceCreateInfo ShadowMapArrayInterfaceCreateInfo;
extern BMR::BMRUniformImageInterfaceCreateInfo ShadowMapElement1InterfaceCreateInfo;
extern BMR::BMRUniformImageInterfaceCreateInfo ShadowMapElement2InterfaceCreateInfo;

extern VkSamplerCreateInfo ShadowMapSamplerCreateInfo;
extern VkSamplerCreateInfo DiffuseSamplerCreateInfo;
extern VkSamplerCreateInfo SpecularSamplerCreateInfo;

extern VkBufferCreateInfo VpBufferInfo;

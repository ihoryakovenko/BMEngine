#pragma once

#include <Windows.h>

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

namespace BMR
{
	bool Init(HWND WindowHandler, const BMRConfig& InConfig);
	void DeInit();

	void TestSetRendeRpasses(BMRRenderPass Main, BMRRenderPass Depth,
		BMRUniform* inVpBuffer, VkDescriptorSetLayout iVpLayout, VkDescriptorSet* iVpSet,
		BMRUniform* inEntityLight, VkDescriptorSetLayout iEntityLightLayout, VkDescriptorSet* iEntityLightSet,
		BMRUniform* iLightSpaceBuffer, VkDescriptorSetLayout iLightSpaceLayout, VkDescriptorSet* iLightSpaceSet,
		BMRUniform iMaterial, VkDescriptorSetLayout iMaterialLayout, VkDescriptorSet iMaterialSet,
		VkImageView* iDeferredInputDepthImage, VkImageView* iDeferredInputColorImage,
		VkDescriptorSetLayout iDeferredInputLayout, VkDescriptorSet* iDeferredInputSet,
		BMRUniform* iShadowArray, VkDescriptorSetLayout iShadowArrayLayout, VkDescriptorSet* iShadowArraySet,
		VkDescriptorSetLayout iEntitySamplerLayout, VkDescriptorSetLayout iTerrainSkyBoxSamplerLayout);

	u32 GetImageCount();
	VkImageView* GetSwapchainImageViews();

	void CreateRenderPass(const BMRRenderPassSettings* Settings, const BMRRenderTarget* Targets,
		VkExtent2D TargetExtent, u32 TargetCount, u32 SwapchainImagesCount, BMRRenderPass* OutPass);
	BMRUniform CreateUniformBuffer(const VkBufferCreateInfo* BufferInfo, VkMemoryPropertyFlags Properties);
	BMRUniform CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo);
	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages, u32 Count);
	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets);
	VkImageView CreateImageInterface(const BMRUniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image);
	VkSampler CreateSampler(const VkSamplerCreateInfo* CreateInfo);

	void DestroyRenderPass(BMRRenderPass* Pass);
	void DestroyUniformBuffer(BMRUniform Buffer);
	void DestroyUniformImage(BMRUniform Image);
	void DestroyUniformLayout(VkDescriptorSetLayout Layout);
	void DestroyImageInterface(VkImageView Interface);
	void DestroySampler(VkSampler Sampler);

	void AttachUniformsToSet(VkDescriptorSet Set, const BMRUniformSetAttachmentInfo* Infos, u32 BufferCount);
	void UpdateUniformBuffer(BMRUniform Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, u32 LayersCount, void* Data);
	
	
	void UpdateMaterialBuffer(const BMRMaterial* Buffer);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void Draw(const BMRDrawScene& Scene);
}
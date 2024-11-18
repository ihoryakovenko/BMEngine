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


	
	
	void UpdateMaterialBuffer(const BMRMaterial* Buffer);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void Draw(const BMRDrawScene& Scene);
}
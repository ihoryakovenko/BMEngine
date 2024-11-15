#pragma once

#include <Windows.h>

#include "Util/EngineTypes.h"
#include "BMRInterfaceTypes.h"

static const u32 MAX_SWAPCHAIN_IMAGES_COUNT = 3;

namespace BMR
{
	bool Init(HWND WindowHandler, const BMRConfig& InConfig);
	void DeInit();

	void TestSetRendeRpasses(BMRRenderPass Main, BMRRenderPass Depth,
		BMRUniform* inVpBuffer, BMRUniformLayout iVpLayout, BMRUniformSet* iVpSet,
		BMRUniform* inEntityLight, BMRUniformLayout iEntityLightLayout, BMRUniformSet* iEntityLightSet,
		BMRUniform* iLightSpaceBuffer, BMRUniformLayout iLightSpaceLayout, BMRUniformSet* iLightSpaceSet,
		BMRUniform iMaterial, BMRUniformLayout iMaterialLayout, BMRUniformSet iMaterialSet);

	u32 GetImageCount();

	BMRRenderPass CreateRenderPass(const BMRRenderPassSettings* Settings);
	BMRUniform CreateUniformBuffer(VkBufferUsageFlags Type, VkMemoryPropertyFlags Usage, VkDeviceSize Size);
	BMRUniform CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo);
	BMRUniformLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages, u32 Count);
	void CreateUniformSets(const BMRUniformLayout* Layouts, u32 Count, BMRUniformSet* OutSets);
	BMRImageInterface CreateImageInterface(const VkImageViewCreateInfo* ViewCreateInfo);

	void DestroyRenderPass(BMRRenderPass Pass);
	void DestroyUniformBuffer(BMRUniform Buffer);
	void DestroyUniformImage(BMRUniform Image);
	void DestroyUniformLayout(BMRUniformLayout Layout);
	void DestroyImageInterface(BMRImageInterface Interface);

	void AttachUniformsToSet(BMRUniformSet Set, const BMRUniformSetAttachmentInfo* Infos, u32 BufferCount);
	void UpdateUniformBuffer(BMRUniform Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data);
	
	
	void UpdateMaterialBuffer(const BMRMaterial* Buffer);
	u32 LoadTexture(BMRTextureArrayInfo Info, BMRTextureType TextureType);
	u32 LoadEntityMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex);
	u32 LoadTerrainMaterial(u32 DiffuseTextureIndex);
	u32 LoadSkyBoxMaterial(u32 CubeTextureIndex);
	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount);
	u64 LoadIndices(const u32* Indices, u32 IndicesCount);

	void Draw(const BMRDrawScene& Scene);
}
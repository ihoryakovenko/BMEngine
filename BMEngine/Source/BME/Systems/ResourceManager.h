#pragma once

#include <string>
#include <vector>

#include "Util/EngineTypes.h"
#include "BMR/BMRInterface.h"

namespace ResourceManager
{
	void Init();
	void DeInit();

	BMR::BMRTexture LoadTexture(const std::string& Id, const std::vector<std::string>& PathNames,
		VkImageViewType Type, VkImageCreateFlags Flags = 0);

	BMR::BMRTexture EmptyTexture(const std::string& Id, u32 Width, u32 Height, u32 Layers,
		VkImageViewType Type, VkImageCreateFlags Flags = 0);

	void LoadToTexture(BMR::BMRTexture* Texture, const std::vector<std::string>& PathNames);

	VkDescriptorSet FindMaterial(const std::string& Id);
	BMR::BMRTexture* FindTexture(const std::string& Id);

	// Test
	void CreateEntityMaterial(const std::string& Id, VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach);
	void CreateSkyBoxTerrainTexture(const std::string& Id, VkImageView DefuseImage, VkDescriptorSet* SetToAttach);
}
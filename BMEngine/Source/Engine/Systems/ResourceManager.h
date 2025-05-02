#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "Util/EngineTypes.h"

#include "Render/Render.h"

namespace ResourceManager
{
	extern u64 DefaultTextureHash;
	extern const u8 DefaultTextureData[];
	extern u64 DefaultTextureDataCount;

	void Init();
	void DeInit();

	Render::RenderTexture LoadTexture(const std::string& Id, const std::vector<std::string>& PathNames,
		VkImageViewType Type, VkImageCreateFlags Flags = 0);

	Render::RenderTexture EmptyTexture(const std::string& Id, u32 Width, u32 Height, u32 Layers,
		VkImageViewType Type, VkImageCreateFlags Flags = 0);

	void LoadToTexture(Render::RenderTexture* Texture, const std::vector<std::string>& PathNames);

	VkDescriptorSet FindMaterial(const std::string& Id);
	Render::RenderTexture* FindTexture(const std::string& Id);
	Render::RenderTexture* FindTexture(u64 Hash);

	// Test
	void CreateSkyBoxTerrainTexture(const std::string& Id, VkImageView DefuseImage, VkDescriptorSet* SetToAttach);

	std::vector<VkImageView> TestGetAllImages();
}
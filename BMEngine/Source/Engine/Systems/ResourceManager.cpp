#include "ResourceManager.h"

#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ResourceManager
{
	static const std::string TexturesPath = "./Resources/Textures/";

	static std::map<std::string, Render::RenderTexture> Textures;
	static std::map<std::string, VkDescriptorSet> EngineMaterials;

	void Init()
	{
		// Default resources
		Render::RenderTexture TestTexture = LoadTexture("1giraffe", std::vector<std::string> {"1giraffe.jpg"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture WhiteTexture = LoadTexture("White", std::vector<std::string> {"White.png"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture ContainerTexture = LoadTexture("container2", std::vector<std::string> {"container2.png"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture ContainerSpecularTexture = LoadTexture("container2_specular", std::vector<std::string> {"container2_specular.png"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture BlendWindow = LoadTexture("blending_transparent_window", std::vector<std::string> {"blending_transparent_window.png"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture GrassTexture = LoadTexture("grass", std::vector<std::string> {"grass.png"}, VK_IMAGE_VIEW_TYPE_2D);
		Render::RenderTexture SkyBoxCubeTexture = LoadTexture("skybox", std::vector<std::string> {
			"skybox/right.jpg",
				"skybox/left.jpg",
				"skybox/top.jpg",
				"skybox/bottom.jpg",
				"skybox/front.jpg",
				"skybox/back.jpg", }, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

		VkDescriptorSet TestMaterial;
		VkDescriptorSet WhiteMaterial;
		VkDescriptorSet ContainerMaterial;
		VkDescriptorSet BlendWindowMaterial;
		VkDescriptorSet GrassMaterial;
		VkDescriptorSet SkyBoxMaterial;
		VkDescriptorSet TerrainMaterial;

		CreateEntityMaterial("TestMaterial", TestTexture.ImageView, TestTexture.ImageView, &TestMaterial);
		CreateEntityMaterial("WhiteMaterial", WhiteTexture.ImageView, WhiteTexture.ImageView, &WhiteMaterial);
		CreateEntityMaterial("ContainerMaterial", ContainerTexture.ImageView, ContainerSpecularTexture.ImageView, &ContainerMaterial);
		CreateEntityMaterial("BlendWindowMaterial", BlendWindow.ImageView, BlendWindow.ImageView, &BlendWindowMaterial);
		CreateEntityMaterial("GrassMaterial", GrassTexture.ImageView, GrassTexture.ImageView, &GrassMaterial);
	}

	void DeInit()
	{
		for (auto& Texture : Textures)
		{
			Render::DestroyTexture(&Texture.second);
		}
	}

	Render::RenderTexture LoadTexture(const std::string& Id, const std::vector<std::string>& PathNames,
		VkImageViewType Type, VkImageCreateFlags Flags)
	{
		assert(PathNames.size() > 0);
		std::vector<stbi_uc*> ImageData(PathNames.size());

		int Width = 0;
		int Height = 0;
		int Channels = 0;

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			ImageData[i] = stbi_load((TexturesPath + PathNames[i]).c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);

			if (ImageData[i] == nullptr)
			{
				assert(false);
			}
		}

		Render::TextureArrayInfo Info;
		Info.Width = Width;
		Info.Height = Height;
		Info.Format = STBI_rgb_alpha;
		Info.LayersCount = PathNames.size();
		Info.Data = ImageData.data();
		Info.ViewType = Type;
		Info.Flags = Flags;

		Render::RenderTexture Texture = Render::CreateTexture(&Info);

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			stbi_image_free(ImageData[i]);
		};

		Textures[Id] = Texture;
		return Texture;
	}

	Render::RenderTexture EmptyTexture(const std::string& Id, u32 Width, u32 Height, u32 Layers,
		VkImageViewType Type, VkImageCreateFlags Flags)
	{
		Render::TextureArrayInfo Info;
		Info.Width = Width;
		Info.Height = Height;
		Info.Format = STBI_rgb_alpha;
		Info.LayersCount = Layers;
		Info.ViewType = Type;
		Info.Flags = Flags;

		Render::RenderTexture Texture = Render::CreateEmptyTexture(&Info);

		Textures[Id] = Texture;
		return Texture;
	}

	void LoadToTexture(Render::RenderTexture* Texture, const std::vector<std::string>& PathNames)
	{
		assert(PathNames.size() > 0);
		std::vector<stbi_uc*> ImageData(PathNames.size());

		int Width = 0;
		int Height = 0;
		int Channels = 0;

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			ImageData[i] = stbi_load((TexturesPath + PathNames[i]).c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);

			if (ImageData[i] == nullptr)
			{
				assert(false);
			}
		}

		Render::TextureArrayInfo Info;
		Info.Width = Width;
		Info.Height = Height;
		Info.Format = STBI_rgb_alpha;
		Info.LayersCount = PathNames.size();
		Info.Data = ImageData.data();

		Render::UpdateTexture(Texture, &Info);

		for (u32 i = 0; i < PathNames.size(); ++i)
		{
			stbi_image_free(ImageData[i]);
		};
	}

	VkDescriptorSet FindMaterial(const std::string& Id)
	{
		auto it = EngineMaterials.find(Id);
		if (it != EngineMaterials.end())
		{
			return it->second;
		}

		return nullptr;
	}

	Render::RenderTexture* FindTexture(const std::string& Id)
	{
		auto it = Textures.find(Id);
		if (it != Textures.end())
		{
			return &it->second;
		}

		return nullptr;
	}

	void CreateEntityMaterial(const std::string& Id, VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach)
	{
		Render::TestAttachEntityTexture(DefuseImage, SpecularImage, SetToAttach);
		EngineMaterials[Id] = *SetToAttach;
	}

	void CreateSkyBoxTerrainTexture(const std::string& Id, VkImageView DefuseImage, VkDescriptorSet* SetToAttach)
	{
		Render::TestAttachSkyNoxTerrainTexture(DefuseImage, SetToAttach);
		EngineMaterials[Id] = *SetToAttach;
	}
}
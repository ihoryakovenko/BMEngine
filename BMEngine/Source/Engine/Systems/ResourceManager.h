#pragma once

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

	template<typename T>
	struct ResourceInitializer
	{
		static inline std::unordered_map<std::string_view, T*> RegisteredResources;
		static inline std::unordered_map<std::string_view, std::vector<void(*)(T*)>> Delegates;

		static void Register(const std::string_view& Name, T* Resource)
		{
			auto RegisteredResourceIterator = RegisteredResources.find(Name);
			assert(RegisteredResourceIterator == RegisteredResources.end() && "Resource already set");
			RegisteredResources[Name] = Resource;

			if (auto DelegateIterator = Delegates.find(Name); DelegateIterator != Delegates.end())
			{
				for (auto& Function : DelegateIterator->second)
					Function(Resource);
				Delegates.erase(DelegateIterator);
			}
		}

		static void Request(const std::string_view& Name, void(*DelegateFunction)(T*))
		{
			if (auto it = RegisteredResources.find(Name); it != RegisteredResources.end())
			{
				DelegateFunction(it->second);
			}
			else
			{
				Delegates[Name].push_back(DelegateFunction);
			}
		}
	};

	template<typename T>
	struct ResourceInitializer2
	{
		enum Resources
		{
			DrawEntities = 0,

			Count
		};

		static inline T* RegisteredResources[Resources::Count] = { };
		static inline std::vector<void(*)(T*)> Delegates[Resources::Count];


		static void Register(Resources ResourceId, T* Resource)
		{
			assert(RegisteredResources[ResourceId] == nullptr && "Resource already set");
			RegisteredResources[ResourceId] = Resource;

			for (auto Delegate : Delegates[ResourceId])
			{
				Delegate(Resource);
			}

			Delegates[ResourceId].clear();
		}

		static void Request(Resources ResourceId, void(*DelegateFunction)(T*))
		{
			if (RegisteredResources[ResourceId] != nullptr)
			{
				DelegateFunction(RegisteredResources[ResourceId]);
			}
			else
			{
				Delegates[ResourceId].push_back(DelegateFunction);
			}
		}
	};

}
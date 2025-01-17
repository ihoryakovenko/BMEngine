#include "DynamicMapSystem.h"

#include <vector>
#include <string>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace DynamicMapSystem
{
	static void UpdateTilesData(s32 TextureTilesPerAxis);

	static s32 LonToTileX(f64 lon, s32 z);
	static s32 LatToTileY(f64 lat, s32 z);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const s32 MaxTileZoom = 5;

	static s32 VertexZoom = 1;
	static s32 TileZoom = 4;

	static bool UpdateVertexDataFlag = false;
	static bool UpdateTileDataFlag = false;

	static s32 CameraLat = 0;
	static s32 CameraLon = 0;

	void Init()
	{
		const s32 VertexTilesPerAxis = Math::GetTilesPerAxis(VertexZoom);
		const s32 TextureTilesPerAxis = Math::GetTilesPerAxis(TileZoom);
		const s32 MaxTextureTilesPerAxis = Math::GetTilesPerAxis(MaxTileZoom);

		BMR::BMRTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, 256, 256,
			MaxTextureTilesPerAxis * MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);


		VkDescriptorSet TilesMaterial;
		ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
		Scene.MapEntity.TextureSet = TilesMaterial;

		UpdateTilesData(TextureTilesPerAxis);
	}

	struct Tile
	{
		int x;      // Tile X index
		int y;      // Tile Y index
		int Zoom;   // Zoom level
	};

	static s32 LonToTileX(f64 lon, s32 z)
	{
		return (s32)(floor((lon + 180.0) / 360.0 * (1 << z)));
	}

	static s32 LatToTileY(f64 lat, s32 z)
	{
		f64 LatRad = lat * glm::pi<f64>() / 180.0;
		return (s32)(floor((1.0 - asinh(tan(LatRad)) / glm::pi<f64>()) / 2.0 * (1 << z)));
	}

	void Update(const MapCamera& Camera, s32 Zoom)
	{
		//const s32 NewVertexZoom = Zoom > 1.0f ? Zoom : 1.0f;

		//bool IsZoomChanged = false;
		//if (VertexZoom != NewVertexZoom)
		//{
		//	VertexZoom = NewVertexZoom;
		//	IsZoomChanged = true;
		//}

		VertexZoom = Zoom;

		const s32 VertexTilesPerAxis = Math::GetTilesPerAxis(VertexZoom);

		std::vector<Tile> visibleTiles;

		f32 halfVSide = std::tan(glm::radians(Camera.fov / 2.0f)) * glm::length(Camera.position);
		f32 halfHSide = halfVSide * Camera.aspectRatio;

		glm::vec3 nearCenter = Camera.position + Camera.front * glm::length(Camera.position);
		glm::vec3 right = glm::cross(Camera.front, Camera.up);

		glm::vec3 frustumCorners[] = {
			nearCenter + Camera.up * halfVSide - right * halfHSide, // Top-left
			nearCenter + Camera.up * halfVSide + right * halfHSide, // Top-right
			nearCenter - Camera.up * halfVSide - right * halfHSide, // Bottom-left
			nearCenter - Camera.up * halfVSide + right * halfHSide  // Bottom-right
		};

		bool IsIntersected = false;
		double minLat = 85.0, maxLat = -86.0, minLon = 180.0, maxLon = -180.0;

		for (const auto& corner : frustumCorners)
		{
			glm::vec3 intersection;
			if (Math::RaySphereIntersection(Camera.position, glm::normalize(corner - Camera.position), 1.0f, intersection))
			{
				IsIntersected = true;

				double lat = glm::degrees(asin(intersection.y));
				double lon = glm::degrees(atan2(intersection.x, intersection.z));

				minLat = std::min<>(minLat, lat);
				maxLat = std::max<>(maxLat, lat);
				minLon = std::min<>(minLon, lon);
				maxLon = std::max<>(maxLon, lon);
			}
		}

		//if (!IsIntersected)
		//{
		//	std::swap(minLon, maxLon);
		//	std::swap(minLat, maxLat);
		//}

		int minTileX = LonToTileX(minLon, Zoom);
		int maxTileX = LonToTileX(maxLon, Zoom);
		int minTileY = LatToTileY(maxLat, Zoom);
		int maxTileY = LatToTileY(minLat, Zoom);

		//if (minTileY > maxTileY)
		//{
		//	std::swap(minTileY, maxTileY);
		//}

		//if (minTileX > maxTileX)
		//{
		//	std::swap(minTileX, maxTileX);
		//}

		if (IsIntersected)
		{
			maxTileX += (maxTileX > 0) ? 1 : -1;
			maxTileY += (maxTileY > 0) ? 1 : -1;
		}

		std::vector<u32> Indices;
		Indices.reserve(((minTileX + maxTileX) / 2) * ((minTileY + maxTileY) / 2) * 6);

		for (int x = minTileX; x < maxTileX; ++x)
		{
			for (int y = minTileY; y < maxTileY; ++y)
			{
				// Clamp tile indices to valid ranges
				int validX = (x + (1 << Zoom)) % (1 << Zoom); // Wrap X around at 2^Zoom
				int validY = glm::clamp(y, 0, (1 << Zoom) - 1);

				visibleTiles.push_back({ validX, validY, Zoom });

				s32 first = validY * (VertexTilesPerAxis + 1) + validX;
				s32 second = first + (VertexTilesPerAxis + 1);

				Indices.push_back(first);
				Indices.push_back(second);
				Indices.push_back(first + 1);

				Indices.push_back(second);
				Indices.push_back(second + 1);
				Indices.push_back(first + 1);
			}
		}

		BMR::ClearIndices();
		Scene.MapEntity.IndexOffset = BMR::LoadIndices(Indices.data(), Indices.size());
		Scene.MapEntity.IndicesCount = Indices.size();
		Scene.MapTileSettings.VertexTilesPerAxis = VertexTilesPerAxis;

		if (UpdateTileDataFlag)
		{
			const u32 TextureTilesPerAxis = Math::GetTilesPerAxis(TileZoom);
			UpdateTilesData(TextureTilesPerAxis);
		}
	}

	void DeInit()
	{
	}

	void UpdateTilesData(s32 TextureTilesPerAxis)
	{
		std::vector<std::string> OsmTiles;
		const std::string BasePath = "osm_tiles_Zoom" + std::to_string(TileZoom) +
			"/tile_" + std::to_string(TileZoom) + '_';

		for (int i = 0; i < TextureTilesPerAxis; ++i)
		{
			for (int j = 0; j < TextureTilesPerAxis; ++j)
			{
				std::string FileName = BasePath + std::to_string(j) + "_" + std::to_string(i) + ".png";
				OsmTiles.emplace_back(std::move(FileName));
			}
		}

		BMR::BMRTexture* Texture = ResourceManager::FindTexture(TilesTextureId);
		assert(Texture);
		ResourceManager::LoadToTexture(Texture, OsmTiles);

		Scene.MapTileSettings.TextureTilesPerAxis = TextureTilesPerAxis;

		UpdateTileDataFlag = false;
	}
}

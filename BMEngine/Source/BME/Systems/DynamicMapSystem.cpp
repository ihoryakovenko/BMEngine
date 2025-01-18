#include "DynamicMapSystem.h"

#include <vector>
#include <string>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

namespace DynamicMapSystem
{
	static void UpdateTilesData(s32 TextureTilesPerAxis);

	static s32 LonToTileX(f64 lon, s32 z);
	static s32 LatToTileY(f64 lat, s32 z);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const s32 MaxTileZoom = 5;

	static const f32 SphereRadius = 1.0f;

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
		s32 x;      // Tile X index
		s32 y;      // Tile Y index
		s32 Zoom;   // Zoom level
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

		const f32 HalfVSide = std::tan(glm::radians(Camera.fov / 2.0f)) * glm::length(Camera.position);
		const f32 HalfHSide = HalfVSide * Camera.aspectRatio;

		const glm::vec3 NearCenter = Camera.position + Camera.front * glm::length(Camera.position);
		const glm::vec3 Right = glm::cross(Camera.front, Camera.up);

		//f32 minLat = -90.0;
		//f32 maxLat = 90.0;
		//f32 minLon = -180.0;
		//f32 maxLon = 180.0;

		f32 minLat = 90.0;
		f32 maxLat = -90.0;
		f32 minLon = 180.0;
		f32 maxLon = -180.0;

		const glm::vec3 FrustumCorners[] = {
			NearCenter + Camera.up * HalfVSide - Right * HalfHSide,  // Top-left
			NearCenter + Camera.up * HalfVSide + Right * HalfHSide,  // Top-right
			NearCenter - Camera.up * HalfVSide - Right * HalfHSide,  // Bottom-left
			NearCenter - Camera.up * HalfVSide + Right * HalfHSide,   // Bottom-right
			NearCenter + Camera.up * HalfVSide,		// Up
			NearCenter - Camera.up * HalfVSide,		// Down
		};

		s32 IntersectionCounter = 0;
		glm::vec3 Intersection;
		for (const glm::vec3& Corner : FrustumCorners)
		{
			if (Math::RaySphereIntersection(Camera.position, glm::normalize(Corner - Camera.position), SphereRadius, Intersection))
			{
				const f32 Lat = glm::degrees(asin(Intersection.y / SphereRadius));
				const f32 Lon = glm::degrees(atan2(Intersection.x, Intersection.z));

				minLat = std::min<>(minLat, Lat);
				maxLat = std::max<>(maxLat, Lat);
				minLon = std::min<>(minLon, Lon);
				maxLon = std::max<>(maxLon, Lon);

				++IntersectionCounter;
			}
		}

		auto norm = glm::normalize(Camera.position);
		//auto norm = glm::normalize(Camera.position + Camera.up * HalfVSide);
		const f32 Lat = glm::degrees(asin(norm.y / SphereRadius));
		const f32 Lon = glm::degrees(atan2(norm.x, norm.z));

		s32 centerTileX = LonToTileX(Lon, Zoom);
		s32 centerTileY = LatToTileY(Lat, Zoom);

		float hfov = Camera.fov * Camera.aspectRatio; // Horizontal FOV in degrees

		float verticalAngle = glm::degrees(2.0f * atan(tan(glm::radians(Camera.fov * 0.5f)) * (SphereRadius / (SphereRadius + Camera.altitude))));
		float horizontalAngle = glm::degrees(2.0f * atan(tan(glm::radians(hfov * 0.5f)) * (SphereRadius / (SphereRadius + Camera.altitude))));

		float tileAngularSize = 360.0f / (1 << Zoom);
		tileAngularSize = tileAngularSize * cos(glm::radians(Lat));

		s32 tilesX = ceil(horizontalAngle / tileAngularSize);
		s32 tilesY = ceil(verticalAngle / tileAngularSize);

		s32 minTileX = centerTileX - tilesX / 2;
		s32 maxTileX = centerTileX + tilesX / 2;

		s32 minTileY = centerTileY - tilesY / 2;
		s32 maxTileY = centerTileY + tilesY / 2;

		//s32 minTileX = LonToTileX(minLon, Zoom);
		//s32 maxTileX = LonToTileX(maxLon, Zoom);
		//s32 minTileY = LatToTileY(maxLat, Zoom);
		//s32 maxTileY = LatToTileY(minLat, Zoom);

		if (IntersectionCounter > 5)
		{
			if (maxLon - minLon > 180.0f)
			{
				maxTileX = LonToTileX(maxLon - 360.0f, Zoom);
				if (maxTileX < minTileX)
				{
					std::swap(minTileX, maxTileX);
					//minTileX = LonToTileX(minLon + 360.0f, Zoom);
					//maxTileX = -minTileX;
				}
				//minTileX -= 2;
			}

			maxTileX += (maxTileX >= 0) ? 1 : -1;
			maxTileY += (maxTileY > 0) ? 1 : -1;
		}
		else
		{
			minTileX = 0;
			maxTileX = VertexTilesPerAxis;
			minTileY = 0;
			maxTileY = VertexTilesPerAxis;
		}

		std::vector<u32> Indices;
		//Indices.reserve(((minTileX + maxTileX) / 2) * ((minTileY + maxTileY) / 2) * 6);

		std::vector<Tile> visibleTiles;

		for (s32 x = minTileX; x < maxTileX; ++x)
		{
			const s32 ValidX = (x + (1 << Zoom)) % (1 << Zoom);

			for (s32 y = minTileY; y < maxTileY; ++y)
			{
				const s32 ValidY = glm::clamp(y, 0, (1 << Zoom) - 1);

				visibleTiles.push_back({ ValidX, ValidY, Zoom });

				const s32 First = ValidY * (VertexTilesPerAxis + 1) + ValidX;
				const s32 Second = First + (VertexTilesPerAxis + 1);

				Indices.push_back(First);
				Indices.push_back(Second);
				Indices.push_back(First + 1);

				Indices.push_back(Second);
				Indices.push_back(Second + 1);
				Indices.push_back(First + 1);
			}
		}

		if (Indices.empty())
		{
			return;
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

		for (s32 i = 0; i < TextureTilesPerAxis; ++i)
		{
			for (s32 j = 0; j < TextureTilesPerAxis; ++j)
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

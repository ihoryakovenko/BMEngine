#include "DynamicMapSystem.h"

#include <vector>
#include <string>
#include <cassert>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

namespace DynamicMapSystem
{
	struct Tile
	{
		s32 x;
		s32 y; 
		s32 Zoom;
	};

	static void UpdateTilesData(s32 TextureTilesPerAxis);

	static s32 LonToTileX(f64 lon, s32 z);
	static s32 LatToTileY(f64 lat, s32 z);

	static u32 GetTilesPerAxis(u32 Zoom);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const u32 MaxZoom = 20;
	static const u32 MinZoom = 0;

	static const s32 MaxTileZoom = 5;
	static const f64 SphereRadius = 1.0;
	static const f64 TileAngularSize = 360.0 / (1 << 5);

	static s32 VertexZoom = 1;
	static s32 TileZoom = 4;

	static bool UpdateVertexDataFlag = false;
	static bool UpdateTileDataFlag = false;

	static s32 CameraLat = 0;
	static s32 CameraLon = 0;

	void Init()
	{
		const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);
		const s32 TextureTilesPerAxis = GetTilesPerAxis(TileZoom);
		const s32 MaxTextureTilesPerAxis = GetTilesPerAxis(MaxTileZoom);

		BMR::BMRTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, 256, 256,
			MaxTextureTilesPerAxis * MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);


		VkDescriptorSet TilesMaterial;
		ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
		Scene.MapEntity.TextureSet = TilesMaterial;

		UpdateTilesData(TextureTilesPerAxis);
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

		const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);

		auto CameraRelativeSpherePosition = glm::normalize(Camera.Position);
		const f64 CameraLat = glm::degrees(asin(CameraRelativeSpherePosition.y / SphereRadius));
		const f64 CameraLon = glm::degrees(atan2(CameraRelativeSpherePosition.x, CameraRelativeSpherePosition.z));

		const s32 CameraTileX = LonToTileX(CameraLon, Zoom);
		const s32 CameraTileY = LatToTileY(CameraLat, Zoom);

		const f64 HorizontalFov = Camera.Fov * Camera.AspectRatio;

		const f64 VerticalFovInRadians = glm::radians(Camera.Fov * 0.5);
		const f64 VerticalDistance = SphereRadius / (SphereRadius + glm::length(Camera.Position));
		const f64 VerticalAngle = glm::degrees(2.0 * atan(tan(VerticalFovInRadians) * VerticalDistance));

		const f64 HorizontalFovInRadians = glm::radians(HorizontalFov * 0.5);
		const f64 HorizontalDistance = SphereRadius / (SphereRadius + glm::length(Camera.Position));
		const f64 HorizontalAngle = glm::degrees(2.0 * atan(tan(HorizontalFovInRadians) * HorizontalDistance));

		const f64 RelativeTileAngularSize = TileAngularSize * cos(glm::radians(CameraLat));

		s32 TilesCountX = ceil(HorizontalAngle / RelativeTileAngularSize);
		s32 TilesCountY = ceil(VerticalAngle / RelativeTileAngularSize);

		if (TilesCountY % 2 != 0)
		{
			TilesCountY += (TilesCountY >= 0) ? 1 : -1;
		}

		s32 MinTileX = CameraTileX - TilesCountX / 2;
		s32 MaxTileX = CameraTileX + TilesCountX / 2;

		s32 MinTileY = CameraTileY - TilesCountY / 2;
		s32 MaxTileY = CameraTileY + TilesCountY / 2;

		MaxTileX += (MaxTileX >= 0) ? 1 : -1;
		MaxTileY += (MaxTileY > 0) ? 1 : -1;

		std::vector<u32> Indices;
		//Indices.reserve(((minTileX + maxTileX) / 2) * ((minTileY + maxTileY) / 2) * 6);

		std::vector<Tile> visibleTiles;

		for (s32 x = MinTileX; x < MaxTileX; ++x)
		{
			const s32 ValidX = (x + (1 << Zoom)) % (1 << Zoom);

			for (s32 y = MinTileY; y < MaxTileY; ++y)
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
			const u32 TextureTilesPerAxis = GetTilesPerAxis(TileZoom);
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

	glm::vec3 SphericalToMercator(const glm::vec3& Position)
	{
		//const f64 Latitude = glm::half_pi<f64>() * Position.x; // Map [-1, 1] -> [-p/2, p/2]

		// Map[-1, 1] -> Mercator latitude in radians [-p/2, p/2]
		const f64 Latitude = glm::atan(glm::sinh(glm::pi<f64>() * Position.x));
		const f64 Longitude = glm::pi<f64>() * Position.y; // Map [-1, 1] -> [-p, p]
		const f64 Altitude = Position.z;

		const f64 CosLatitude = glm::cos(Latitude);
		const f64 SinLatitude = glm::sin(Latitude);

		glm::vec3 CartesianPos;
		CartesianPos.x = Altitude * CosLatitude * glm::sin(Longitude);
		CartesianPos.y = Altitude * SinLatitude;
		CartesianPos.z = Altitude * CosLatitude * glm::cos(Longitude);

		return CartesianPos;
	}

	f64 CalculateCameraAltitude(s32 ZoomLevel)
	{
		return Math::Circumference / glm::pow(2, ZoomLevel - 1);
	}

	u32 GetTilesPerAxis(u32 Zoom)
	{
		assert(Zoom <= MaxZoom);
		return 2 << glm::clamp(Zoom - 1u, MinZoom, MaxZoom);
	}

	s32 LonToTileX(f64 Lon, s32 Zoom)
	{
		return (s32)(floor((Lon + 180.0) / 360.0 * (1 << Zoom)));
	}

	s32 LatToTileY(f64 Lat, s32 Zoom)
	{
		f64 LatRad = Lat * glm::pi<f64>() / 180.0;
		return (s32)(floor((1.0 - asinh(tan(LatRad)) / glm::pi<f64>()) / 2.0 * (1 << Zoom)));
	}
}

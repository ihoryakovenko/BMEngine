#include "DynamicMapSystem.h"

#include <vector>
#include <string>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

namespace DynamicMapSystem
{
	static void UpdateVertexData(u32 VertexTilesPerAxis);
	static void UpdateTilesData(u32 TextureTilesPerAxis);

	static void ComputeVisibleTileRange(const glm::vec3& CameraSphericalPosition, f32 FovHorizontal, f32 AspectRatio, u32 VertexTilesPerAxis);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const u32 MaxTileZoom = 5;

	static u32 VertexZoom = 4;
	static u32 TileZoom = 4;
	static s32 LatMin = 0;
	static s32 LatMax = 0;
	static s32 LonMin = 0;
	static s32 LonMax = 0;

	static bool UpdateVertexDataFlag = false;
	static bool UpdateTileDataFlag = false;

	void Init()
	{
		const u32 VertexTilesPerAxis = Math::GetTilesPerAxis(VertexZoom);
		const u32 TextureTilesPerAxis = Math::GetTilesPerAxis(TileZoom);
		const u32 MaxTextureTilesPerAxis = Math::GetTilesPerAxis(MaxTileZoom);

		LatMax = VertexTilesPerAxis;
		LonMax = VertexTilesPerAxis;

		UpdateVertexData(VertexTilesPerAxis);

		BMR::BMRTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, 256, 256,
			MaxTextureTilesPerAxis * MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);


		VkDescriptorSet TilesMaterial;
		ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
		Scene.MapEntity.TextureSet = TilesMaterial;

		UpdateTilesData(TextureTilesPerAxis);
	}

	void Update(const glm::vec3& CameraPosition, f32 FovHorizontal, f32 AspectRatio)
	{
		const u32 VertexTilesPerAxis = Math::GetTilesPerAxis(VertexZoom);
		ComputeVisibleTileRange(CameraPosition, FovHorizontal, AspectRatio, VertexTilesPerAxis);
		//if (UpdateVertexDataFlag)
		{
			UpdateVertexData(VertexTilesPerAxis);
		}

		if (UpdateTileDataFlag)
		{
			const u32 TextureTilesPerAxis = Math::GetTilesPerAxis(TileZoom);
			UpdateTilesData(TextureTilesPerAxis);
		}
	}

	void DeInit()
	{
	}

	void UpdateVertexData(u32 VertexTilesPerAxis)
	{
		std::vector<u32> Indices;
		Indices.reserve(VertexTilesPerAxis * VertexTilesPerAxis * 6);

		for (s32 lat = LatMin; lat < LatMax; ++lat)
		{
			for (s32 j = LonMin; j < LonMax; ++j)
			{
				const s32 Lon = j % VertexTilesPerAxis;

				s32 first = lat * (VertexTilesPerAxis + 1) + Lon;
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

		UpdateVertexDataFlag = false;
	}

	void UpdateTilesData(u32 TextureTilesPerAxis)
	{
		std::vector<std::string> OsmTiles;
		const std::string BasePath = "osm_tiles_zoom" + std::to_string(TileZoom) +
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

	void ComputeVisibleTileRange(const glm::vec3& CameraSphericalPosition, f32 FovHorizontal, f32 AspectRatio, u32 VertexTilesPerAxis)
	{
		const f32 LatitudeNorm = (-CameraSphericalPosition.x + 1.0f) / 2.0f;
		const f32 LongitudeNorm = (CameraSphericalPosition.y + 1.0f) / 2.0f;

		const u32 CameraLat = LatitudeNorm * VertexTilesPerAxis;
		const u32 CameraLon = LongitudeNorm * VertexTilesPerAxis;

		f32 HalfFovHorizontal = FovHorizontal / 2.0f;

		// Calculate how many tiles fit in the horizontal FOV (Longitude)
		f32 VisibleLon = glm::tan(HalfFovHorizontal) * CameraSphericalPosition.z * 2.0f;
		u32 LonRange = VisibleLon * VertexTilesPerAxis / glm::two_pi<f32>();

		// Calculate how many tiles fit in the vertical FOV (Latitude)
		f32 VisibleLat = glm::tan(HalfFovHorizontal / AspectRatio) * CameraSphericalPosition.z * 2.0f;
		u32 LatRange = static_cast<u32>(VisibleLat * VertexTilesPerAxis / glm::pi<f32>());

		// Adjust the LatMin, LatMax, LonMin, LonMax based on the visible range
		LatMin = glm::max<u32>(0, static_cast<u32>(CameraLat) - LatRange);
		LatMax = glm::min<u32>(static_cast<u32>(VertexTilesPerAxis), static_cast<u32>(CameraLat) + LatRange);

		//LonMin = glm::max<u32>(0, static_cast<u32>(CameraLon) - LonRange);
		//LonMax = glm::min<u32>(static_cast<u32>(VertexTilesPerAxis), static_cast<u32>(CameraLon) + LonRange);

		LonMin = CameraLon - LonRange;
		LonMax = CameraLon + LonRange;

		int i = 0;
	}

	void SetVertexZoom(s32 Zoom, s32 InLatMin, s32 InLatMax, s32 InLonMin, s32 InLonMax)
	{
		VertexZoom = Zoom;
		LatMin = InLatMin;
		LatMax = InLatMax;
		LonMin = InLonMin;
		LonMax = InLonMax;

		UpdateVertexDataFlag = true;
	}

	void SetTileZoom(s32 Zoom)
	{
		TileZoom = Zoom;
		UpdateTileDataFlag = true;
	}
}

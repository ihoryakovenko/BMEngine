#include "DynamicMapSystem.h"

#include <vector>
#include <string>
#include <cassert>

#include <httplib.h>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

#include "stb_image.h"

namespace DynamicMapSystem
{
	static s32 LonToTileX(f64 Lon, s32 Zoom);
	static s32 LatToTileY(f64 Lat, s32 Zoom);

	static u32 GetTilesPerAxis(u32 Zoom);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const u32 MaxZoom = 20;
	static const u32 MinZoom = 0;

	static const s32 MaxTileZoom = 5;
	static const f64 SphereRadius = 1.0;
	static const f64 TileAngularSize = 360.0 / (1 << 6);

	static u32 TextureTileSize = 256;

	static s32 VertexZoom = 1;
	static s32 TileZoom = 4;

	static s32 MinTileX = 0;
	static s32 MaxTileX = 0;

	static s32 MinTileY = 0;
	static s32 MaxTileY = 0;

	static bool TestDownload = false;

	void Init()
	{
		const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);
		const s32 TextureTilesPerAxis = GetTilesPerAxis(TileZoom);
		const s32 MaxTextureTilesPerAxis = GetTilesPerAxis(MaxTileZoom);

		BMR::BMRTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, TextureTileSize, TextureTileSize,
			MaxTextureTilesPerAxis * MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

		VkDescriptorSet TilesMaterial;
		ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
		Scene.MapEntity.TextureSet = TilesMaterial;
	}

	void Update(const MapCamera& Camera, s32 Zoom)
	{
		VertexZoom = Zoom;

		const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);

		const glm::vec3 CameraRelativeSpherePosition = glm::normalize(Camera.Position);
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

		MinTileX = CameraTileX - TilesCountX / 2;
		MaxTileX = CameraTileX + TilesCountX / 2;

		MinTileY = CameraTileY - TilesCountY / 2;
		MaxTileY = CameraTileY + TilesCountY / 2;

		MaxTileX += (MaxTileX >= 0) ? 1 : -1;
		MaxTileY += (MaxTileY > 0) ? 1 : -1;

		std::vector<u32> Indices;

		for (s32 x = MinTileX; x < MaxTileX; ++x)
		{
			const s32 ValidX = (x + (1 << Zoom)) % (1 << Zoom);

			for (s32 y = MinTileY; y < MaxTileY; ++y)
			{
				const s32 ValidY = glm::clamp(y, 0, (1 << Zoom) - 1);

				const s32 First = ValidY * (VertexTilesPerAxis + 1) + ValidX;
				const s32 Second = First + (VertexTilesPerAxis + 1);

				Indices.push_back(First);
				Indices.push_back(Second);
				Indices.push_back(First + 1);

				Indices.push_back(Second);
				Indices.push_back(Second + 1);
				Indices.push_back(First + 1);

				if (TestDownload)
				{
					static httplib::Client Client("http://tile.openstreetmap.org");
					const std::string Path = "/" + std::to_string(VertexZoom) + "/" +
						std::to_string(ValidX) + "/" + std::to_string(ValidY) + ".png";

					const httplib::Result Res = Client.Get(Path);
					if (Res && Res->status == 200)
					{
						BMR::BMRTexture* Texture = ResourceManager::FindTexture(TilesTextureId);

						unsigned char* data = reinterpret_cast<unsigned char*>(const_cast<char*>(Res->body.data()));
						int width, height, channels;
						unsigned char* image = stbi_load_from_memory(data, Res->body.size(), &width, &height, &channels, 4);

						BMR::BMRTextureArrayInfo Info;
						Info.Width = TextureTileSize;
						Info.Height = TextureTileSize;
						Info.Format = 4;
						Info.LayersCount = 1;
						Info.BaseArrayLayer = ValidY * VertexTilesPerAxis + ValidX;
						Info.Data = &image;

						BMR::UpdateTexture(Texture, &Info);

						Scene.MapTileSettings.TextureTilesPerAxis = VertexTilesPerAxis;
					}
				}
			}
		}

		TestDownload = false;

		if (Indices.empty())
		{
			return;
		}

		BMR::ClearIndices();
		Scene.MapEntity.IndexOffset = BMR::LoadIndices(Indices.data(), Indices.size());
		Scene.MapEntity.IndicesCount = Indices.size();
		Scene.MapTileSettings.VertexTilesPerAxis = VertexTilesPerAxis;
	}

	void DeInit()
	{
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

	void TestSetDownload(bool Download)
	{
		TestDownload = Download;
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

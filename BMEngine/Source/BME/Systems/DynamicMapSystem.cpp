#include "DynamicMapSystem.h"

#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <cassert>

#include <httplib.h>

#include "ResourceManager.h"
#include "BME/Scene.h"
#include "Util/Math.h"

#include "stb_image.h"

namespace DynamicMapSystem
{
	struct Tile
	{
		s32 Zoom;
		s32 X;
		s32 Y;
		s32 ArrayLayer;
		std::string Data;
	};

	static s32 LonToTileX(f64 Lon, s32 Zoom);
	static s32 LatToTileY(f64 Lat, s32 Zoom);

	static s32 GetTilesPerAxis(s32 Zoom);

	static void DownloadTiles(const std::vector<Tile>& Tiles);

	static const char TilesTextureId[] = "TilesTexture";
	static const char TilesMaterialId[] = "TilesMaterial";

	static const s32 MaxZoom = 20;
	static const s32 MinZoom = 1;

	static const s32 MinVertexZoom = 6;

	static const f64 SphereRadius = 1.0;

	static const s32 MaxTextureTilesPerAxis = 2048;

	static s32 TextureTileSize = 256;

	static s32 VertexZoom = 0;
	static s32 TileZoom = 0;

	//static s32 MinTileX = 0;
	//static s32 MaxTileX = 0;

	//static s32 MinTileY = 0;
	//static s32 MaxTileY = 0;

	static std::mutex QueueMutex;
	static std::queue<Tile> DownloadQueue;
	static std::atomic<bool> StopDownload = false;
	static std::mutex DeInitMutex;

	static httplib::Client* Client = nullptr;

	static bool TestDownload = false;

	void Init()
	{
		BMR::BMRTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, TextureTileSize, TextureTileSize,
			MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

		VkDescriptorSet TilesMaterial;
		ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
		Scene.MapEntity.TextureSet = TilesMaterial;

		static httplib::Client ClientInstance("http://tile.openstreetmap.org");
		Client = &ClientInstance;
	}

	void Update(const MapCamera& Camera, s32 Zoom)
	{
		VertexZoom = Zoom > MinVertexZoom ? Zoom : MinVertexZoom;

		const glm::vec3 CameraRelativeSpherePosition = glm::normalize(Camera.Position);
		const f64 CameraLat = glm::degrees(asin(CameraRelativeSpherePosition.y / SphereRadius));
		const f64 CameraLon = glm::degrees(atan2(CameraRelativeSpherePosition.x, CameraRelativeSpherePosition.z));

		const f64 HorizontalFov = Camera.Fov * Camera.AspectRatio;
		const f64 DistanceToSurface = glm::length(Camera.Position) - SphereRadius;

		const f64 VerticalFovInRadians = glm::radians(Camera.Fov * 0.5);
		const f64 VerticalAngle = glm::degrees(2.0 * atan(tan(VerticalFovInRadians) * DistanceToSurface));

		const f64 HorizontalFovInRadians = glm::radians(HorizontalFov * 0.5);
		const f64 HorizontalAngle = glm::degrees(2.0 * atan(tan(HorizontalFovInRadians) * DistanceToSurface));

		{
			const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);

			s32 MinTileX;
			s32 MaxTileX;

			s32 MinTileY;
			s32 MaxTileY;

			const f64 RelativeTileAngularSize = 360.0 / VertexTilesPerAxis * cos(glm::radians(CameraLat));

			const s32 CameraTileX = LonToTileX(CameraLon, VertexTilesPerAxis);
			const s32 CameraTileY = LatToTileY(CameraLat, VertexTilesPerAxis);

			s32 VertexCountX = ceil(HorizontalAngle / RelativeTileAngularSize);
			s32 VertexCountY = ceil(VerticalAngle / RelativeTileAngularSize);

			//TilesCountY += (TilesCountY >= 0) ? 1 : -1;

			if (VertexCountY % 2 != 0)
			{
				VertexCountY += (VertexCountY >= 0) ? 1 : -1;
			}

			MinTileX = CameraTileX - VertexCountX / 2;
			MaxTileX = CameraTileX + VertexCountX / 2;

			MinTileY = CameraTileY - VertexCountY / 2;
			MaxTileY = CameraTileY + VertexCountY / 2;

			//MaxVertexTileX += (MaxVertexTileX >= 0) ? 1 : -1;
			//MaxVertexTileY += (MaxVertexTileY > 0) ? 1 : -1;

			//MinVertexTileX -= (MinVertexTileX >= 0) ? 1 : -1;
			//MinVertexTileY -= (MinVertexTileY > 0) ? 1 : -1;

			std::vector<u32> Indices;
			for (s32 x = MinTileX; x < MaxTileX; ++x)
			{
				const s32 ValidX = (x + VertexTilesPerAxis) % VertexTilesPerAxis;

				for (s32 y = MinTileY; y < MaxTileY; ++y)
				{
					const s32 ValidY = glm::clamp(y, 0, VertexTilesPerAxis - 1);

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
		}

		{
			TileZoom = Zoom;
			
			s32 MinTileX;
			s32 MaxTileX;

			s32 MinTileY;
			s32 MaxTileY;

			const s32 TilesPerAxis = GetTilesPerAxis(TileZoom);
			const f64 RelativeTileAngularSize = 360.0 / TilesPerAxis * cos(glm::radians(CameraLat));

			const s32 CameraTileX = LonToTileX(CameraLon, TilesPerAxis);
			const s32 CameraTileY = LatToTileY(CameraLat, TilesPerAxis);

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

			s32 ClampedMinTileX;
			s32 ClampedMinTileY;

			if (TestDownload)
			{
				ClampedMinTileX = (MinTileX + TilesPerAxis) % TilesPerAxis;
				ClampedMinTileY = glm::clamp(MinTileY, 0, TilesPerAxis - 1);

				Scene.MapTileSettings.TextureTilesPerAxis = TilesPerAxis;
				Scene.MapTileSettings.MinTileX = ClampedMinTileX;
				Scene.MapTileSettings.MinTileY = ClampedMinTileY;
				Scene.MapTileSettings.TilesCountY = TilesCountY;
			}

			std::vector<Tile> TilesToDownload;

			for (s32 x = MinTileX; x < MaxTileX; ++x)
			{
				const s32 ValidX = (x + TilesPerAxis) % TilesPerAxis;

				for (s32 y = MinTileY; y < MaxTileY; ++y)
				{
					const s32 ValidY = glm::clamp(y, 0, TilesPerAxis - 1);

					if (TestDownload)
					{
						Tile T;
						T.Zoom = VertexZoom;
						T.X = ValidX;
						T.Y = ValidY;

						const s32 OffsetX = (ValidX - ClampedMinTileX + TilesPerAxis) % TilesPerAxis;
						const s32 OffsetY = (ValidY - ClampedMinTileY + TilesPerAxis) % TilesPerAxis;
						T.ArrayLayer = OffsetX * (TilesCountY + 1) + OffsetY;

						TilesToDownload.emplace_back(T);
					}
				}
			}

			if (!TilesToDownload.empty())
			{
				std::thread Thread(&DownloadTiles, TilesToDownload);
				Thread.detach();
			}

			TestDownload = false;
		}
		
		BMR::BMRTexture* Texture = ResourceManager::FindTexture(TilesTextureId);

		{
			std::unique_lock Lock(QueueMutex);
			while (!DownloadQueue.empty())
			{
				const Tile& TileDataCompressed = DownloadQueue.front();

				unsigned char* data = reinterpret_cast<unsigned char*>(const_cast<char*>(TileDataCompressed.Data.data()));
				int width, height, channels;
				unsigned char* image = stbi_load_from_memory(data, TileDataCompressed.Data.size(), &width, &height, &channels, 4);

				BMR::BMRTextureArrayInfo Info;
				Info.Width = TextureTileSize;
				Info.Height = TextureTileSize;
				Info.Format = 4;
				Info.LayersCount = 1;
				Info.BaseArrayLayer = TileDataCompressed.ArrayLayer;
				Info.Data = &image;

				BMR::UpdateTexture(Texture, &Info);

				DownloadQueue.pop();
			}
		}
	}

	void DeInit()
	{
		StopDownload = true;
		std::unique_lock DeInitLock(DeInitMutex);
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

	s32 GetTilesPerAxis(s32 Zoom)
	{
		assert(Zoom <= MaxZoom);
		return 1 << glm::clamp(Zoom, MinZoom, MaxZoom);
	}

	void DownloadTiles(const std::vector<Tile>& Tiles)
	{
		std::unique_lock DeInitLock(DeInitMutex);

		for (s32 i = 0; i < Tiles.size(); ++i)
		{
			if (StopDownload == true)
			{
				return;
			}

			Tile TileToDownload = Tiles[i];

			const std::string Path = "/" + std::to_string(VertexZoom) + "/" +
				std::to_string(TileToDownload.X) + "/" + std::to_string(TileToDownload.Y) + ".png";

			httplib::Result Res = Client->Get(Path);

			if (Res && Res->status == httplib::StatusCode::OK_200)
			{
				std::unique_lock Lock(QueueMutex);

				TileToDownload.Data = std::move(Res->body);
				DownloadQueue.emplace(std::move(TileToDownload));
			}
		}
	}

	s32 LonToTileX(f64 Lon, s32 TilePerAxis)
	{
		return (s32)(floor((Lon + 180.0) / 360.0 * TilePerAxis));
	}

	s32 LatToTileY(f64 Lat, s32 TilePerAxis)
	{
		f64 LatRad = Lat * glm::pi<f64>() / 180.0;
		return (s32)(floor((1.0 - asinh(tan(LatRad)) / glm::pi<f64>()) / 2.0 * TilePerAxis));
	}
}

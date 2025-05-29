//struct RecyclingCommandPool
//{
//	VkCommandPool CommandPool;
//	VkCommandBuffer* Buffers;
//	VkFence* BufferFences;
//	u32 BufferCount;
//	u32 NextBufferIndex;
//};
//
//static void WaitForNextBuffer(VkDevice Device, RecyclingCommandPool* Pool, VkCommandBuffer* OutBuffer, VkFence* OutFence);
//
//void WaitForNextBuffer(VkDevice Device, RecyclingCommandPool* Pool, VkCommandBuffer* OutBuffer, VkFence* OutFence)
//{
//	const VkFence* Fence = Pool->BufferFences + Pool->NextBufferIndex;
//
//	vkWaitForFences(Device, 1, Fence, VK_TRUE, UINT64_MAX);
//	vkResetFences(Device, 1, Fence);
//
//	Pool->NextBufferIndex = (Pool->NextBufferIndex + 1) % Pool->BufferCount;
//	*OutBuffer = Pool->Buffers[Pool->NextBufferIndex];
//	*OutFence = *Fence;
//}
//
//void InitTransferCommandPool(VkDevice Device, u32 QueueFamilyIndex, u32 BufferCount)
//{
//	TransferCommandPool.BufferCount = BufferCount;
//	TransferCommandPool.NextBufferIndex = 0;
//	TransferCommandPool.Buffers = Memory::MemoryManagementSystem::Allocate<VkCommandBuffer>(TransferCommandPool.BufferCount);
//	TransferCommandPool.BufferFences = Memory::MemoryManagementSystem::Allocate<VkFence>(TransferCommandPool.BufferCount);
//
//	VkCommandPoolCreateInfo PoolInfo = { };
//	PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//	PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//	PoolInfo.queueFamilyIndex = QueueFamilyIndex;
//
//	VkFenceCreateInfo FenceCreateInfo = { };
//	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//
//	VkResult Result = vkCreateCommandPool(Device, &PoolInfo, nullptr, &TransferCommandPool.CommandPool);
//	if (Result != VK_SUCCESS)
//	{
//		Util::RenderLog(Util::BMRVkLogType_Error, "vkCreateCommandPool result is %d", Result);
//	}
//
//	VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
//	TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//	TransferCommandBufferAllocateInfo.commandPool = TransferCommandPool.CommandPool;
//	TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//	TransferCommandBufferAllocateInfo.commandBufferCount = TransferCommandPool.BufferCount;
//
//	for (u32 i = 0; i < TransferCommandPool.BufferCount; ++i)
//	{
//		if (vkCreateFence(Device, &FenceCreateInfo, nullptr, TransferCommandPool.BufferFences + i) != VK_SUCCESS)
//		{
//			Util::RenderLog(Util::BMRVkLogType_Error, "TransferFences creation error");
//			assert(false);
//		}
//	}
//
//	if (vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, TransferCommandPool.Buffers) != VK_SUCCESS)
//	{
//		Util::RenderLog(Util::BMRVkLogType_Error, "vkAllocateCommandBuffers error");
//		assert(false);
//	}
//}

//struct ResourceHandle
	//{
	//	u32 Index;
	//	u32 Generation;
	//};

	//ResourceHandle CreateTexture(VkImage image, VkImageView view)
	//{
	//	u32 LogicalIndex;

	//	if (!freeList.empty())
	//	{
	//		LogicalIndex = freeList.back();
	//		freeList.pop_back();
	//	}
	//	else
	//	{
	//		LogicalIndex = static_cast<u32>(handleEntries.size());
	//		handleEntries.emplace_back();
	//	}

	//	const u32 PhysicalIndex = TextureIndex++;

	//	Textures[PhysicalIndex] = image;
	//	TextureViews[PhysicalIndex] = view;

	//	handleEntries[LogicalIndex].Generation++;
	//	handleEntries[LogicalIndex].Alive = true;
	//	handleEntries[LogicalIndex].PhysicalIndex = PhysicalIndex;

	//	reverseMapping[PhysicalIndex] = LogicalIndex;

	//	return ResourceHandle{ LogicalIndex, handleEntries[LogicalIndex].Generation };
	//}

	//void DestroyTexture(ResourceHandle h)
	//{
	//	if (!IsValid(h)) return;

	//	u32 removedPhys = handleEntries[h.Index].PhysicalIndex;
	//	u32 lastPhys = TextureIndex - 1;

	//	if (removedPhys != lastPhys)
	//	{
	//		Textures[removedPhys] = Textures[lastPhys];
	//		TextureViews[removedPhys] = TextureViews[lastPhys];

	//		u32 movedLogical = reverseMapping[lastPhys];
	//		handleEntries[movedLogical].PhysicalIndex = removedPhys;
	//		reverseMapping[removedPhys] = movedLogical;
	//	}

	//	handleEntries[h.Index].Alive = false;
	//	freeList.push_back(h.Index);
	//	--TextureIndex;
	//}

	//bool IsValid(ResourceHandle h)
	//{
	//	return h.Index < handleEntries.size() &&
	//		handleEntries[h.Index].Alive &&
	//		handleEntries[h.Index].Generation == h.Generation;
	//}

	//VkImage GetImage(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return Textures[PhysicalIndex];
	//}

	//VkImageView GetView(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return TextureViews[PhysicalIndex];
	//}

















//namespace DynamicMapSystem
//{
//	void Init();
//	void DeInit();
//
//	void Update(f64 DeltaTime, const Camera& Camera, s32 Zoom);
//
//	glm::vec3 SphericalToMercator(const glm::vec3& Position);
//	f64 CalculateCameraAltitude(s32 ZoomLevel);
//
//	void TestSetDownload(bool Download);
//}

//#include "DynamicMapSystem.h"
//
//#include <vector>
//#include <string>
//#include <thread>
//#include <queue>
//#include <cassert>
//#include <cfloat>
//
//#include <glm/gtc/constants.hpp>
//
//#include <httplib.h>
//
//#include "ResourceManager.h"
//#include "Engine/Scene.h"
//#include "Util/Math.h"
//#include "Util/Util.h"
//#include "Util/Settings.h"
//#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
//#include "Render/FrameManager.h"
//#include "Engine/Systems/Render/Render.h"
//
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//
//namespace DynamicMapSystem
//{
//	struct Tile
//	{
//		s32 Zoom;
//		s32 X;
//		s32 Y;
//		s32 ArrayLayer;
//		std::string Data;
//	};
//
//	// TODO: alignas(64) is TMP solution, solve using Properties->limits.minUniformBufferOffsetAlignment
//	struct alignas(64) TileSettingsBuffer
//	{
//		u32 VertexTilesPerAxis;
//		u32 TextureTilesPerAxis;
//		u32 MinTileX;
//		u32 MinTileY;
//		u32 TilesCountY;
//	};
//
//	static void OnDraw();
//
//	static s32 LonToTileX(f64 Lon, s32 Zoom);
//	static s32 LatToTileY(f64 Lat, s32 Zoom);
//
//	static s32 GetTilesPerAxis(s32 Zoom);
//
//	static void DownloadTiles(const std::vector<Tile>& Tiles);
//	static void GetCameraVisibleRange(f64 CameraLat, f64 CameraLon, f64 HorizontalAngle, f64 VerticalAngle, s32 VertexTilesPerAxis,
//		s32& MinTileX, s32& MaxTileX, s32& MinTileY, s32& MaxTileY, s32& TilesCountY);
//
//	static const char TilesTextureId[] = "TilesTexture";
//	static const char TilesMaterialId[] = "TilesMaterial";
//
//	static const s32 MaxZoom = 20;
//	static const s32 MinZoom = 1;
//
//	static const s32 MinVertexZoom = 6;
//
//	static const f64 SphereRadius = 1.0;
//
//	static const s32 MaxTextureTilesPerAxis = 2048;
//
//	static s32 TextureTileSize = 256;
//
//	static s32 VertexZoom = 0;
//	static s32 TileZoom = 0;
//
//	static f64 CashedCameraLat = DBL_MIN;
//	static f64 CashedCameraLon = DBL_MIN;
//	static const f64 TileUpdateTimeSeconds = 1.5;
//	static f64 TileUpdateCounter = 0.0;
//
//	static std::mutex QueueMutex;
//	static std::queue<Tile> DownloadQueue;
//	static std::atomic<bool> StopDownload = false;
//	static std::mutex DeInitMutex;
//
//	static httplib::Client* Client;
//
//	static TileSettingsBuffer MapTileSettings;
//
//	static VulkanInterface::IndexBuffer IndexBuffer;
//	static VkDescriptorSetLayout MapTileSettingsLayout;
//	static FrameManager::UniformMemoryHnadle SettingsBufferHandle;
//	static VkDescriptorSet MapTileSettingsSet;
//	static VulkanInterface::RenderPipeline Pipeline;
//
//	static bool TestDownload = true;
//	static int TestIndicesCount = 0;
//
//	void Init()
//	{
//		const u32 ShaderCount = 2;
//		VulkanInterface::Shader Shaders[ShaderCount];
//
//		std::vector<char> VertexShaderCode;
//		Util::OpenAndReadFileFull("./Resources/Shaders/QuadBasedSphere_vert.spv", VertexShaderCode, "rb");
//		std::vector<char> FragmentShaderCode;
//		Util::OpenAndReadFileFull("./Resources/Shaders/QuadBasedSphere_frag.spv", FragmentShaderCode, "rb");
//
//		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
//		Shaders[0].Code = VertexShaderCode.data();
//		Shaders[0].CodeSize = VertexShaderCode.size();
//
//		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//		Shaders[1].Code = FragmentShaderCode.data();
//		Shaders[1].CodeSize = FragmentShaderCode.size();
//
//		const VkDeviceSize MapTileSettingsSize = sizeof(TileSettingsBuffer);
//
//		MapTileSettingsLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//		SettingsBufferHandle = FrameManager::ReserveUniformMemory(MapTileSettingsSize);
//		MapTileSettingsSet = FrameManager::CreateAndBindSet(SettingsBufferHandle, MapTileSettingsSize, MapTileSettingsLayout);
//
//		VkDescriptorSetLayout MapDescriptorLayouts[] =
//		{
//			FrameManager::GetViewProjectionLayout(),
//			//Render::TestGetTerrainSkyBoxLayout(),
//			MapTileSettingsLayout,
//		};
//		const u32 MapDescriptorLayoutCount = sizeof(MapDescriptorLayouts) / sizeof(MapDescriptorLayouts[0]);
//
//		//Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(MapDescriptorLayouts, MapDescriptorLayoutCount, nullptr, 0);
//
//		VulkanInterface::PipelineResourceInfo ResourceInfo;
//		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
//		//ResourceInfo.RenderPass = Render::TestGetRenderPass();
//		//ResourceInfo.SubpassIndex = 0;
//
//		VulkanInterface::PipelineSettings PipelineSettings;
//		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/DynamicMapSystem.ini");
//		PipelineSettings.Extent = MainScreenExtent;
//
//		VulkanInterface::BMRVertexInputBinding VertexInput;
//
//		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, &VertexInput, 0,
//			&PipelineSettings, &ResourceInfo);
//
//		//IndexBuffer = VulkanInterface::CreateIndexBuffer(MB64);
//
//		//Render::RenderTexture TextureArrayTiles = ResourceManager::EmptyTexture(TilesTextureId, TextureTileSize, TextureTileSize,
//			//MaxTextureTilesPerAxis, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
//
//		VkDescriptorSet TilesMaterial;
//		//ResourceManager::CreateSkyBoxTerrainTexture(TilesMaterialId, TextureArrayTiles.ImageView, &TilesMaterial);
//		//Scene.MapEntity.TextureSet = TilesMaterial;
//
//		static httplib::Client ClientInstance("http://tile.openstreetmap.org");
//		Client = &ClientInstance;
//	}
//
//	void Update(f64 DeltaTime, const Camera& Camera, s32 Zoom)
//	{
//		VertexZoom = Zoom > MinVertexZoom ? Zoom : MinVertexZoom;
//
//		const glm::vec3 CameraRelativeSpherePosition = glm::normalize(Camera.Position);
//		const f64 CameraLat = glm::degrees(asin(CameraRelativeSpherePosition.y / SphereRadius));
//		const f64 CameraLon = glm::degrees(atan2(CameraRelativeSpherePosition.x, CameraRelativeSpherePosition.z));
//
//		const f64 HorizontalFov = Camera.Fov * Camera.AspectRatio;
//		const f64 DistanceToSurface = glm::length(Camera.Position) - SphereRadius;
//
//		const f64 VerticalFovInRadians = glm::radians(Camera.Fov * 0.5);
//		const f64 VerticalAngle = glm::degrees(2.0 * atan(tan(VerticalFovInRadians) * DistanceToSurface));
//
//		const f64 HorizontalFovInRadians = glm::radians(HorizontalFov * 0.5);
//		const f64 HorizontalAngle = glm::degrees(2.0 * atan(tan(HorizontalFovInRadians) * DistanceToSurface));
//
//		{
//			const s32 VertexTilesPerAxis = GetTilesPerAxis(VertexZoom);
//
//			s32 MinTileX;
//			s32 MaxTileX;
//
//			s32 MinTileY;
//			s32 MaxTileY;
//
//			s32 TilesCountY;
//
//			GetCameraVisibleRange(CameraLat, CameraLon, HorizontalAngle, VerticalAngle, VertexTilesPerAxis,
//				MinTileX, MaxTileX, MinTileY, MaxTileY, TilesCountY);
//
//			std::vector<u32> Indices;
//			for (s32 x = MinTileX; x < MaxTileX; ++x)
//			{
//				const s32 ValidX = (x + VertexTilesPerAxis) % VertexTilesPerAxis;
//
//				for (s32 y = MinTileY; y < MaxTileY; ++y)
//				{
//					const s32 ValidY = glm::clamp(y, 0, VertexTilesPerAxis - 1);
//
//					const s32 First = ValidY * (VertexTilesPerAxis + 1) + ValidX;
//					const s32 Second = First + (VertexTilesPerAxis + 1);
//
//					Indices.push_back(First);
//					Indices.push_back(Second);
//					Indices.push_back(First + 1);
//
//					Indices.push_back(Second);
//					Indices.push_back(Second + 1);
//					Indices.push_back(First + 1);
//				}
//			}
//
//			if (Indices.empty())
//			{
//				return;
//			}
//
//			TestIndicesCount = Indices.size();
//			//Render::LoadIndices(&IndexBuffer, Indices.data(), Indices.size(), 0);
//			MapTileSettings.VertexTilesPerAxis = VertexTilesPerAxis;
//		}
//
//		bool UpdateTiles = false;
//
//		//if (TileZoom != Zoom)
//		//{
//		//	UpdateTiles = true;
//		//}
//
//		//if (CashedCameraLat != CameraLat || CashedCameraLon != CameraLon || TileZoom != Zoom)
//		if (false)
//		{
//			if (TileUpdateCounter > TileUpdateTimeSeconds)
//			{
//				UpdateTiles = true;
//			}
//			else
//			{
//				TileUpdateCounter += DeltaTime;
//			}
//		}
//
//		if (UpdateTiles)
//		{
//			TileZoom = Zoom;
//			CashedCameraLat = CameraLat;
//			CashedCameraLon = CameraLon;
//			TileUpdateCounter = 0.0;
//
//			s32 MinTileX;
//			s32 MaxTileX;
//
//			s32 MinTileY;
//			s32 MaxTileY;
//
//			s32 TilesCountY;
//
//			const s32 TilesPerAxis = GetTilesPerAxis(TileZoom);
//
//			GetCameraVisibleRange(CameraLat, CameraLon, HorizontalAngle, VerticalAngle, TilesPerAxis,
//				MinTileX, MaxTileX, MinTileY, MaxTileY, TilesCountY);
//
//			s32 ClampedMinTileX;
//			s32 ClampedMinTileY;
//
//			if (TestDownload)
//			{
//				ClampedMinTileX = (MinTileX + TilesPerAxis) % TilesPerAxis;
//				ClampedMinTileY = glm::clamp(MinTileY, 0, TilesPerAxis - 1);
//
//				MapTileSettings.TextureTilesPerAxis = TilesPerAxis;
//				MapTileSettings.MinTileX = ClampedMinTileX;
//				MapTileSettings.MinTileY = ClampedMinTileY;
//				MapTileSettings.TilesCountY = TilesCountY;
//			}
//
//			std::vector<Tile> TilesToDownload;
//
//			for (s32 x = MinTileX; x < MaxTileX; ++x)
//			{
//				const s32 ValidX = (x + TilesPerAxis) % TilesPerAxis;
//
//				for (s32 y = MinTileY; y < MaxTileY; ++y)
//				{
//					const s32 ValidY = glm::clamp(y, 0, TilesPerAxis - 1);
//
//					if (TestDownload)
//					{
//						Tile T;
//						T.Zoom = TileZoom;
//						T.X = ValidX;
//						T.Y = ValidY;
//
//						const s32 OffsetX = (ValidX - ClampedMinTileX + TilesPerAxis) % TilesPerAxis;
//						const s32 OffsetY = (ValidY - ClampedMinTileY + TilesPerAxis) % TilesPerAxis;
//						T.ArrayLayer = OffsetX * (TilesCountY + 1) + OffsetY;
//
//						TilesToDownload.emplace_back(T);
//					}
//				}
//			}
//
//			if (!TilesToDownload.empty())
//			{
//				std::thread Thread(&DownloadTiles, TilesToDownload);
//				Thread.detach();
//			}
//
//			//TestDownload = false;
//		}
//
//		//Render::RenderTexture* Texture = ResourceManager::FindTexture(TilesTextureId);
//
//		{
//			std::unique_lock Lock(QueueMutex);
//			while (!DownloadQueue.empty())
//			{
//				const Tile& TileDataCompressed = DownloadQueue.front();
//
//				unsigned char* data = reinterpret_cast<unsigned char*>(const_cast<char*>(TileDataCompressed.Data.data()));
//				int width, height, channels;
//				unsigned char* image = stbi_load_from_memory(data, TileDataCompressed.Data.size(), &width, &height, &channels, 4);
//
//				//Render::TextureArrayInfo Info;
//				//Info.Width = TextureTileSize;
//				//Info.Height = TextureTileSize;
//				//Info.Format = 4;
//				//Info.LayersCount = 1;
//				//Info.BaseArrayLayer = TileDataCompressed.ArrayLayer;
//				//Info.Data = &image;
//
//				//Render::UpdateTexture(Texture, &Info);
//
//				DownloadQueue.pop();
//			}
//		}
//	}
//
//	void OnDraw()
//	{
//		FrameManager::UpdateUniformMemory(SettingsBufferHandle, &MapTileSettings, sizeof(TileSettingsBuffer));
//
//		const VkDescriptorSet DescriptorSetGroup[] = {
//			FrameManager::GetViewProjectionSet(),
//			//Scene.MapEntity.TextureSet,
//			MapTileSettingsSet
//		};
//
//		const u32 DynamicOffsets[] = {
//			{ VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer) },
//			{ VulkanInterface::TestGetImageIndex() * sizeof(TileSettingsBuffer) },
//		};
//
//		const u32 DescriptorCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);
//		const u32 DynamicOffsetCount = sizeof(DynamicOffsets) / sizeof(DynamicOffsets[0]);
//
//		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
//		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
//		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout, 0, DescriptorCount,
//			DescriptorSetGroup, DynamicOffsetCount, DynamicOffsets);
//
//		vkCmdBindIndexBuffer(CmdBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
//		vkCmdDrawIndexed(CmdBuffer, TestIndicesCount, 1, 0, 0, 0);
//	}
//
//	void DeInit()
//	{
//		StopDownload = true;
//
//		VkDevice Device = VulkanInterface::GetDevice();
//
//		vkDestroyBuffer(Device, IndexBuffer.Buffer, nullptr);
//		vkFreeMemory(Device, IndexBuffer.Memory, nullptr);
//
//		vkDestroyDescriptorSetLayout(Device, MapTileSettingsLayout, nullptr);
//		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
//		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
//
//		std::unique_lock DeInitLock(DeInitMutex);
//	}
//
//	glm::vec3 SphericalToMercator(const glm::vec3& Position)
//	{
//		//const f64 Latitude = glm::half_pi<f64>() * Position.x; // Map [-1, 1] -> [-p/2, p/2]
//
//		// Map[-1, 1] -> Mercator latitude in radians [-p/2, p/2]
//		const f64 Latitude = glm::atan(glm::sinh(glm::pi<f64>() * Position.x));
//		const f64 Longitude = glm::pi<f64>() * Position.y; // Map [-1, 1] -> [-p, p]
//		const f64 Altitude = Position.z;
//
//		const f64 CosLatitude = glm::cos(Latitude);
//		const f64 SinLatitude = glm::sin(Latitude);
//
//		glm::vec3 CartesianPos;
//		CartesianPos.x = Altitude * CosLatitude * glm::sin(Longitude);
//		CartesianPos.y = Altitude * SinLatitude;
//		CartesianPos.z = Altitude * CosLatitude * glm::cos(Longitude);
//
//		return CartesianPos;
//	}
//
//	f64 CalculateCameraAltitude(s32 ZoomLevel)
//	{
//		return Math::Circumference / glm::pow(2, ZoomLevel - 1);
//	}
//
//	void TestSetDownload(bool Download)
//	{
//		TestDownload = Download;
//	}
//
//	s32 GetTilesPerAxis(s32 Zoom)
//	{
//		assert(Zoom <= MaxZoom);
//		return 1 << glm::clamp(Zoom, MinZoom, MaxZoom);
//	}
//
//	void DownloadTiles(const std::vector<Tile>& Tiles)
//	{
//		std::unique_lock DeInitLock(DeInitMutex);
//
//		for (s32 i = 0; i < Tiles.size(); ++i)
//		{
//			if (StopDownload == true)
//			{
//				return;
//			}
//
//			Tile TileToDownload = Tiles[i];
//
//			const std::string Path = "/" + std::to_string(TileToDownload.Zoom) + "/" +
//				std::to_string(TileToDownload.X) + "/" + std::to_string(TileToDownload.Y) + ".png";
//
//			auto start = std::chrono::high_resolution_clock::now();
//
//			httplib::Result Res = Client->Get(Path);
//
//			auto end = std::chrono::high_resolution_clock::now();
//			std::chrono::duration<double> duration = end - start;
//			std::cout << "Request took " << duration.count() << " seconds" << std::endl;
//
//			if (Res && Res->status == httplib::StatusCode::OK_200)
//			{
//				std::unique_lock Lock(QueueMutex);
//
//				TileToDownload.Data = std::move(Res->body);
//				DownloadQueue.emplace(std::move(TileToDownload));
//			}
//		}
//	}
//
//	void GetCameraVisibleRange(f64 CameraLat, f64 CameraLon, f64 HorizontalAngle, f64 VerticalAngle, s32 TilesPerAxis,
//		s32& MinTileX, s32& MaxTileX, s32& MinTileY, s32& MaxTileY, s32& TilesCountY)
//	{
//		const f64 RelativeTileAngularSize = 360.0 / TilesPerAxis * cos(glm::radians(CameraLat));
//
//		const s32 CameraTileX = LonToTileX(CameraLon, TilesPerAxis);
//		const s32 CameraTileY = LatToTileY(CameraLat, TilesPerAxis);
//
//		s32 TilesCountX = ceil(HorizontalAngle / RelativeTileAngularSize);
//		TilesCountY = ceil(VerticalAngle / RelativeTileAngularSize);
//
//		TilesCountY += (TilesCountY >= 0) ? 1 : -1;
//
//		if (TilesCountY % 2 != 0)
//		{
//			TilesCountY += (TilesCountY >= 0) ? 1 : -1;
//		}
//
//		TilesCountX += TilesCountX / 2;
//		TilesCountY += TilesCountY / 2;
//
//		MinTileX = CameraTileX - TilesCountX / 2;
//		MaxTileX = CameraTileX + TilesCountX / 2;
//
//		MinTileY = CameraTileY - TilesCountY / 2;
//		MaxTileY = CameraTileY + TilesCountY / 2;
//
//		//MaxVertexTileX += (MaxVertexTileX >= 0) ? 1 : -1;
//		//MaxVertexTileY += (MaxVertexTileY > 0) ? 1 : -1;
//
//		//MinVertexTileX -= (MinVertexTileX >= 0) ? 1 : -1;
//		//MinVertexTileY -= (MinVertexTileY > 0) ? 1 : -1;
//	}
//
//	s32 LonToTileX(f64 Lon, s32 TilePerAxis)
//	{
//		return (s32)(floor((Lon + 180.0) / 360.0 * TilePerAxis));
//	}
//
//	s32 LatToTileY(f64 Lat, s32 TilePerAxis)
//	{
//		f64 LatRad = Lat * glm::pi<f64>() / 180.0;
//		return (s32)(floor((1.0 - asinh(tan(LatRad)) / glm::pi<f64>()) / 2.0 * TilePerAxis));
//	}
//}

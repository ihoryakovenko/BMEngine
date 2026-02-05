#pragma once

#include <ShortTypes.h>

#include <glm/glm.hpp>

#include <RenderInterface.h>

struct GLFWwindow;

struct StreetsRender_BuildingVertex
{
	glm::ivec2 NanoDegPosition;
	float AltitudeMeters;
	glm::vec3 Color;
	glm::vec3 Normal;
};

struct StreetsRender_FrameData
{
	glm::mat4 vp;
	glm::ivec2 CameraWorldNanoDegPosition;
	f32 CameraWorldAltitudeMeters;
	f32 _pad;
	glm::vec2 MetersPerNanoDegLonLat;
	s32 DebugMode;
};

struct StreetsRender_3DObjectsTile
{
	BmRender_GPUBuffer VertexBuffer;
	BmRender_GPUBuffer IndexBuffer;
	BmRender_GPUBuffer InstanceBuffer;
	BmRender_GPUBuffer IndirectBuffer;
	u32 VertexCount;
	u32 IndexCount;
	u32 CommandCount;
};

struct StreetsRender_3DObjectRange
{
	u32 FirstIndex;
	u32 IndexCount;
};

struct StreetsRender_3DObjectInstance
{
	u32 MaterialIndex;
};

struct StreetsRender_3DObjectsTileCreateData
{
	StreetsRender_BuildingVertex* Vertices;
	u32* Indices;
	StreetsRender_3DObjectInstance* Instances;
	StreetsRender_3DObjectRange* Ranges;
	u32 VertexCount;
	u32 IndexCount;
	u32 RangesCount;
};

struct StreetsRender_Material
{
	alignas (16) glm::vec3 BaseColor;
	f32 Metallic;
	f32 Roughness;
};

int StreetsRender_Init(GLFWwindow* Window, s32 WindowWidth, s32 WindowHeight);
void StreetsRender_DeInit();

void StreetsRender_Draw(StreetsRender_FrameData* FrameData, StreetsRender_3DObjectsTile* Buildings, u32 MeshCount);

StreetsRender_3DObjectsTile StreetsRender_Create3DObjectsTile(StreetsRender_3DObjectsTileCreateData* TileData);
void StreetsRender_Destroy3DObjectsTile(StreetsRender_3DObjectsTile* Mesh);

void StreetsRender_CreateMaterials(StreetsRender_Material* Materials, u32 MaterialsCount);
void StreetsRender_DestroyMaterials();
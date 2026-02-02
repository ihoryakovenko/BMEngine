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
};

struct StreetsRender_FrameData
{
	glm::mat4 vp;
	glm::ivec2 CameraWorldNanoDegPosition;
	f32 CameraWorldAltitudeMeters;
	f32 _pad;
	glm::vec2 MetersPerNanoDegLonLat;
};

struct StreetsRender_BuildingsMesh
{
	BmRender_GPUBuffer VertexBuffer;
	BmRender_GPUBuffer IndexBuffer;
	u32 VertexCount;
	u32 IndexCount;
};

int StreetsRender_Init(GLFWwindow* Window, s32 WindowWidth, s32 WindowHeight);
void StreetsRender_DeInit();

void StreetsRender_Draw(StreetsRender_FrameData* FrameData, StreetsRender_BuildingsMesh* Buildings, u32 MeshCount);

StreetsRender_BuildingsMesh StreetsRender_CreateBuildingsMesh(StreetsRender_BuildingVertex* Vertices, u32 VertexCount, u32* Indices, u32 IndexCount);
void StreetsRender_DestroyBuildingsMesh(StreetsRender_BuildingsMesh* Mesh);
#pragma once

#include <ShortTypes.h>

#include <glm/glm.hpp>

#include <RenderInterface.h>

struct GLFWwindow;

struct StreetsRender_Vertex {
	glm::vec3 pos;
};

struct StreetsRender_Mesh
{
	BmRender_GPUBuffer VertexBuffer;
	BmRender_GPUBuffer IndexBuffer;
	u32 VertexCount;
	u32 IndexCount;
};

int StreetsRender_Init(GLFWwindow* Window, s32 WindowWidth, s32 WindowHeight);
void StreetsRender_DeInit();

void StreetsRender_Draw(glm::mat4 vp, StreetsRender_Mesh* Meshes, u32 MeshCount);

StreetsRender_Mesh StreetsRender_CreateMesh(StreetsRender_Vertex* Vertices, u32 VertexCount, u16* Indices, u32 IndexCount);
void StreetsRender_DestroyMesh(StreetsRender_Mesh* Mesh);
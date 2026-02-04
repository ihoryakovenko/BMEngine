#pragma once

#include <vector>

#include "ShortTypes.h"
#include "Render.h"

#pragma warning(disable : 4996)

#define NOMINMAX
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/tags/filter.hpp>
#include <osmium/geom/wkt.hpp>

#include <earcut.hpp>

struct WayGeometry
{
	bool IsClockwise;
	std::vector<std::array<s32, 2>> Ring;
};

struct BuildingWay
{
	WayGeometry Way;
	f32 Height;
	f32 MinHeight;
	bool IsOutline;
};

struct Mesh
{
	std::vector<StreetsRender_BuildingVertex> vertices;
	std::vector<u32> Indices;
	std::vector<StreetsRender_3DObjectRange> Ranges;
};

struct BuildingHandler : public osmium::handler::Handler
{
	Mesh TestMesh;
	std::unordered_map<osmium::object_id_type, WayGeometry> Ways;
	std::unordered_map<osmium::object_id_type, BuildingWay> BuildingWays;

	void ConstructObjects();

	void ProcessMultipolygonRelation(const osmium::Relation& Rel);
	void GenerateBuildingWall(Mesh& mesh, s32 CurrentNanoDegX, s32 CurrentNanoDegY, s32 NextNanoDegX, s32 NextNanoDegY, f32 Height, f32 MinHeight);
	bool GetBuildingData(const osmium::OSMObject& Object, f32& OutHeight, f32& OutMinHeight);
	void RemoveColinearPoints(std::vector<std::array<s32, 2>>& Ring);

	void way(const osmium::Way& Way);

	void relation(const osmium::Relation& rel);

	const f32 LevelHeight = 3.0f;
};
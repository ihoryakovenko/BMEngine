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
#include <glm/glm.hpp>

// OSM building:material values (https://wiki.openstreetmap.org/wiki/Key:building:material)
enum class BuildingMaterial : u32
{
	Empty,

	CementBlock,
	Brick,
	Plaster,
	Wood,
	Concrete,
	Metal,
	Steel,
	Stone,
	Glass,
	Mirror,
	Mud,
	Masonry,
	Tin,
	Plastic,
	TimberFraming,
	Sandstone,
	Clay,
	Reed,
	Loam,
	Marble,
	Copper,
	Slate,
	Vinyl,
	Limestone,
	Tiles,
	Pebbledash,
	MetalPlates,
	Bamboo,
	Adobe,
	RammedEarth,
	SolarPanels,
	Tyres,

	MAX
};

extern StreetsRender_Material BuildingMaterials[(u32)BuildingMaterial::MAX];

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
	BuildingMaterial Material;
	bool IsOutline;
};

struct Mesh
{
	std::vector<StreetsRender_BuildingVertex> vertices;
	std::vector<u32> Indices;
	std::vector<StreetsRender_3DObjectRange> Ranges;
	std::vector<StreetsRender_3DObjectInstance> Instances;
};

struct BuildingHandler : public osmium::handler::Handler
{
	Mesh TestMesh;
	std::unordered_map<osmium::object_id_type, WayGeometry> Ways;
	std::unordered_map<osmium::object_id_type, BuildingWay> BuildingWays;
	std::unordered_map<std::string, BuildingMaterial> MaterialTable;

	void ConstructObjects();

	void ProcessMultipolygonRelation(const osmium::Relation& Rel);
	void GenerateBuildingWall(Mesh& mesh, s32 CurrentNanoDegX, s32 CurrentNanoDegY, s32 NextNanoDegX, s32 NextNanoDegY, f32 Height, f32 MinHeight);
	void AddBuildingPolygonGeometry(Mesh& mesh, const std::vector<std::vector<std::array<s32, 2>>>& Polygon, f32 Height, f32 MinHeight, const glm::vec3& RoofColor);
	bool GetBuildingData(const osmium::OSMObject& Object, f32& OutHeight, f32& OutMinHeight, BuildingMaterial& OutMaterial);
	void RemoveColinearPoints(std::vector<std::array<s32, 2>>& Ring);

	void way(const osmium::Way& Way);

	void relation(const osmium::Relation& rel);

	void InitializeMaterials();

	const f32 LevelHeight = 3.0f;
};
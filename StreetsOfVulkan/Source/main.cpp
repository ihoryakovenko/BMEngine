#include <ShortTypes.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render.h"

#pragma warning(disable : 4996)

#include <iostream>

#define NOMINMAX
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/tags/filter.hpp>
#include <osmium/geom/wkt.hpp>

#include <vector>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

#include <earcut.hpp>

struct Mesh
{
	std::vector<StreetsRender_Vertex> vertices;
	std::vector<u32> Indices;
};

Mesh TestMesh;

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

constexpr f64 R = 6378137.0; // Earth radius
constexpr f64 DEG_TO_RAD = glm::pi<f64>() / 180.0;

// Tile info
s32 center_nanodeg_x;
s32 center_nanodeg_y;

f64 lat_rad;

f64 meters_per_nanodeg_lat;
f64 meters_per_nanodeg_lon;
// Tile info



struct BuildingHandler : public osmium::handler::Handler
{

	std::unordered_map<osmium::object_id_type, const osmium::Way*> Ways;

	void way(const osmium::Way& Way)
	{
		Ways[Way.id()] = &Way;

		if (!Way.tags().has_key("building"))
		{
			return;
		}

		if (!Way.is_closed())
		{
			return;
		}

		u32 BaseVertex = TestMesh.vertices.size();
		const u64 NodeCount = Way.nodes().size();

		std::vector<std::array<s32, 2>> Ring;
		Ring.reserve(NodeCount);

		u64 Area = 0.0;

		for (u64 i = 0; i < NodeCount; ++i)
		{
			const osmium::NodeRef& CurrentNode = Way.nodes()[i];
			const osmium::NodeRef& NextNode = Way.nodes()[(i + 1) % NodeCount];

			const osmium::Location& CurrentLoc = CurrentNode.location();
			const osmium::Location& NextLoc = NextNode.location();

			if (!CurrentLoc.valid() || !NextLoc.valid())
			{
				assert(false);
				return;
			}

			Area += (u64)CurrentLoc.x() * (u64)NextLoc.y() - (u64)NextLoc.x() * (u64)CurrentLoc.y();

			Ring.push_back({ CurrentLoc.x(), CurrentLoc.y() });
		}

		const f32 Height = 30.0f;

		const bool IsClockwise = (Area < 0.0);
		if (IsClockwise)
		{
			Ring.reserve(Ring.size());
		}

		for (u32 i = 0; i < Ring.size(); ++i)
		{
			const s32 CurrentNanoDegX = Ring[i][0];
			const s32 CurrentNanoDegY = Ring[i][1];

			TestMesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height });
		}

		std::vector<std::vector<std::array<s32, 2>>> Polygon;
		Polygon.push_back(Ring);

		std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);

		for (u32 i = 0; i < Indices.size(); ++i)
		{
			TestMesh.Indices.push_back(Indices[i] + BaseVertex);
		}

		for (u32 i = 0; i < NodeCount; ++i)
		{
			BaseVertex = TestMesh.vertices.size();

			const s32 CurrentNanoDegX = Ring[i][0];
			const s32 CurrentNanoDegY = Ring[i][1];

			const s32 NextNanoDegX = Ring[(i + 1) % NodeCount][0];
			const s32 NextNanoDegY = Ring[(i + 1) % NodeCount][1];

			TestMesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height });
			TestMesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), 0.0f });

			TestMesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), 0.0f });
			TestMesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), Height });

			TestMesh.Indices.push_back(BaseVertex + 2);
			TestMesh.Indices.push_back(BaseVertex + 3);
			TestMesh.Indices.push_back(BaseVertex + 0);

			TestMesh.Indices.push_back(BaseVertex + 0);
			TestMesh.Indices.push_back(BaseVertex + 1);
			TestMesh.Indices.push_back(BaseVertex + 2);
		}
	}

	//void relation(const osmium::Relation& rel)
	//{
	//	if (!rel.tags().has_tag("type", "multipolygon"))
	//	{
	//		return;
	//	}

	//	if (!rel.tags().has_key("building"))
	//	{
	//		return;
	//	}

	//	std::vector<std::vector<std::array<f32, 2>>> polygon;

	//	for (const osmium::RelationMember& member : rel.members())
	//	{
	//		if (member.type() != osmium::item_type::way)
	//		{
	//			continue;
	//		}

	//		auto it = ways.find(member.ref());
	//		if (it == ways.end())
	//		{
	//			continue;
	//		}

	//		const osmium::Way& way = *it->second;
	//		if (!way.is_closed())
	//		{
	//			continue;
	//		}

	//		std::vector<std::array<f32, 2>> ring;
	//		if (build_ring(way, ring))
	//		{
	//			polygon.push_back(std::move(ring));
	//		}
	//	}

	//	if (polygon.empty())
	//	{
	//		return;
	//	}

	//	emit_mesh(polygon);
	//}
};

struct FlyCamera
{
	glm::ivec2 WorldNanoDegPosition;
	f32 Altitude;

	f32 Yaw;
	f32 Pitch;

	f32 Aspect;
	f32 FOV;
	f32 NearPlane;
	f32 FarPlane;
};

u32 main()
{
	osmium::io::Reader reader("C:/Users/igor_/Desktop/planet_19.91022,50.0482_19.97117,50.07131.osm.pbf", osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);

	const auto& header = reader.header();
	const osmium::Box& box = header.box();

	if (box.valid())
	{
		center_nanodeg_x = (box.bottom_left().x() + box.top_right().x()) / 2;
		center_nanodeg_y = (box.bottom_left().y() + box.top_right().y()) / 2;

		lat_rad = (center_nanodeg_y / 1e7) * DEG_TO_RAD;

		meters_per_nanodeg_lat = R * DEG_TO_RAD / 1e7;
		meters_per_nanodeg_lon = R * DEG_TO_RAD * std::cos(lat_rad) / 1e7;
	}
	else
	{
		assert(false);
	}

	index_type index;
	location_handler_type location_handler{ index };
	location_handler.ignore_errors();

	BuildingHandler building_handler;

	osmium::apply(reader, location_handler, building_handler);
	reader.close();

	s32 WindowWidth = 1920;
	s32 WindowHeight = 1080;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
	StreetsRender_Init(Window, WindowWidth, WindowHeight);

	FlyCamera Camera;
	Camera.WorldNanoDegPosition = glm::ivec2(center_nanodeg_x, center_nanodeg_y);
	Camera.Altitude = 100.0f;

	Camera.Yaw = glm::radians(90.0f);
	Camera.Pitch = 0.0f;

	Camera.Aspect = (f32)WindowWidth / (f32)WindowHeight;
	Camera.FOV = glm::radians(45.0f);
	Camera.NearPlane = 0.1f;
	Camera.FarPlane = 10000.0f;

	glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	f32 mouseSensitivity = 0.001f;
	f64 lastCursorX = (f64)WindowWidth / 2.0;
	f64 lastCursorY = (f64)WindowHeight / 2.0;
	bool firstMouse = true;

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	f32 time = 0.0f;

	std::vector<StreetsRender_Mesh> Meshes;


	Meshes.push_back(StreetsRender_CreateMesh(TestMesh.vertices.data(), TestMesh.vertices.size(), TestMesh.Indices.data(), TestMesh.Indices.size()));

	while (!glfwWindowShouldClose(Window))
	{
		if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

		glfwPollEvents();
		time += 0.016f;

		f64 cursorX, cursorY;
		glfwGetCursorPos(Window, &cursorX, &cursorY);
		if (firstMouse)
		{
			lastCursorX = cursorX;
			lastCursorY = cursorY;
			firstMouse = false;
		}
		f32 deltaX = (f32)(cursorX - lastCursorX);
		f32 deltaY = (f32)(cursorY - lastCursorY);
		lastCursorX = cursorX;
		lastCursorY = cursorY;

		Camera.Yaw += deltaX * mouseSensitivity;
		Camera.Pitch -= deltaY * mouseSensitivity;
		f32 pitchLimit = glm::radians(89.0f);
		if (Camera.Pitch > pitchLimit)  Camera.Pitch = pitchLimit;
		if (Camera.Pitch < -pitchLimit) Camera.Pitch = -pitchLimit;

		glm::vec3 CameraFront;
		CameraFront.x = std::cos(Camera.Pitch) * std::cos(Camera.Yaw);
		CameraFront.y = std::sin(Camera.Pitch);
		CameraFront.z = std::cos(Camera.Pitch) * std::sin(Camera.Yaw);

		const glm::vec3 CameraRight = glm::normalize(glm::cross(CameraFront, WorldUp));

		glm::vec3 movement = glm::vec3(0.0f);
		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) movement += CameraFront;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) movement -= CameraFront;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) movement -= CameraRight;
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) movement += CameraRight;

		if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS) movement += WorldUp;
		if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) movement -= WorldUp;

		movement *= 10.0f;
		Camera.WorldNanoDegPosition.x += (s32)std::round(movement.z / meters_per_nanodeg_lon);
		Camera.WorldNanoDegPosition.y += (s32)std::round(movement.x / meters_per_nanodeg_lat);
		Camera.Altitude += (s32)std::round(movement.y);

		glm::mat4 proj = glm::perspective(Camera.FOV, Camera.Aspect, Camera.NearPlane, Camera.FarPlane);
		proj[1][1] *= -1;
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), CameraFront, WorldUp);

		glm::mat4 vp = proj * view;

		StreetsRender_FrameData FrameData;
		FrameData.CameraWorldAltitudeMeters = Camera.Altitude;
		FrameData.CameraWorldNanoDegPosition = Camera.WorldNanoDegPosition;
		FrameData.MetersPerNanoDegLonLat = glm::vec2((f32)meters_per_nanodeg_lon, (f32)meters_per_nanodeg_lat);
		FrameData.vp = vp;
		StreetsRender_Draw(&FrameData, Meshes.data(), Meshes.size());
	}

	for (u32 i = 0; i < Meshes.size(); ++i)
	{
		StreetsRender_DestroyMesh(&Meshes[i]);
	}

	StreetsRender_DeInit();

	return 0;
}
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

s32 limit_lat_nd;
s32 limit_lon_nd;
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

		const u32 BaseVertex = TestMesh.vertices.size();
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
			const s32 dx = Ring[i][0] - center_nanodeg_x;
			const s32 dy = Ring[i][1] - center_nanodeg_y;

			const f32 x = f32(dx * meters_per_nanodeg_lon);
			const f32 y = f32(dy * meters_per_nanodeg_lat);

			TestMesh.vertices.push_back({ glm::vec3(y, Height, x) });
		}

		std::vector<std::vector<std::array<s32, 2>>> Polygon;
		Polygon.push_back(std::move(Ring));

		std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);

		for (u32 i = 0; i < Indices.size(); ++i)
		{
			TestMesh.Indices.push_back(Indices[i] + BaseVertex);
		}

		//for (u32 i = 0; i < Ring.size(); ++i)
		//{
		//	const s32 CurrentNanoDegX = Ring[i][0];
		//	const s32 CurrentNanoDegY = Ring[i][1];

		//	const s32 NextNanoDegX = Ring[(i + 1) % NodeCount][0];
		//	const s32 NextNanoDegY = Ring[(i + 1) % NodeCount][1];


		//}
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

u32 main()
{
	osmium::io::Reader reader("", osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);

	const auto& header = reader.header();
	const osmium::Box& box = header.box();

	if (box.valid())
	{
		center_nanodeg_x = (box.bottom_left().x() + box.top_right().x()) / 2;
		center_nanodeg_y = (box.bottom_left().y() + box.top_right().y()) / 2;

		lat_rad = (center_nanodeg_y / 1e7) * DEG_TO_RAD;

		meters_per_nanodeg_lat = R * DEG_TO_RAD / 1e7;
		meters_per_nanodeg_lon = R * DEG_TO_RAD * std::cos(lat_rad) / 1e7;

		limit_lat_nd = s32(5000.0f / meters_per_nanodeg_lat);
		limit_lon_nd = s32(5000.0f / meters_per_nanodeg_lon);
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



	f32 aspect = (f32)WindowWidth / (f32)WindowHeight;
	f32 fov = glm::radians(45.0f);
	f32 nearPlane = 0.1f;
	f32 farPlane = 10000.0f;

	glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	f32 cameraSpeed = 0.05f;
	f32 mouseSensitivity = 0.001f;

	f32 yaw = glm::radians(90.0f);
	f32 pitch = 0.0f;
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

		yaw += deltaX * mouseSensitivity;
		pitch -= deltaY * mouseSensitivity;
		f32 pitchLimit = glm::radians(89.0f);
		if (pitch > pitchLimit)  pitch = pitchLimit;
		if (pitch < -pitchLimit) pitch = -pitchLimit;

		glm::vec3 cameraFront;
		cameraFront.x = std::cos(pitch) * std::cos(yaw);
		cameraFront.y = std::sin(pitch);
		cameraFront.z = std::cos(pitch) * std::sin(yaw);
		cameraFront = glm::normalize(cameraFront);

		glm::vec3 movement = glm::vec3(0.0f);
		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) movement += cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) movement -= cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) movement -= glm::normalize(glm::cross(cameraFront, cameraUp));
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) movement += glm::normalize(glm::cross(cameraFront, cameraUp));
		if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS) movement += cameraUp;
		if (glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) movement -= cameraUp;

		cameraPos += movement * 10.0f;

		glm::mat4 proj = glm::perspective(fov, aspect, nearPlane, farPlane);
		proj[1][1] *= -1;
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		f32 rotY = time * 0.5f;
		f32 rotX = time * 0.3f;
		glm::mat4 model = glm::mat4(1.0f);
		//model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
		//model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 vp = proj * view;

		StreetsRender_Draw(vp, Meshes.data(), Meshes.size());
	}

	for (u32 i = 0; i < Meshes.size(); ++i)
	{
		StreetsRender_DestroyMesh(&Meshes[i]);
	}

	StreetsRender_DeInit();

	return 0;
}
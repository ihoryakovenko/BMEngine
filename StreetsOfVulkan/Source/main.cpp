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

//double computeSignedArea(const QVector<PointI>& points)
//{
//	double area = 0.0;
//	const int n = points.size();
//	for (int i = 0; i < n; ++i)
//	{
//		const auto& p0 = points[i];
//		const auto& p1 = points[(i + 1) % n];
//		area += static_cast<double>(p0.x) * p1.y - static_cast<double>(p1.x) * p0.y;
//	}
//
//	return 0.5 * area;
//}

struct Mesh
{
	std::vector<StreetsRender_Vertex> vertices;
	std::vector<u16> Indices;
};

std::vector<Mesh> objects;

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

s32 center_m_x;
s32 center_m_y;

constexpr double R = 6378137.0; // Earth radius
constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;

struct BuildingHandler : public osmium::handler::Handler
{

	std::unordered_map<osmium::object_id_type, const osmium::Way*> ways;

	bool build_ring(const osmium::Way& way, std::vector<std::array<float, 2>>& ring)
	{
		for (const osmium::NodeRef& nr : way.nodes()) {
			const osmium::Location& loc = nr.location();
			if (!loc.valid()) {
				return false;
			}

			int32_t dx = loc.x() - center_m_x;
			int32_t dy = loc.y() - center_m_y;

			double lon_scale = R * DEG_TO_RAD * std::cos(center_m_y / 1e7 * DEG_TO_RAD) / 1e7;
			double lat_scale = R * DEG_TO_RAD / 1e7;

			float x = float(dx * lon_scale);
			float y = float(dy * lat_scale);

			if (x < -500 || x > 500 || y < -500 || y > 500) {
				continue;
			}

			ring.push_back({ x, y });
		}
		return !ring.empty();
	}

	void emit_mesh(const std::vector<std::vector<std::array<float, 2>>>& polygon)
	{
		Mesh mesh;
		mesh.Indices = mapbox::earcut<u16>(polygon);
		if (mesh.Indices.empty())
		{
			return;
		}

		for (const auto& ring : polygon)
		{
			for (const auto& p : ring)
			{
				mesh.vertices.push_back({ glm::vec3(p[0], 0.0f, p[1]) });
			}
		}

		objects.push_back(std::move(mesh));
	}

	void way(const osmium::Way& way)
	{
		ways[way.id()] = &way;

		if (!way.tags().has_key("building"))
		{
			return;
		}

		if (!way.is_closed())
		{
			return;
		}

		std::vector<std::array<float, 2>> ring;
		if (!build_ring(way, ring))
		{
			return;
		}

		std::vector<std::vector<std::array<float, 2>>> polygon;
		polygon.push_back(std::move(ring));

		emit_mesh(polygon);
	}

	void relation(const osmium::Relation& rel)
	{
		if (!rel.tags().has_tag("type", "multipolygon"))
		{
			return;
		}

		if (!rel.tags().has_key("building"))
		{
			return;
		}

		std::vector<std::vector<std::array<float, 2>>> polygon;

		for (const osmium::RelationMember& member : rel.members())
		{
			if (member.type() != osmium::item_type::way)
			{
				continue;
			}

			auto it = ways.find(member.ref());
			if (it == ways.end())
			{
				continue;
			}

			const osmium::Way& way = *it->second;
			if (!way.is_closed())
			{
				continue;
			}

			std::vector<std::array<float, 2>> ring;
			if (build_ring(way, ring))
			{
				polygon.push_back(std::move(ring));
			}
		}

		if (polygon.empty())
		{
			return;
		}

		emit_mesh(polygon);
	}
};

int main()
{
	osmium::io::Reader reader("", osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);

	const auto& header = reader.header();
	const osmium::Box& box = header.box();

	if (box.valid())
	{
		center_m_x = (box.bottom_left().x() + box.top_right().x()) / 2;
		center_m_y = (box.bottom_left().y() + box.top_right().y()) / 2;
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

	//Mesh TestMesh;

	//std::vector<std::array<float, 2>> wayObject = {
	//	{10.0, -10.0},
	//	{10.0, 10.0},
	//	{-10.0, 10.0},
	//	{-10.0, -10.0}
	//};

	//std::vector<std::vector<std::array<float, 2>>> polygon;
	//polygon.push_back(wayObject);
	//
	//TestMesh.Indices = mapbox::earcut<uint16_t>(polygon);
	//for (u32 i = 0; i < wayObject.size(); ++i)
	//{
	//	TestMesh.vertices.push_back({ glm::vec3(wayObject[i][0], 0.0f, wayObject[i][1])});
	//}
	//
	//objects.push_back(TestMesh);

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

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
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
	Meshes.reserve(objects.size());

	for (u32 i = 0; i < objects.size(); ++i)
	{
		auto& mesh = objects[i];
		Meshes.push_back(StreetsRender_CreateMesh(mesh.vertices.data(), mesh.vertices.size(), mesh.Indices.data(), mesh.Indices.size()));
	}

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

		cameraPos += movement * 3.0f;

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
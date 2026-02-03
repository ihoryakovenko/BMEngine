#include <ShortTypes.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render.h"



#include <iostream>





#include <vector>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

#include "OSMBuilder.h"

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
	srand((unsigned)time(nullptr));

	const osmium::osm_entity_bits::type ReadTypes = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation;

	osmium::io::Reader reader("C:/Users/igor_/Desktop/planet_19.91022,50.0482_19.97117,50.07131.osm.pbf", ReadTypes);

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

	std::vector<StreetsRender_BuildingsMesh> Meshes;


	Meshes.push_back(StreetsRender_CreateBuildingsMesh(building_handler.TestMesh.vertices.data(), building_handler.TestMesh.vertices.size(), building_handler.TestMesh.Indices.data(), building_handler.TestMesh.Indices.size()));

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
		StreetsRender_DestroyBuildingsMesh(&Meshes[i]);
	}

	StreetsRender_DeInit();

	return 0;
}
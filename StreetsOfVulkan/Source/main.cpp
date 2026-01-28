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

#define NOMINMAX
#include <osmium/io/any_input.hpp>

int main()
{
	osmium::io::Reader reader("");
	reader.close();

	s32 WindowWidth = 1920;
	s32 WindowHeight = 1080;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);
	StreetRender_Init(Window, WindowWidth, WindowHeight);



	f32 aspect = (f32)WindowWidth / (f32)WindowHeight;
	f32 fov = glm::radians(45.0f);
	f32 nearPlane = 0.1f;
	f32 farPlane = 100.0f;
	
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	f32 cameraSpeed = 0.05f;

	f32 time = 0.0f;

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		time += 0.016f;

		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += cameraSpeed * cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= cameraSpeed * cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
			cameraPos -= cameraUp * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
			cameraPos += cameraUp * cameraSpeed;

		glm::mat4 proj = glm::perspective(fov, aspect, nearPlane, farPlane);
		proj[1][1] *= -1;
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		f32 rotY = time * 0.5f;
		f32 rotX = time * 0.3f;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 mvp = proj * view * model;

		StreetRender_Draw(mvp);
	}

	StreetRender_DeInit();

	return 0;
}
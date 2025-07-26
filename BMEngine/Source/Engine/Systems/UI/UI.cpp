#include "UI.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <Engine/Systems/Render/Render.h>

namespace UI
{
	static GuiData* Data;

	void Init(GuiData* DataPtr)
	{
		Data = DataPtr;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& GuiIo = ImGui::GetIO();
		GuiIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		GuiIo.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
	}

	void DeInit()
	{
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Update()
	{
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Options");

		//ImGui::DragFloat3("DirectionLightDirection", &(*Data->DirectionLightDirection)[0], 0.05, -1, 1);
		//ImGui::DragFloat3("eye", &(*Data->Eye)[0], 0.05, -100, 100);

		Render::RenderState* State =  Render::GetRenderState();



		ImGuiIO& GuiIo = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GuiIo.Framerate, GuiIo.Framerate);
		ImGui::End();
	}
}
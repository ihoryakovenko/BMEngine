#include "DebugUI.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Render/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/MainPass.h"


// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
#endif

namespace DebugUi
{
// Data
	static VkAllocationCallbacks* g_Allocator = nullptr;

	static ImGui_ImplVulkanH_Window g_MainWindowData;

	static GLFWwindow* window = nullptr;

	static bool show_demo_window = false;
	static bool show_another_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	static double DeltaTime = 0.0f;
	static double LastTime = 0.0f;

	static float CameraLat = 50.45032261691907;
	static float CameraLon = 30.52466412479834;

	static GuiData* Data;

	void Init(GLFWwindow* Wnd, GuiData* DataPtr)
	{
		window = Wnd;
		Data = DataPtr;

		ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = MainPass::GetAttachmentData()->ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = MainPass::GetAttachmentData()->ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = MainPass::GetAttachmentData()->DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = MainPass::GetAttachmentData()->DepthAttachmentFormat;

		ImGui_ImplVulkan_InitInfo init_info = { };
		init_info.Instance = VulkanInterface::GetInstance();
		//init_info.Instance = g_Instance;
		init_info.PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		init_info.Device = VulkanInterface::GetDevice();
		init_info.QueueFamily = VulkanInterface::GetQueueGraphicsFamilyIndex();
		init_info.Queue = VulkanInterface::GetTransferQueue();
		//init_info.PipelineCache = g_PipelineCache;
		init_info.PipelineCache = nullptr;
		init_info.DescriptorPool = VulkanInterface::GetDescriptorPool();
		init_info.RenderPass = nullptr;
		init_info.UseDynamicRendering = true;
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = g_Allocator;
		init_info.CheckVkResultFn = nullptr;
		init_info.PipelineRenderingCreateInfo = RenderingInfo;
		//init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info);
	}

	void DeInit()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Draw()
	{
		const double CurrentTime = glfwGetTime();
		DeltaTime = CurrentTime - LastTime;
		LastTime = CurrentTime;

		//if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		//{
		//	ImGui_ImplGlfw_Sleep(10);
		//	return;
		//}

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Options");

			ImGui::DragFloat3("DirectionLightDirection", &(*Data->DirectionLightDirection)[0], 0.05, -1, 1);
			ImGui::DragFloat3("eye", &(*Data->Eye)[0], 0.05, -100, 100);

			//ImGui::DragFloat2("CameraPos", &(*Data.CameraMercatorPosition)[0], 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Camera latitude", &CameraLat, 1.01f, -90.0f, 90.0f);
			ImGui::DragFloat("Camera longitude", &CameraLon, 1.01f, -180.0f, 180.0f);
			ImGui::DragInt("Zoom", &(*Data->Zoom), 0.05f, 1.0f, 20.0f);

			float LatRadians = glm::radians(CameraLat);
			float MercatorLat = std::log(std::tan(glm::pi<float>() / 4.0f + LatRadians / 2.0f));
			float NormalizedMercatorLat = MercatorLat / glm::pi<float>();

			Data->CameraMercatorPosition->x = NormalizedMercatorLat;
			Data->CameraMercatorPosition->y = CameraLon / 180.0f;

			if (ImGui::Button("Download tiles"))
			{
				Data->OnTestSetDownload(true);
			}


			//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			//ImGui::Checkbox("Another Window", &show_another_window);

			//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			//if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			//	counter++;
			//ImGui::SameLine();
			//ImGui::Text("counter = %d", counter);

			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized)
		{
			VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
			ImGui_ImplVulkan_RenderDrawData(draw_data, CmdBuffer);
		}
	}
}
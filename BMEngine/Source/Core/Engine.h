#pragma once

#include "Window.h"
#include "VulkanRender.h"

namespace Core
{
	class Engine
	{
	public:
		bool Init();
		void GameLoop();
		void Terminate();

	private:
		Core::Window _Window;
		Core::VulkanRender _VulkanRender;
	};
}

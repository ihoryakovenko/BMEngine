#include "Settings.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

BmRender_Format ColorFormat = BmRender_Format::R8G8B8A8_UNORM; // Todo: check if R8G8B8A8_UNORM supported
BmRender_Format DepthFormat = BmRender_Format::D32_SFLOAT_S8_UINT;

void LoadSettings(u32 WindowWidth, u32 WindowHeight)
{
	MainScreenExtent = { WindowWidth, WindowHeight };
}

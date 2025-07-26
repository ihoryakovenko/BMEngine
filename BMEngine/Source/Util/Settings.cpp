#include "Settings.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

void LoadSettings(u32 WindowWidth, u32 WindowHeight)
{
	MainScreenExtent = { WindowWidth, WindowHeight };
}

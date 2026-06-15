#pragma once

#include <vulkan/vulkan.h>
#include "VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

#include <SharedLib.h>

struct GLFWwindow;

struct VulkanCoreContext;

struct PipelineData
{
	BmRender_PipelineLayout Layout;
};

void InitializeFrameMemory();
void DeMemory_LinearAllocator_Init();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory_LinearAllocator* GetFrameMemory();

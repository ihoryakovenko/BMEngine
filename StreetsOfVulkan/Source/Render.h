#pragma once

#include <ShortTypes.h>

#include <glm/glm.hpp>

struct GLFWwindow;

int StreetRender_Init(GLFWwindow* Window, s32 WindowWidth, s32 WindowHeight);
void StreetRender_Draw(glm::mat4 mvp);
void StreetRender_DeInit();
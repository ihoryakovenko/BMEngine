#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;

layout(binding = 0) uniform MVP
{
	mat4 Model;
	mat4 View;
	mat4 Projection;
} Mvp;

layout(location = 0) out vec3 FragmentColor;

void main()
{
	gl_Position = Mvp.Projection * Mvp.View * Mvp.Model * vec4(Position, 1.0);

	FragmentColor = Color;
}
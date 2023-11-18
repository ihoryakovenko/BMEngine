#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;

layout(binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(binding = 1) uniform UboModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec3 FragmentColor;

void main()
{
	gl_Position = ViewProjection.Projection * ViewProjection.View * Model.Model * vec4(Position, 1.0);

	FragmentColor = Color;
}
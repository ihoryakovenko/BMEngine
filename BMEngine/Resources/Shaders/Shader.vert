#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;
layout(location = 2) in vec2 TextureCoords;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

// TODO: Not in use!!!
layout(set = 0, binding = 1) uniform UboModel
{
	mat4 Model;
} Model_deprecated;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec3 FragmentColor;
layout(location = 1) out vec2 FragmentTexture;

void main()
{
	gl_Position = ViewProjection.Projection * ViewProjection.View * Model.Model * vec4(Position, 1.0);

	FragmentColor = Color;
	FragmentTexture = TextureCoords;
}
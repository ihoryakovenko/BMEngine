#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TextureCoords;
layout(location = 2) in vec3 Normal;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec2 FragmentTexture;
layout(location = 1) out vec3 FragmentNormal;
layout(location = 2) out vec4 WorldFragPos;

void main()
{
	FragmentTexture = TextureCoords;
	WorldFragPos = Model.Model * vec4(Position, 1.0);
	FragmentNormal = normalize(mat3(transpose(inverse(ViewProjection.View * Model.Model))) * Normal);

	gl_Position = ViewProjection.Projection * ViewProjection.View * Model.Model * vec4(Position, 1.0);
}
#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;
layout(location = 2) in vec2 TextureCoords;
layout(location = 3) in vec3 Normal;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(set = 4, binding = 0) uniform UBOLightSpaceMatrix
{
	mat4 Matrix;
} LightSpaceMatrix;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec3 FragmentColor;
layout(location = 1) out vec2 FragmentTexture;
layout(location = 2) out vec3 FragmentNormal;
layout(location = 3) out vec3 FragmentPosition;
layout(location = 4) out vec4 FragPosLightSpace;

void main()
{
	vec4 WorldFragPos = Model.Model * vec4(Position, 1.0);

	FragmentColor = Color;
	FragmentTexture = TextureCoords;
	FragmentNormal = normalize(mat3(transpose(inverse(ViewProjection.View * Model.Model))) * Normal);
	FragmentPosition = vec3(ViewProjection.View * WorldFragPos);
	FragPosLightSpace = LightSpaceMatrix.Matrix * WorldFragPos;

	gl_Position = ViewProjection.Projection * ViewProjection.View * Model.Model * vec4(Position, 1.0);
}
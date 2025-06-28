#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TextureCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in mat4 ModelMatrix;  // occupies locations 3,4,5,6
layout(location = 7) in uint MaterialIndex;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(location = 0) out vec2 FragmentTexture;
layout(location = 1) out vec3 FragmentNormal;
layout(location = 2) out vec4 WorldFragPos;
layout(location = 3) out flat uint FragmentMaterialIndex;

void main()
{
	FragmentTexture = TextureCoords;
	WorldFragPos = ModelMatrix * vec4(Position, 1.0);
	FragmentNormal = normalize(mat3(transpose(inverse(ViewProjection.View * ModelMatrix))) * Normal);
	FragmentMaterialIndex = MaterialIndex;

	gl_Position = ViewProjection.Projection * ViewProjection.View * ModelMatrix * vec4(Position, 1.0);
}
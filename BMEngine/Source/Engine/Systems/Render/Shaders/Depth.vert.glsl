#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in mat4 ModelMatrix;  // occupies locations 1,2,3,4

layout(set = 0, binding = 0) uniform LightSpaceMatrix
{
	mat4 Matrix;
} lightSpaceMatrix;

void main()
{
	gl_Position = lightSpaceMatrix.Matrix * ModelMatrix * vec4(Position, 1.0);
}
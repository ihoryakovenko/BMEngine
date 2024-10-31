#version 450

layout(location = 0) in vec3 Position;

layout(set = 0, binding = 0) uniform LightSpaceMatrix
{
	mat4 Matrix;
} lightSpaceMatrix;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

void main()
{
	gl_Position = lightSpaceMatrix.Matrix * Model.Model * vec4(Position, 1.0);
}
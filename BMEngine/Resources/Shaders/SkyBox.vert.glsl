#version 450

layout(location = 0) in vec3 Position;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(location = 0) out vec3 FragDirection;

void main()
{
	
	FragDirection = Position;

	vec4 Pos = ViewProjection.Projection * mat4(mat3(ViewProjection.View)) * vec4(Position, 1.0);
    gl_Position = Pos.xyww;
}
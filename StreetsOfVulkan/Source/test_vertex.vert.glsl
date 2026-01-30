#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
	mat4 vp;
} pc;

void main()
{
	gl_Position = pc.vp * vec4(inPosition, 1.0);
}
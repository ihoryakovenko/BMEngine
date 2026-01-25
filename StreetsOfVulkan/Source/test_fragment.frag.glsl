#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main()
{
	vec3 color = vec3(fragTexCoord, 0.5);
	outColor = vec4(color, 1.0);
}


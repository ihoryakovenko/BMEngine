#version 450


layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;

layout(set = 1, binding = 0) uniform sampler2D DiffuseTexture;

layout(location = 0) out vec4 OutColor;

void main()
{
	vec4 DiffuseTexture = texture(DiffuseTexture, FragmentTexture);
	OutColor = DiffuseTexture;
}

#version 450

layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;

layout(set = 1, binding = 0) uniform sampler2D TextureSampler;

layout(location = 0) out vec4 OutColour;

void main()
{
	OutColour = texture(TextureSampler, FragmentTexture) * vec4(FragmentColor, 1.0);
	//OutColour = vec4(FragmentColor, 1.0);
}

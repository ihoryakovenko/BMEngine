#version 450

layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;

layout(set = 1, binding = 0) uniform sampler2D TextureSampler;
layout(set = 2, binding = 0) uniform LightData
{
    vec3 LightColor;
} lightData;

layout(location = 0) out vec4 OutColour;

void main()
{
	vec4 LightAmplifiedColor = vec4(FragmentColor, 1.0) * vec4(lightData.LightColor, 1.0);
	OutColour = texture(TextureSampler, FragmentTexture) * LightAmplifiedColor;
	//OutColour = vec4(FragmentColor, 1.0);
}

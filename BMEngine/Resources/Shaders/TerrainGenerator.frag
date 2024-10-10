#version 450

layout(location = 0) in vec2 FragmentUV;

layout(set = 1, binding = 0) uniform sampler2D TextureSampler;

layout(location = 0) out vec4 OutColour;

void main()
{
	//OutColour = vec4(0.0, 0.5, 0.0, 1.0);
	OutColour = texture(TextureSampler, FragmentUV);
}
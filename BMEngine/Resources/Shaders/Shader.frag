#version 450

layout(location = 0) in vec3 FragColour;

layout(location = 0) out vec4 OutColour;

void main()
{
	OutColour = vec4(FragColour, 1.0);
}

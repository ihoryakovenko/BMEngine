#version 450

layout(location = 0) in vec3 FragmentColor;

layout(location = 0) out vec4 OutColour;

void main()
{
	OutColour = vec4(FragmentColor, 1.0);
}

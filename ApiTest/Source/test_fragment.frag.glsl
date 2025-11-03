#version 450
layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(fragUV.x, fragUV.y, (fragUV.x + fragUV.y) * 0.5, 1.0);
}


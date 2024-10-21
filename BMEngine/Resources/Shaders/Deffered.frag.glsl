#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput InputColour; // Colour output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput InputDepth;  // Depth output from subpass 1

layout(location = 0) out vec4 Colour;

void main()
{
	int xHalf = 1200;
	if (gl_FragCoord.x > xHalf)
	{
		float lowerBound = 0.99;
		float upperBound = 1;
		
		float depth = subpassLoad(InputDepth).r;
		float depthColourScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));
		Colour = vec4(depthColourScaled, 0.0f, 0.0f, 1.0f);
		//Colour = vec4(subpassLoad(InputColour).rgb * depthColourScaled, 1.0f);
	}
	else
	{
		Colour = subpassLoad(InputColour).rgba;
	}
}
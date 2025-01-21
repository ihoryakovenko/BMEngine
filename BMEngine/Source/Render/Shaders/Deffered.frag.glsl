#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput InputColor; // Color output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput InputDepth;  // Depth output from subpass 1

layout(location = 0) out vec4 Color;

vec3 InverseColor(vec3 Color)
{
	return vec3(1.0 - Color);
}

vec3 Grayscale(vec3 Color)
{
	float Average = 0.2126 * Color.r + 0.7152 * Color.g + 0.0722 * Color.b;
	return vec3(Average, Average, Average);
}

void main()
{
	float Gamma = 2.2;

	int xHalf = 1600;
	//if (gl_FragCoord.x > xHalf)
	if (false)
	{
		float lowerBound = 0.99;
		float upperBound = 1;
		
		vec4 SceneColor = subpassLoad(InputColor).rgba;
		float depth = subpassLoad(InputDepth).r;
		float depthColorScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));
		Color = vec4(0.0f, depthColorScaled, 0.0f, SceneColor.a);
		//Color = vec4(subpassLoad(InputColor).rgb * depthColorScaled, 1.0f);
	}
	else
	{
		vec4 SceneColor = subpassLoad(InputColor).rgba;
		//Color = vec4(Grayscale(SceneColor.rgb), SceneColor.a);
		//Color = vec4(InverseColor(SceneColor.rgb), SceneColor.a);
		Color = SceneColor;
	}

	 Color.rgb = pow(Color.rgb, vec3(1.0 / Gamma));
}
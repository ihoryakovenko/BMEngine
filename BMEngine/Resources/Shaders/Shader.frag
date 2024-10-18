#version 450

layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;
layout(location = 2) in vec3 FragmentNormal;
layout(location = 3) in vec3 FragmentPosition;


layout(set = 1, binding = 0) uniform sampler2D TextureSampler;
layout(set = 2, binding = 0) uniform AmbientLight
{
    vec3 LightColor;
} ambientLight;
layout(set = 3, binding = 0) uniform PointLight
{
    vec3 LightPosition;
} pointLight;

layout(location = 0) out vec4 OutColour;

void main()
{
	vec3 LightDirection = normalize(pointLight.LightPosition - FragmentPosition);
	float DiffuseImpact = max(dot(FragmentNormal, LightDirection), 0.0);
	//vec3 Diffuse = DiffuseImpact * lightColor;
	vec3 DiffuseColor = DiffuseImpact * vec3(1.0, 1.0, 1.0);

	vec3 ResultLightColor = (ambientLight.LightColor + DiffuseColor);

	vec4 LightAmplifiedColor = vec4(FragmentColor, 1.0) * vec4(ResultLightColor, 1.0);
	OutColour = texture(TextureSampler, FragmentTexture) * LightAmplifiedColor;
	//OutColour = vec4(FragmentColor, 1.0);
}

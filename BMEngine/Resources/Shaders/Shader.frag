#version 450

layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;
layout(location = 2) in vec3 FragmentNormal;
layout(location = 3) in vec3 FragmentPosition;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(set = 1, binding = 0) uniform sampler2D TextureSampler;

layout(set = 2, binding = 0) uniform AmbientLight
{
	vec3 LightColor;
}
ambientLight;

layout(set = 2, binding = 1) uniform PointLight
{
	vec3 LightPosition;
	vec3 LightColor;
}
pointLight;

layout(location = 0) out vec4 OutColour;

void main()
{
	vec3 Norm = normalize(FragmentNormal);
	vec3 ViewLightPosition = vec3(ViewProjection.View * vec4(pointLight.LightPosition, 1.0));

	// Ambient color
	float AmbientStrength = 0.1;
	vec3 AmbientColor = ambientLight.LightColor * AmbientStrength;

	// Diffuse color
	vec3 LightDirection = normalize(ViewLightPosition - FragmentPosition);

	float DiffuseImpact = max(dot(Norm, LightDirection), 0.0);
	vec3 DiffuseColor = DiffuseImpact * pointLight.LightColor;

	// Specular color
	float SpecularStrength = 0.5;
	vec3 ViewDirection = normalize(-FragmentPosition);
	vec3 ReflectDirection = reflect(-LightDirection, Norm);

	float SpecularImpact = pow(max(dot(ViewDirection, ReflectDirection), 0.0), 32);
	vec3 SpecularColor = SpecularStrength * SpecularImpact * pointLight.LightColor; 

	vec3 ResultLightColor = AmbientColor + DiffuseColor + SpecularColor;

	vec4 LightAmplifiedColor = vec4(FragmentColor, 1.0) * vec4(ResultLightColor, 1.0);
	OutColour = texture(TextureSampler, FragmentTexture) * LightAmplifiedColor;
}

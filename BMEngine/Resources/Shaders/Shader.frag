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

layout(set = 1, binding = 0) uniform sampler2D DiffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D SpecularTexture;


layout(set = 2, binding = 0) uniform PointLight
{
	vec3 Position;
	vec3 Ambient;
	vec3 Diffuse;
	vec3 Specular;
}
pointLight;

layout(set = 3, binding = 0) uniform Materials
{
	float Shininess;
}
materials;

layout(location = 0) out vec4 OutColour;

void main()
{
	vec3 Norm = normalize(FragmentNormal);
	vec3 ViewLightPosition = vec3(ViewProjection.View * vec4(pointLight.Position, 1.0));
	vec3 DiffuseTexture = vec3(texture(DiffuseTexture, FragmentTexture));
	vec3 SpecularTexture = vec3(texture(SpecularTexture, FragmentTexture));

	// Ambient color
	vec3 AmbientColor = pointLight.Ambient * DiffuseTexture;

	// Diffuse color
	vec3 LightDirection = normalize(ViewLightPosition - FragmentPosition);
	float DiffuseImpact = max(dot(Norm, LightDirection), 0.0);

	vec3 DiffuseColor = pointLight.Diffuse * DiffuseImpact * DiffuseTexture;

	// Specular color
	vec3 ViewDirection = normalize(-FragmentPosition);
	vec3 ReflectDirection = reflect(-LightDirection, Norm);

	float SpecularImpact = pow(max(dot(ViewDirection, ReflectDirection), 0.0), 32);
	vec3 SpecularColor = pointLight.Specular * SpecularImpact * SpecularTexture; 

	vec3 ResultLightColor = AmbientColor + DiffuseColor + SpecularColor;
	OutColour = vec4(ResultLightColor, 1.0);
}

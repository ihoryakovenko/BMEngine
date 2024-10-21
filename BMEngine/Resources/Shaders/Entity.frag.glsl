#version 450

struct PointLight
{
	vec4 Position;
	vec3 Ambient;
	float Constant;
	vec3 Diffuse;
	float Linear;
	vec3 Specular;
	float Quadratic;
};

struct DirectionLight
{
	vec3 Direction;
	vec3 Ambient;
	vec3 Diffuse;
	vec3 Specular;
};

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


layout(set = 2, binding = 0) uniform LightCasters
{
	PointLight pointlight;
	DirectionLight directionLight;
}
lightCasters;

layout(set = 3, binding = 0) uniform Materials
{
	float Shininess;
}
materials;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec4 OutColor;

vec3 CastLight()
{
	vec3 Norm = normalize(mat3(transpose(inverse(ViewProjection.View * Model.Model))) * FragmentNormal);
	vec3 ViewLightPosition = vec3(ViewProjection.View * lightCasters.pointlight.Position);
	vec3 DiffuseTexture = vec3(texture(DiffuseTexture, FragmentTexture));
	vec3 SpecularTexture = vec3(texture(SpecularTexture, FragmentTexture));
	vec3 FragPosition = vec3(ViewProjection.View * Model.Model * vec4(FragmentPosition, 1.0));
	vec3 LightDirection = normalize(ViewLightPosition - FragPosition);

	// Ambient color
	vec3 AmbientColor = lightCasters.pointlight.Ambient * DiffuseTexture;

	// Diffuse color
	float DiffuseImpact = max(dot(Norm, LightDirection), 0.0);
	vec3 DiffuseColor = lightCasters.pointlight.Diffuse * DiffuseImpact * DiffuseTexture;

	// Specular color
	vec3 ViewDirection = normalize(-FragPosition);
	vec3 ReflectDirection = reflect(-LightDirection, Norm);

	float SpecularImpact = pow(max(dot(ViewDirection, ReflectDirection), 0.0), materials.Shininess);
	vec3 SpecularColor = lightCasters.pointlight.Specular * SpecularImpact * SpecularTexture;

	float Distance = length(vec3(ViewLightPosition) - FragPosition);
	float Attenuation = 1.0 / (lightCasters.pointlight.Constant + lightCasters.pointlight.Linear * Distance +
		lightCasters.pointlight.Quadratic * (Distance * Distance)); 

	return AmbientColor * Attenuation + DiffuseColor * Attenuation + SpecularColor * Attenuation;
}

vec3 CastLight2()
{
	vec3 Norm = normalize(mat3(transpose(inverse(ViewProjection.View * Model.Model))) * FragmentNormal);
	vec3 ViewLightPosition = vec3(ViewProjection.View * vec4(-lightCasters.directionLight.Direction, 1.0));
	vec3 DiffuseTexture = vec3(texture(DiffuseTexture, FragmentTexture));
	vec3 SpecularTexture = vec3(texture(SpecularTexture, FragmentTexture));
	vec3 FragPosition = vec3(ViewProjection.View * Model.Model * vec4(FragmentPosition, 1.0));
	vec3 LightDirection = normalize(mat3(ViewProjection.View) * (-lightCasters.directionLight.Direction));

	// Ambient color
	vec3 AmbientColor = lightCasters.directionLight.Ambient * DiffuseTexture;

	// Diffuse color
	float DiffuseImpact = max(dot(Norm, LightDirection), 0.0);
	vec3 DiffuseColor = lightCasters.directionLight.Diffuse * DiffuseImpact * DiffuseTexture;

	// Specular color
	vec3 ViewDirection = normalize(-FragPosition);
	vec3 ReflectDirection = reflect(-LightDirection, Norm);

	float SpecularImpact = pow(max(dot(ViewDirection, ReflectDirection), 0.0), materials.Shininess);
	vec3 SpecularColor = lightCasters.directionLight.Specular * SpecularImpact * SpecularTexture; 

	return AmbientColor + DiffuseColor + SpecularColor;
}

void main()
{
	vec3 ResultLightColor = CastLight();
	//vec3 ResultLightColor = CastLight2();
	OutColor = vec4(ResultLightColor, 1.0);
}

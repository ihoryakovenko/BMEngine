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

struct SpotLight
{
	vec3 Position;
	float CutOff;
	vec3 Direction;
	float OuterCutOff;
	vec3 Ambient;
	float Constant;
	vec3 Diffuse;
	float Linear;
	vec3 Specular;
	float Quadratic;
};

layout(location = 0) in vec3 FragmentColor;
layout(location = 1) in vec2 FragmentTexture;
layout(location = 2) in vec3 FragmentNormal;
layout(location = 3) in vec3 FragmentPosition;
layout(location = 4) in vec4 FragPosLightSpace;

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
	SpotLight spotlight;
}
lightCasters;

layout(set = 3, binding = 0) uniform Materials
{
	float Shininess;
}
materials;

layout(set = 5, binding = 0) uniform sampler2D ShadowMap;

layout(push_constant) uniform PushModel
{
	mat4 Model;
} Model;

layout(location = 0) out vec4 OutColor;

vec3 Diffuse(vec3 LightDirection, vec3 Color, vec3 Texture)
{
	float DiffuseImpact = max(dot(FragmentNormal, LightDirection), 0.0);
	return Color * DiffuseImpact * Texture;
}

vec3 Specular(vec3 LightDirection, vec3 Color, vec3 Texture)
{
	vec3 ViewDirection = normalize(-FragmentPosition);
	vec3 HalfwayDirection = normalize(LightDirection + ViewDirection);

	float SpecularImpact = pow(max(dot(FragmentNormal, HalfwayDirection), 0.0), materials.Shininess);
	return Color * SpecularImpact * Texture; 
}

float LightDistanceAttenuation(vec3 LightPosition, float Constant, float Linear, float Quadratic)
{
	float Distance = length(LightPosition - FragmentPosition);
	return 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance)); 
}

float ShadowCalculation()
{
	vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	float currentDepth = projCoords.z;
	float closestDepth = texture(ShadowMap, projCoords.xy).r;
	float shadow = closestDepth < currentDepth ? 1.0 : 0.0;

	return shadow;
}

vec3 CastDirectionLight(vec3 DiffuseTexture, vec3 SpecularTexture)
{
	vec3 LightDirection = normalize(mat3(ViewProjection.View) * (-lightCasters.directionLight.Direction));

	vec3 AmbientColor = lightCasters.directionLight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(LightDirection, lightCasters.directionLight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(LightDirection, lightCasters.directionLight.Specular, SpecularTexture);

	float Shadow = ShadowCalculation();

	return AmbientColor + (1.0 - Shadow) * (DiffuseColor + SpecularColor);
}

vec3 CastPointLight(vec3 DiffuseTexture, vec3 SpecularTexture)
{
	vec3 LightPosition = vec3(ViewProjection.View * lightCasters.pointlight.Position);
	vec3 LightDirection = normalize(LightPosition - FragmentPosition);

	vec3 AmbientColor = lightCasters.pointlight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(LightDirection, lightCasters.pointlight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(LightDirection, lightCasters.pointlight.Specular, SpecularTexture);

	float Attenuation = LightDistanceAttenuation(LightPosition, lightCasters.pointlight.Constant, 
		lightCasters.pointlight.Linear, lightCasters.pointlight.Quadratic);

	float Shadow = ShadowCalculation();

	return AmbientColor * Attenuation + DiffuseColor * Attenuation + SpecularColor * Attenuation;
}

vec3 CastSpotLigh(vec3 DiffuseTexture, vec3 SpecularTexture)
{
	vec3 ViewLightPosition = vec3(ViewProjection.View * vec4(lightCasters.spotlight.Position, 1.0));
	vec3 ViewLightDirection = normalize(mat3(ViewProjection.View) * (-lightCasters.spotlight.Direction));

	vec3 AmbientColor = lightCasters.spotlight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(ViewLightDirection, lightCasters.spotlight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(ViewLightDirection, lightCasters.spotlight.Specular, SpecularTexture);

	// Soft edges
	vec3 LightDirection = normalize(ViewLightPosition - FragmentPosition);
	float Theta = dot(LightDirection, ViewLightDirection);
	float Intensity = smoothstep(0.0, 1.0, (Theta - lightCasters.spotlight.OuterCutOff) /
		(lightCasters.spotlight.CutOff - lightCasters.spotlight.OuterCutOff));

	float Attenuation = LightDistanceAttenuation(ViewLightPosition, lightCasters.spotlight.Constant, 
	lightCasters.spotlight.Linear, lightCasters.spotlight.Quadratic);

	AmbientColor *= Attenuation * Intensity;
	DiffuseColor *= Attenuation * Intensity;
	SpecularColor *= Attenuation * Intensity;

	return AmbientColor + DiffuseColor + SpecularColor;
}

void main()
{
	vec4 DiffuseTexture = texture(DiffuseTexture, FragmentTexture);
	vec3 SpecularTexture = vec3(texture(SpecularTexture, FragmentTexture));

	vec3 ResultLightColor = vec3(0.0);
	ResultLightColor += CastDirectionLight(vec3(DiffuseTexture), SpecularTexture);
	ResultLightColor += CastPointLight(vec3(DiffuseTexture), SpecularTexture);
	ResultLightColor += CastSpotLigh(vec3(DiffuseTexture), SpecularTexture);
	OutColor = vec4(ResultLightColor, DiffuseTexture.a);
}

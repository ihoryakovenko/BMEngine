#version 450

#extension GL_EXT_nonuniform_qualifier : enable

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
	mat4 LightSpaceMatrix;
	vec3 Direction;
	vec3 Ambient;
	vec3 Diffuse;
	vec3 Specular;
};

struct SpotLight
{
	mat4 LightSpaceMatrix;
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
	vec2 Planes;
};

struct Material {
	uint AlbedoTexIndex;
	uint SpecularTexIndex;
	float Shininess;
};

#define MAX_SHADOW_TEXTURES 2
#define DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX 0
#define SPOT_LIGHT_SHADOW_TEXTURE_INDEX 1

layout(location = 0) in vec2 FragmentTexture;
layout(location = 1) in vec3 FragmentNormal;
layout(location = 2) in vec4 WorldFragPos;
layout(location = 3) in flat uint FragmentMaterialIndex;

layout(push_constant) uniform PushConstants {
	uint FrameIndex;
} Constants;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(set = 1, binding = 0) uniform sampler2D DiffuseTexture[];
layout(set = 1, binding = 1) uniform sampler2D SpecularTexture[];


layout(set = 2, binding = 0) uniform LightCasters
{
	PointLight pointlight;
	DirectionLight directionLight;
	SpotLight spotlight;
}
lightCasters;

layout(std430, set = 3, binding = 0) readonly buffer MaterialsBuffer {
	Material materials[];
} Materials;

layout(set = 4, binding = 0) uniform sampler2DArray ShadowMaps;

layout(location = 0) out vec4 OutColor;

vec3 Diffuse(vec3 LightDirection, vec3 Color, vec3 Texture)
{
	float DiffuseImpact = max(dot(FragmentNormal, LightDirection), 0.0);
	return Color * DiffuseImpact * Texture;
}

vec3 Specular(vec3 FragmentPosition, vec3 LightDirection, vec3 Color, vec3 Texture, float Shininess)
{
	vec3 ViewDirection = normalize(-FragmentPosition);
	vec3 HalfwayDirection = normalize(LightDirection + ViewDirection);

	float SpecularImpact = pow(max(dot(FragmentNormal, HalfwayDirection), 0.0), Shininess);
	return Color * SpecularImpact * Texture; 
}

float LightDistanceAttenuation(vec3 FragmentPosition, vec3 LightPosition, float Constant, float Linear, float Quadratic)
{
	float Distance = length(LightPosition - FragmentPosition);
	return 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance)); 
}

float LinearizeDepth(float depth, vec2 Planes)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * Planes.x * Planes.y) / (Planes.y + Planes.x - z * (Planes.y - Planes.x));
}

float ApplyShadow(mat4 LightSpaceMatrix, float ShadowMapLayer, bool Linearize, vec2 Planes)
{
	vec4 FragPosLightSpace = LightSpaceMatrix * WorldFragPos;
	vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
	if (projCoords.z > 1.0f)
	{
		return 0;
	}

	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	float currentDepth = projCoords.z;
	currentDepth = Linearize ? LinearizeDepth(currentDepth, Planes) : currentDepth;

	float shadow = 0;
	vec2 texelSize = 1.0 / textureSize(ShadowMaps, 0).xy;

	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			vec3 texCoordWithLayer = vec3(projCoords.xy + vec2(x, y) * texelSize, ShadowMapLayer);
			float pcfDepth = texture(ShadowMaps, texCoordWithLayer).r;
			pcfDepth = Linearize ? LinearizeDepth(pcfDepth, Planes) : pcfDepth;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;        
		}    
	}

	return shadow /= 9.0;
}

vec3 CastDirectionLight(mat4 View, DirectionLight directionLight, vec3 FragmentPosition, vec3 DiffuseTexture, vec3 SpecularTexture, float Shininess)
{
	vec3 LightDirection = normalize(mat3(View) * (-directionLight.Direction));

	vec3 AmbientColor = directionLight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(LightDirection, directionLight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(FragmentPosition, LightDirection, directionLight.Specular, SpecularTexture, Shininess);

	float Shadow = ApplyShadow(directionLight.LightSpaceMatrix,
	DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX, false, vec2(1.0));

	return AmbientColor + (1.0 - Shadow) * (DiffuseColor + SpecularColor);
}

vec3 CastPointLight(mat4 View, PointLight pointlight, vec3 FragmentPosition, vec3 DiffuseTexture, vec3 SpecularTexture, float Shininess)
{
	vec3 LightPosition = vec3(View * pointlight.Position);
	vec3 LightDirection = normalize(LightPosition - FragmentPosition);

	vec3 AmbientColor = pointlight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(LightDirection, pointlight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(FragmentPosition, LightDirection, pointlight.Specular, SpecularTexture, Shininess);

	float Attenuation = LightDistanceAttenuation(FragmentPosition, LightPosition, pointlight.Constant, 
		pointlight.Linear, pointlight.Quadratic);

	return AmbientColor * Attenuation + DiffuseColor * Attenuation + SpecularColor * Attenuation;
}

vec3 CastSpotLigh(mat4 View, SpotLight spotlight, vec3 FragmentPosition, vec3 DiffuseTexture, vec3 SpecularTexture, float Shininess)
{
	vec3 ViewLightPosition = vec3(View * vec4(spotlight.Position, 1.0));
	vec3 ViewLightDirection = normalize(mat3(View) * (-spotlight.Direction));

	vec3 AmbientColor = spotlight.Ambient * DiffuseTexture;
	vec3 DiffuseColor = Diffuse(ViewLightDirection, spotlight.Diffuse, DiffuseTexture);
	vec3 SpecularColor = Specular(FragmentPosition, ViewLightDirection, spotlight.Specular, SpecularTexture, Shininess);

	// Soft edges
	vec3 LightDirection = normalize(ViewLightPosition - FragmentPosition);
	float Theta = dot(LightDirection, ViewLightDirection);
	float Intensity = smoothstep(0.0, 1.0, (Theta - spotlight.OuterCutOff) /
		(spotlight.CutOff - spotlight.OuterCutOff));

	float Attenuation = LightDistanceAttenuation(FragmentPosition, ViewLightPosition, spotlight.Constant, 
	spotlight.Linear, spotlight.Quadratic);

	float Shadow = ApplyShadow(spotlight.LightSpaceMatrix,
	SPOT_LIGHT_SHADOW_TEXTURE_INDEX, true, spotlight.Planes);

	AmbientColor *= Attenuation * Intensity;
	DiffuseColor *= Attenuation * Intensity;
	SpecularColor *= Attenuation * Intensity;

	//return AmbientColor + (1.0 - Shadow) * (DiffuseColor + SpecularColor);
	return AmbientColor + (DiffuseColor + SpecularColor);
}

void main()
{
	int idx = int(Constants.FrameIndex);

	vec3 FragmentPosition = vec3(ViewProjection.View * WorldFragPos);

	Material Mat = Materials.materials[FragmentMaterialIndex];
	vec4 DiffuseTexture = texture(DiffuseTexture[nonuniformEXT(Mat.AlbedoTexIndex)], FragmentTexture);
	vec3 SpecularTexture = vec3(texture(SpecularTexture[nonuniformEXT(Mat.SpecularTexIndex)], FragmentTexture));

	vec3 ResultLightColor = vec3(0.0);
	ResultLightColor += CastDirectionLight(ViewProjection.View, lightCasters.directionLight, FragmentPosition, vec3(DiffuseTexture), SpecularTexture, Mat.Shininess);
	ResultLightColor += CastPointLight(ViewProjection.View, lightCasters.pointlight, FragmentPosition, vec3(DiffuseTexture), SpecularTexture, Mat.Shininess);
	ResultLightColor += CastSpotLigh(ViewProjection.View, lightCasters.spotlight, FragmentPosition, vec3(DiffuseTexture), SpecularTexture, Mat.Shininess);
	OutColor = vec4(ResultLightColor, DiffuseTexture.a);
}

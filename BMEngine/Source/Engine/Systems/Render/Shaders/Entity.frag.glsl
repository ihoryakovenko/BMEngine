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
layout(location = 2) in vec3 WorldFragPos;
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


const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a      = roughness * roughness;
	float a2     = a * a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return a2 / max(denom, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float denom = NdotV * (1.0 - k) + k;
	return NdotV / max(denom, 0.000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness)
{
	vec3 H = normalize(V + L);

	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float HdotV = max(dot(H, V), 0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3  F = FresnelSchlick(HdotV, F0);

	vec3 numerator = D * G * F;
	float denominator = 4.0 * NdotV * NdotL + 0.0001;

	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= (1.0 - metallic);

	vec3 diffuse = kD * albedo / PI;
	
	return (diffuse + specular) * NdotL;
}

float LinearizeDepth(float depth, vec2 Planes)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * Planes.x * Planes.y) / (Planes.y + Planes.x - z * (Planes.y - Planes.x));
}

float ApplyShadow(mat4 LightSpaceMatrix, float ShadowMapLayer, bool Linearize, vec2 Planes)
{
	vec4 FragPosLightSpace = LightSpaceMatrix * vec4(WorldFragPos, 1.0);
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

vec3 CastDirectionLight(DirectionLight directionLight, vec3 albedo, vec3 V, vec3 N)
{
	vec3 L = normalize(-directionLight.Direction);
	vec3 radiance = directionLight.Diffuse * 1.0;

	vec3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);
	float Shadow = ApplyShadow(directionLight.LightSpaceMatrix, DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX, false, vec2(1.0));

	return (1.0 - Shadow) * (brdf * radiance);
}

vec3 CastPointLight(PointLight pointlight, vec3 FragmentPosition, vec3 albedo, vec3 V, vec3 N)
{
	vec3 LightPosition = vec3(pointlight.Position);

    vec3 Lvec = LightPosition - FragmentPosition;
    float distance = length(Lvec);
    vec3 L = Lvec / distance;

	float Intensity = 20.0;
    vec3 radiance = pointlight.Diffuse * Intensity / (distance * distance);

    vec3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);
    return brdf * radiance;
}

vec3 CastSpotLigh(SpotLight spotlight,
    vec3 FragmentPosition,
    vec3 albedo,
    vec3 V,
    vec3 N)
{
    vec3 LightPosition = spotlight.Position;
    vec3 LightDir = normalize(-spotlight.Direction);

    vec3 Lvec = LightPosition - FragmentPosition;
    float distance = length(Lvec);
    vec3 L = Lvec / distance;

    float attenuation = 1.0 / (distance * distance);

    float theta = dot(L, LightDir);

    float cone = smoothstep(spotlight.OuterCutOff, spotlight.CutOff, theta);


	float Intensity = 20.0;
    vec3 radiance = spotlight.Diffuse * Intensity * attenuation * cone;

    vec3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);

	float Shadow = ApplyShadow(spotlight.LightSpaceMatrix,
	SPOT_LIGHT_SHADOW_TEXTURE_INDEX, true, spotlight.Planes);

    //return (1.0 - Shadow) * brdf * radiance;
    return brdf * radiance;
}

void main()
{
	Material Mat = Materials.materials[FragmentMaterialIndex];
	vec4 albedo = texture(DiffuseTexture[nonuniformEXT(Mat.AlbedoTexIndex)], FragmentTexture);

	vec3 N = normalize(FragmentNormal);
	vec3 V = normalize(-WorldFragPos);
	
	vec3 Lo = vec3(0.0);
	Lo += CastDirectionLight(lightCasters.directionLight, albedo.rgb, V, N);
	Lo += CastPointLight(lightCasters.pointlight, WorldFragPos, albedo.rgb, V, N);
	Lo += CastSpotLigh(lightCasters.spotlight, WorldFragPos, albedo.rgb, V, N);
	OutColor = vec4(Lo, albedo.a);
}

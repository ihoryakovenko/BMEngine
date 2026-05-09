#include "Common.h"
#include "ShaderTypes.h"

static const int MAX_SHADOW_TEXTURES = 2;
static const int DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX = 0;
static const int SPOT_LIGHT_SHADOW_TEXTURE_INDEX = 1;

struct FragmentInput
{
    [[vk::location(0)]] float2 FragmentTexture;
    [[vk::location(1)]] float3 FragmentNormal;
    [[vk::location(2)]] float3 WorldFragPos;
    [[vk::location(3)]] nointerpolation uint FragmentMaterialIndex;
};

struct FragmentOutput
{
    [[vk::location(0)]] float4 OutColor;
};

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.000001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);

    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CookTorranceBRDF(float3 N,float3 V,float3 L,float3 albedo,float metallic,float roughness)
{
    float3 H = normalize(V + L);

    float NdotL = max(dot(N,L),0.0);
    float NdotV = max(dot(N,V),0.0);
    float HdotV = max(dot(H,V),0.0);

    float3 F0 = lerp(float3(0.04), albedo, metallic);

    float D = DistributionGGX(N,H,roughness);
    float G = GeometrySmith(N,V,L,roughness);
    float3 F = FresnelSchlick(HdotV,F0);

    float3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;

    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - metallic);

    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * NdotL;
}

float LinearizeDepth(float depth, float2 Planes)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * Planes.x * Planes.y) /
           (Planes.y + Planes.x - z * (Planes.y - Planes.x));
}

float ApplyShadow(float4x4 LightSpaceMatrix, float ShadowMapLayer, bool doLinearize, float2 Planes, float3 worldPos)
{
    float4 FragPosLightSpace = mul(LightSpaceMatrix, float4(worldPos,1.0));

    float3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;

    if(projCoords.z > 1.0)
        return 0.0;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if(doLinearize)
        currentDepth = LinearizeDepth(currentDepth, Planes);

    uint width, height, layers;
    ShadowMaps.GetDimensions(width, height, layers);

    float2 texelSize = 1.0 / float2(width, height);

    float shadow = 0.0;

    for(int x=-1; x<=1; x++)
    {
        for(int y=-1; y<=1; y++)
        {
            float3 uvw = float3(
                projCoords.xy + float2(x,y) * texelSize,
                ShadowMapLayer
            );

            float pcfDepth = ShadowMaps.SampleLevel(uvw, 0).r;

            if(doLinearize)
                pcfDepth = LinearizeDepth(pcfDepth, Planes);

            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

float3 CastDirectionLight(DirectionLight light, float3 albedo, float3 V, float3 N, float3 worldPos)
{
    float3 L = normalize(-light.Direction);
    float3 radiance = light.Color;

    float3 brdf = CookTorranceBRDF(N,V,L,albedo,0.5,0.5);

    float shadow = ApplyShadow(light.LightSpaceMatrix, DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX, false, float2(1.0,1.0), worldPos);

    return (1.0 - shadow) * brdf * radiance;
}

float3 CastPointLight(PointLight light, float3 fragPos, float3 albedo, float3 V, float3 N)
{
    float3 lightPos = light.Position.xyz;

    float3 Lvec = lightPos - fragPos;
    float distance = length(Lvec);
    float3 L = Lvec / distance;

    float intensity = 20.0;
    float3 radiance = light.Color * intensity / (distance * distance);

    float3 brdf = CookTorranceBRDF(N,V,L,albedo,0.5,0.5);
    return brdf * radiance;
}

float3 CastSpotLight(SpotLight light, float3 fragPos, float3 albedo, float3 V, float3 N)
{
    float3 lightPos = light.Position;
    float3 lightDir = normalize(-light.Direction);

    float3 Lvec = lightPos - fragPos;
    float distance = length(Lvec);
    float3 L = Lvec / distance;

    float attenuation = 1.0 / (distance * distance);

    float theta = dot(L, lightDir);
    float cone = smoothstep(light.OuterCutOff, light.CutOff, theta);

    float intensity = 20.0;
    float3 radiance = light.Color * intensity * attenuation * cone;
    
    float3 brdf = CookTorranceBRDF(N,V,L,albedo,0.5,0.5);

    return brdf * radiance;
}

[shader("fragment")]
FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;

    Material Mat = MaterialsDescriptor[input.FragmentMaterialIndex];

    float4 albedo = AlbedoTexture[Mat.AlbedoTexIndex].Sample(input.FragmentTexture);

    float3 N = normalize(input.FragmentNormal);
    float3 V = normalize(-input.WorldFragPos);

    float3 Lo = 0.0;

    Lo += CastDirectionLight(FrameBufferDescriptor.directionLight, albedo.rgb, V, N, input.WorldFragPos);
    Lo += CastPointLight(FrameBufferDescriptor.pointlight, input.WorldFragPos, albedo.rgb, V, N);
    Lo += CastSpotLight(FrameBufferDescriptor.spotlight, input.WorldFragPos, albedo.rgb, V, N);

    output.OutColor = float4(Lo, albedo.a);
    return output;
}
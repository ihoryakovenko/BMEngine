#include "Common.slang"
#include "ShaderTypes.h"

static const int DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX = 0;
static const int SPOT_LIGHT_SHADOW_TEXTURE_INDEX = 1;

struct Shader_EntityLayout
{
    float2 FragmentTexture;
    float3 FragmentNormal;
    float3 WorldFragPos;
    nointerpolation uint FragmentMaterialIndex;
};

struct FragmentInput
{
    Shader_EntityLayout layout;
};

struct FragmentOutput
{
    float4 OutColor;
};

struct VertexOutput
{
    Shader_EntityLayout layout;
    float4 Position : SV_Position;
};

[[vk::binding(0, 1)]] Sampler2DArray ShadowMaps;

float3x3 inverse(float3x3 m)
{
    float3 a = m[0];
    float3 b = m[1];
    float3 c = m[2];

    float3 r0 = cross(b, c);
    float3 r1 = cross(c, a);
    float3 r2 = cross(a, b);

    float invDet = 1.0 / dot(r2, c);
    return float3x3(r0 * invDet, r1 * invDet, r2 * invDet);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
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

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness)
{
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float3 F0 = lerp(float3(0.04), albedo, metallic);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(HdotV, F0);

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
    float4 FragPosLightSpace = mul(LightSpaceMatrix, float4(worldPos, 1.0));

    float3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;

    if (projCoords.z > 1.0)
        return 0.0;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (doLinearize)
        currentDepth = LinearizeDepth(currentDepth, Planes);

    uint width, height, layers;
    ShadowMaps.GetDimensions(width, height, layers);

    float2 texelSize = 1.0 / float2(width, height);

    float shadow = 0.0;

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float3 uvw = float3(
                projCoords.xy + float2(x, y) * texelSize,
                ShadowMapLayer
            );

            float pcfDepth = ShadowMaps.SampleLevel(uvw, 0).r;

            if (doLinearize)
                pcfDepth = LinearizeDepth(pcfDepth, Planes);

            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

float3 CastDirectionLight(Shader_DirectionLight light, float3 albedo, float3 V, float3 N, float3 worldPos)
{
    float3 L = normalize(-light.Direction);
    float3 radiance = light.Color;

    float3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);

    float shadow = ApplyShadow(light.LightSpaceMatrix, DIRECTIONAL_LIGHT_SHADOW_TEXTURE_INDEX, false, float2(1.0, 1.0), worldPos);

    return (1.0 - shadow) * brdf * radiance;
}

float3 CastPointLight(Shader_PointLight light, float3 fragPos, float3 albedo, float3 V, float3 N)
{
    float3 lightPos = light.Position.xyz;

    float3 Lvec = lightPos - fragPos;
    float distance = length(Lvec);
    float3 L = Lvec / distance;

    float intensity = 20.0;
    float3 radiance = light.Color * intensity / (distance * distance);

    float3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);
    return brdf * radiance;
}

float3 CastSpotLight(Shader_SpotLight light, float3 fragPos, float3 albedo, float3 V, float3 N)
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

    float3 brdf = CookTorranceBRDF(N, V, L, albedo, 0.5, 0.5);

    return brdf * radiance;
}

[shader("vertex")]
VertexOutput MainVertex(uint vID: SV_VertexID, uint iID: SV_StartInstanceLocation)
{
    Shader_StaticMeshVertex vertex = VertexDescriptor[vID];
    Shader_StaticMeshInstance instance = InstanceDescriptor[iID];

    VertexOutput output;
    output.layout.FragmentTexture = vertex.TextureCoords;

    float4 worldPos = mul(instance.ModelMatrix, float4(vertex.Position, 1.0));
    output.layout.WorldFragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)transpose(inverse((float3x3)instance.ModelMatrix));

    output.layout.FragmentNormal = normalize(mul(normalMatrix, vertex.Normal));
    output.layout.FragmentMaterialIndex = instance.MaterialIndex;
    output.Position = mul(FrameBufferDescriptor.Projection, mul(FrameBufferDescriptor.View, worldPos));

    return output;
}

[shader("fragment")]
FragmentOutput MainFragment(FragmentInput input)
{
    FragmentOutput output;

    Shader_Material Mat = MaterialsDescriptor[input.layout.FragmentMaterialIndex];

    float4 albedo = Textures[Mat.AlbedoTexIndex].Sample(Sampler, input.layout.FragmentTexture);

    float3 N = normalize(input.layout.FragmentNormal);
    float3 V = normalize(-input.layout.WorldFragPos);

    float3 Lo = 0.0;

    Lo += CastDirectionLight(FrameBufferDescriptor.directionLight, albedo.rgb, V, N, input.layout.WorldFragPos);
    Lo += CastPointLight(FrameBufferDescriptor.pointlight, input.layout.WorldFragPos, albedo.rgb, V, N);
    Lo += CastSpotLight(FrameBufferDescriptor.spotlight, input.layout.WorldFragPos, albedo.rgb, V, N);

    output.OutColor = float4(Lo, albedo.a);
    return output;
}

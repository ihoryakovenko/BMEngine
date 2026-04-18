#version 450

const float PI = 3.14159265359;

struct Material
{
    vec3 baseColor;
    float metallic;
    float roughness;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in flat uint materialIndex;
layout(location = 3) in vec3 vWorldPos;

layout(location = 0) out vec4 outColor;


layout(push_constant) uniform PushConstants {
	mat4 vp;
	ivec2 CameraWorldNanoDegPosition;
	float CameraWorldAltitudeMeters;
	float _pad;
	vec2 MetersPerNanoDegLonLat;
	int debugMode;
} pc;

layout(set = 0, binding = 0, std430) readonly buffer Materials
{
    Material materials[];
};

float D_GGX(float NoH, float alpha2)
{
    float denom = NoH * NoH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

float G_SchlickGGX(float NoX, float k)
{
    return NoX / (NoX * (1.0 - k) + k);
}

float G_Smith(float NoV, float NoL, float k)
{
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 Fresnel_Schlick(float VoH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

void main()
{
    Material mat = materials[materialIndex];

    vec3 albedo = mat.baseColor;

    switch (pc.debugMode)
    {
        case 1: outColor = vec4(mat.baseColor, 1); return;
        case 2: outColor = vec4(vec3(mat.metallic), 1); return;
        case 3: outColor = vec4(vec3(mat.roughness), 1); return;
        case 4: outColor = vec4(normalize(fragNormal) * 0.5 + 0.5, 1); return;
        case 5: outColor = vec4(fragColor, 1); return;
    }

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-vWorldPos);

    vec3 L = normalize(vec3(0.5, 1.0, 0.3));
    vec3 H = normalize(V + L);

    vec3 lightColor = vec3(1.0); // bright for testing

    float NoL = max(dot(N, L), 0.0);
    float NoV = max(dot(N, V), 0.001); // NEVER allow 0
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    // Roughness remap
    float roughness = clamp(mat.roughness, 0.04, 1.0);
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    // Fresnel reflectance at normal incidence
    vec3 F0 = mix(vec3(0.04), albedo, mat.metallic);

    // GGX normal distribution
    float D = D_GGX(NoH, alpha2);

    // Smith visibility
    float k = (alpha + 1.0);
    k = (k * k) / 8.0;
    float G = G_Smith(NoV, NoL, k);

    // Fresnel
    vec3 F = Fresnel_Schlick(VoH, F0);

    // Specular BRDF
    vec3 specular = (D * G * F) / max(4.0 * NoV * max(NoL, 0.001), 0.001);

    // Diffuse BRDF
    vec3 kd = (1.0 - F) * (1.0 - mat.metallic);
    vec3 diffuse = kd * albedo / PI;

    vec3 color = (diffuse + specular) * lightColor * NoL;
    
    // Fake ambient
    color += albedo * 0.03;

    outColor = vec4(color, 1.0);
}

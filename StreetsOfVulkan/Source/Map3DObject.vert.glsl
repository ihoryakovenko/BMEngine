#version 450

layout(location = 0) in ivec2 InNanoDegPosition;
layout(location = 1) in float InAltitudeMeters;
layout(location = 2) in vec3 Color;
layout(location = 3) in vec3 Normal;
layout(location = 4) in uint MaterialIndex;

layout(push_constant) uniform PushConstants {
	mat4 vp;
	ivec2 CameraWorldNanoDegPosition;
	float CameraWorldAltitudeMeters;
	float _pad;
	vec2 MetersPerNanoDegLonLat;
	int debugMode;
} pc;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out flat uint materialIndex;
layout(location = 3) out vec3 vWorldPos;

void main()
{
	ivec2 RelativeNanoDegPosition = InNanoDegPosition - pc.CameraWorldNanoDegPosition;

	vec2 RelativeMetersPosition = RelativeNanoDegPosition * pc.MetersPerNanoDegLonLat;
	float RelativeAltitudeMeters = InAltitudeMeters - float(pc.CameraWorldAltitudeMeters);

	vWorldPos = vec3(RelativeMetersPosition.y, RelativeAltitudeMeters, RelativeMetersPosition.x);
	gl_Position = pc.vp * vec4(vWorldPos, 1.0);
	fragColor = Color;
	fragNormal = Normal;
	materialIndex = MaterialIndex;
}

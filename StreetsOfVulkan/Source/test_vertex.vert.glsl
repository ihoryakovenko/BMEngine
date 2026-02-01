#version 450

layout(location = 0) in ivec2 InNanoDegPosition;
layout(location = 1) in float InAltitudeMeters;

layout(push_constant) uniform PushConstants {
	mat4 vp;
	ivec2 CameraWorldNanoDegPosition;
	float CameraWorldAltitudeMeters;
	float _pad;
	vec2 MetersPerNanoDegLonLat;
} pc;

void main()
{
	ivec2 RelativeNanoDegPosition = InNanoDegPosition - pc.CameraWorldNanoDegPosition;

	vec2 RelativeMetersPosition = RelativeNanoDegPosition * pc.MetersPerNanoDegLonLat;
	float RelativeAltitudeMeters = InAltitudeMeters - float(pc.CameraWorldAltitudeMeters);

	gl_Position = pc.vp * vec4(RelativeMetersPosition.y, RelativeAltitudeMeters, RelativeMetersPosition.x, 1.0);
}

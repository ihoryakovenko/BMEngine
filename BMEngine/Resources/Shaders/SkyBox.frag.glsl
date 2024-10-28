#version 450


layout(location = 0) in vec3 FragDirection;

layout(set = 1, binding = 0) uniform samplerCube DiffuseTexture;

layout(location = 0) out vec4 OutColor;

void main()
{
	vec4 DiffuseTexture = texture(DiffuseTexture, FragDirection);
	OutColor = DiffuseTexture;
}

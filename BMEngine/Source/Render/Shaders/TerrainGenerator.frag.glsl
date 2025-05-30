#version 450

#extension GL_EXT_nonuniform_qualifier : enable

struct Material
{
	uint AlbedoTexIndex;
	uint SpecularTexIndex;
	float Shininess;
};

layout(location = 0) in vec2 FragmentUV;

layout(set = 1, binding = 0) uniform sampler2D DiffuseTexture[];
layout(set = 1, binding = 1) uniform sampler2D SpecularTexture[];

layout(std430, set = 2, binding = 0) readonly buffer MaterialsBuffer {
	Material materials[];
} Materials;

layout(push_constant) uniform PushModel
{
	mat4 Model;
	int MaterialIndex;
} Model;

layout(location = 0) out vec4 OutColour;

void main()
{
	Material Mat = Materials.materials[Model.MaterialIndex];
	vec4 DiffuseTexture = texture(DiffuseTexture[nonuniformEXT(Mat.AlbedoTexIndex)], FragmentUV);

	//OutColour = vec4(0.0, 0.5, 0.0, 1.0);
	OutColour = DiffuseTexture;
}
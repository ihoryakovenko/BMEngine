#version 450

#extension GL_EXT_buffer_reference : enable

struct Vertex
{
	vec3 pos;
	vec2 uv;
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer VertexBuffer
{
	Vertex vertices[];
};

layout(push_constant) uniform PushConstants
{
	VertexBuffer vertexBufferAddress;  // u64 device address (8 bytes)
	uint vertexStride;                 // u32 (4 bytes)
	uint padding;                       // padding to match C++ struct
} pc;

layout(location = 0) out vec2 fragUV;

void main()
{
	uint vertexIndex = gl_VertexIndex;
	
	// Read vertex data directly from buffer device address
	// The buffer reference is passed as a device address in the push constant
	vec3 pos = pc.vertexBufferAddress.vertices[vertexIndex].pos;
	vec2 uv = pc.vertexBufferAddress.vertices[vertexIndex].uv;
	
	gl_Position = vec4(pos, 1.0);
	fragUV = uv;
}
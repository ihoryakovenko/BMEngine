#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	vec2 texCoord;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
	VertexBuffer vertexBuffer;
} pc;

void main()
{
	Vertex vertex = pc.vertexBuffer.vertices[gl_VertexIndex];
	fragTexCoord = vertex.texCoord;
	gl_Position = pc.mvp * vec4(vertex.position, 1.0);
}

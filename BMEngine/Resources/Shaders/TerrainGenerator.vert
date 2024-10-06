#version 450

layout(location = 0) in float altitude;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

const int gridSize = 30;

void main()
{
	int row = gl_VertexIndex / gridSize;
	int col = gl_VertexIndex % gridSize;

	float u = float(col) / float(gridSize - 1);
	float v = float(row) / float(gridSize - 1);
	
	vec3 vertexPosition = vec3(u + 10 * u, altitude / 10, v + 10 * v);

	gl_Position = ViewProjection.Projection * ViewProjection.View * vec4(vertexPosition, 1.0);
}
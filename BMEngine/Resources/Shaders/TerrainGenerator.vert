#version 450

layout(location = 0) in float altitude;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

const int gridSize = 600;
const float gridSpacing = 0.5;

void main()
{
	int row = gl_VertexIndex / gridSize;
	int col = gl_VertexIndex % gridSize;

	//float u = float(col) / float(gridSize - 1);
	//float v = float(row) / float(gridSize - 1);

	vec3 vertexPosition = vec3(col * gridSpacing - 100, altitude - 5, row * gridSpacing - 100);

	gl_Position = ViewProjection.Projection * ViewProjection.View * vec4(vertexPosition, 1.0);
}
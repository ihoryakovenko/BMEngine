#version 450

layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform UboViewProjection
{
	mat4 View;
	mat4 Projection;
} ViewProjection;

layout(set = 2, binding = 0) uniform UboTileSettings
{
	int VertexTilesPerAxis;
	int TextureTilesPerAxis;
	int MinTileX;
	int MinTileY;
} TileSettings;

const float radius = 1.0;

void main()
{
	int vertexIndex = int(gl_VertexIndex);
	int i = vertexIndex % (TileSettings.VertexTilesPerAxis + 1);
	int j = vertexIndex / (TileSettings.VertexTilesPerAxis + 1);

	float u = float(i) / float(TileSettings.VertexTilesPerAxis);
	float v = float(j) / float(TileSettings.VertexTilesPerAxis);

	// Mercator to Cartesian projection
	float latitude = atan(sinh(3.14159265359 * (1.0 - 2.0 * v)));
	float longitude = u * 2.0 * 3.14159265359 - 3.14159265359;

	vec3 spherePos;
	spherePos.x = radius * cos(latitude) * sin(longitude);
	spherePos.y = radius * sin(latitude);
	spherePos.z = radius * cos(latitude) * cos(longitude);

	fragUV = vec2(u, v);
	gl_Position = ViewProjection.Projection * ViewProjection.View * vec4(spherePos, 1.0);
}

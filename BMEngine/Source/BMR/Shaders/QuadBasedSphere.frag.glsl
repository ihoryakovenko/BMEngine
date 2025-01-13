#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2DArray TextureSampler;

layout(set = 2, binding = 0) uniform UboTileSettings
{
	int VertexTilesPerAxis;
	int TextureTilesPerAxis;
} TileSettings;

void main()
{
	const float tileSize = 1.0 / float(TileSettings.TextureTilesPerAxis);

	int tileX = int(fragUV.x / tileSize);
	int tileY = int(fragUV.y / tileSize);

	int layer = tileY * TileSettings.TextureTilesPerAxis + tileX;
	vec2 tileUV = mod(fragUV, tileSize) / tileSize;

	outColor = texture(TextureSampler, vec3(tileUV, float(layer)));
}

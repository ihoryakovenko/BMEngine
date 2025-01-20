#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2DArray TextureSampler;

layout(set = 2, binding = 0) uniform UboTileSettings
{
	int VertexTilesPerAxis;
	int TextureTilesPerAxis;
	int MinTileX;
	int MinTileY;
	int TilesCountY;
} TileSettings;

void main()
{
	const float tileSize = 1.0 / float(TileSettings.TextureTilesPerAxis);

	int tileX = int(fragUV.x / tileSize);
	int tileY = int(fragUV.y / tileSize);

	int OffsetX = (tileX - TileSettings.MinTileX + TileSettings.TextureTilesPerAxis) % TileSettings.TextureTilesPerAxis;
	int OffsetY = (tileY - TileSettings.MinTileY + TileSettings.TextureTilesPerAxis) % TileSettings.TextureTilesPerAxis;
	int layer = OffsetX * (TileSettings.TilesCountY + 1) + OffsetY;
	
	vec2 tileUV = mod(fragUV, tileSize) / tileSize;

	outColor = texture(TextureSampler, vec3(tileUV, float(layer)));
	//outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

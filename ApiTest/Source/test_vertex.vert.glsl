#version 450

void main()
{
	uint vertexIndex = gl_VertexIndex;
	
	vec2 positions[6] = vec2[](
		vec2(-1.0, -1.0),
		vec2( 1.0, -1.0),
		vec2(-1.0,  1.0),
		vec2( 1.0, -1.0),
		vec2( 1.0,  1.0),
		vec2(-1.0,  1.0)
	);
	
	vec2 pos = positions[vertexIndex];
	
	gl_Position = vec4(pos, 0.0, 1.0);
}
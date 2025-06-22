#version 450

layout(location = 0) out vec2 vUV;

vec2 Positions[3] = vec2[](
    vec2( 3.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 pos = Positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    vUV = pos * 0.5 + 0.5;
}
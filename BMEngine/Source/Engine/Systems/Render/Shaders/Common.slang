#ifndef COMMON_SLANG
#define COMMON_SLANG

static const float PI = 3.14159265359;

struct UboViewProjection
{
    float4x4 View;
    float4x4 Projection;
};

#endif
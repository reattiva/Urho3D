#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"

void VS(float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

static const float4 colors[] = 
{
    float4(0.0, 0.0, 0.0, 0.0),
    float4(0.8, 0.0, 0.0, 0.0),
    float4(0.0, 0.5, 0.0, 0.0),
    float4(0.0, 0.0, 0.5, 0.0)
};

struct Data
{
    uint color;
};

RWStructuredBuffer<Data> DataBuffer: register(u1);

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    float2 offset = floor(iScreenPos * DIVS);
    int index = (offset.x + offset.y * DIVS);
    
    float3 rgb = Sample2D(DiffMap, iScreenPos).rgb;
    float intensity = GetIntensity(rgb);

    rgb.r += 0.5;
    oColor = float4(rgb, 1.0);
    
    uint value = intensity * 255.0;
    InterlockedMax(DataBuffer[index].color, value);
}

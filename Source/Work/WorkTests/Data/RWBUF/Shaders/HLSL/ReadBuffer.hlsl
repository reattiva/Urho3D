#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

struct Data
{
    uint color;
};

StructuredBuffer<Data> DataBuffer: register(t6);

void VS(float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    float2 offset = floor(iScreenPos * DIVS);
    int index = (offset.x + offset.y * DIVS);

    float intensity = DataBuffer[index].color / 255.0;
    //float intensity = (index % 16.0) / 16.0;
    
    //oColor = Sample2D(DiffMap, iScreenPos);
    oColor = float4(intensity, intensity, intensity, 1.0);
}

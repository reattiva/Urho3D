#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

void VS(float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

#ifdef COMPILEPS
Texture2DArray<float4> tArray : register(t0);
SamplerState sArray : register(s0);
#endif

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : OUTCOLOR0)
{
    float layer = 0.0;
    if (iScreenPos.x > 0.5)
        layer += 1.0;
    if (iScreenPos.y > 0.5)
        layer += 2.0;

    float3 texCoord = float3(iScreenPos.x, iScreenPos.y, layer);
    oColor = tArray.Sample(sArray, texCoord);
}

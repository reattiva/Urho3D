#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"
#include "Fog.hlsl"

cbuffer CustomUB : register(b6)
{
    float4 cSceneColor;
};

void PS(
    float2 iTexCoord : TEXCOORD0,
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,
    float3 iVertexLight : TEXCOORD4,
    float4 iScreenPos : TEXCOORD5,
    out float4 oColor : OUTCOLOR0)
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        float4 diffInput = Sample2D(DiffMap, iTexCoord.xy);
        float4 diffColor = cMatDiffColor * diffInput;
    #else
        float4 diffColor = cMatDiffColor;
    #endif

    // Ambient & per-vertex lighting
    float3 finalColor = iVertexLight * diffColor.rgb  + cSceneColor.rgb;
    oColor = float4(finalColor, diffColor.a);
}

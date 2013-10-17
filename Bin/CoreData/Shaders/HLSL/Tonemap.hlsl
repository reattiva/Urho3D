#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "PostProcess.hlsl"

uniform float cTonemapExposer;
uniform float cTonemapWhiteCutoff;
uniform float2 cReinhardOffsets;
uniform float2 cReinhardInvSize;

void VS(float4 iPos : POSITION,
    out float4 oPos : POSITION,
    out float2 oTexCoord : TEXCOORD0,
    out float2 oScreenPos : TEXCOORD1)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oTexCoord = GetQuadTexCoord(oPos) + cReinhardOffsets;
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iTexCoord : TEXCOORD0,
    float2 iScreenPos : TEXCOORD1,
    out float4 oColor : COLOR0)
{
    #ifdef REINHARD
    oColor = float4(ReinhardTonemap(tex2D(sDiffMap, iScreenPos).rgb * cTonemapExposer), 1.0);
    #endif

    #ifdef FILMIC
    float3 rgb = FilmicTonemap(tex2D(sDiffMap, iScreenPos).rgb * cTonemapExposer) / 
        FilmicTonemap(float3(cTonemapWhiteCutoff, cTonemapWhiteCutoff, cTonemapWhiteCutoff));
    oColor = float4(rgb, 1.0);
    #endif
}

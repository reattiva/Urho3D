#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "PostProcess.hlsl"

uniform float cTonemapExposureBias;
uniform float cTonemapMaxWhite;

void VS(float4 iPos : POSITION,
    out float4 oPos : POSITION,
    out float2 oScreenPos : TEXCOORD0)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iScreenPos : TEXCOORD0,
    out float4 oColor : COLOR0)
{
    #ifdef REINHARDEQ3
    float3 color = ReinhardEq3Tonemap(tex2D(sDiffMap, iScreenPos).rgb * cTonemapExposureBias);
    oColor = float4(color, 1.0);
    #endif

    #ifdef REINHARDEQ4
    float3 color = ReinhardEq4Tonemap(tex2D(sDiffMap, iScreenPos).rgb * cTonemapExposureBias, cTonemapMaxWhite);
    oColor = float4(color, 1.0);
    #endif

    #ifdef UNCHARTED2
    float3 color = Uncharted2Tonemap(tex2D(sDiffMap, iScreenPos).rgb * cTonemapExposureBias) / 
        Uncharted2Tonemap(float3(cTonemapMaxWhite, cTonemapMaxWhite, cTonemapMaxWhite));
    oColor = float4(color, 1.0);
    #endif
}

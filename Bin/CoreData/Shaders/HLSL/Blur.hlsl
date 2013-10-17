#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "PostProcess.hlsl"

uniform float cBlurSigma;
uniform float cBlurScale;
uniform float2 cBlurDir;
uniform float2 cBlurOffsets;
uniform float2 cBlurInvSize;

void VS(float4 iPos : POSITION,
    out float4 oPos : POSITION,
    out float2 oTexCoord : TEXCOORD0,
    out float2 oScreenPos : TEXCOORD1)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oTexCoord = GetQuadTexCoord(oPos) + cBlurOffsets;
    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iTexCoord : TEXCOORD0,
    float2 iScreenPos : TEXCOORD1,
    out float4 oColor : COLOR0)
{
    #ifdef BLUR5
        oColor = GaussianBlur(5, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, iTexCoord);
    #endif

    #ifdef BLUR7
        oColor = GaussianBlur(7, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, iTexCoord);
    #endif

    #ifdef BLUR9
        oColor = GaussianBlur(9, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, iTexCoord);
    #endif

    #ifdef BLUR17
        oColor = GaussianBlur(17, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, iTexCoord);
    #endif
}

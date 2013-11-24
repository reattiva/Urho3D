#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "PostProcess.hlsl"

uniform float2 cAutoExposureLumMinMax;
uniform float cAutoExposureLumAdaptRate;
uniform float cAutoExposureMiddleGrey;
uniform float cAutoExposureMaxWhite;
uniform float2 cHDR128Offsets;
uniform float2 cLum64Offsets;
uniform float2 cLum16Offsets;
uniform float2 cLum4Offsets;
uniform float2 cHDR128InvSize;
uniform float2 cLum64InvSize;
uniform float2 cLum16InvSize;
uniform float2 cLum4InvSize;

float GatherAvgLum(sampler2D texSampler, float2 texCoord, float2 texelSize)
{
    float lumAvg = 0.0;
    lumAvg += tex2D(texSampler, texCoord + float2(0.0, 0.0) * texelSize).r;
    lumAvg += tex2D(texSampler, texCoord + float2(0.0, 2.0) * texelSize).r;
    lumAvg += tex2D(texSampler, texCoord + float2(2.0, 2.0) * texelSize).r;
    lumAvg += tex2D(texSampler, texCoord + float2(2.0, 0.0) * texelSize).r;
    return lumAvg / 4.0;
}

void VS(float4 iPos : POSITION,
    out float4 oPos : POSITION,
    out float2 oTexCoord : TEXCOORD0,
    out float2 oScreenPos : TEXCOORD1)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);

    oTexCoord = GetQuadTexCoord(oPos);

    #ifdef LUMINANCE64
    oTexCoord = GetQuadTexCoord(oPos) + cHDR128Offsets;
    #endif

    #ifdef LUMINANCE16
    oTexCoord = GetQuadTexCoord(oPos) + cLum64Offsets;
    #endif

    #ifdef LUMINANCE4
    oTexCoord = GetQuadTexCoord(oPos) + cLum16Offsets;
    #endif

    #ifdef LUMINANCE1
    oTexCoord = GetQuadTexCoord(oPos) + cLum4Offsets;
    #endif

    oScreenPos = GetScreenPosPreDiv(oPos);
}

void PS(float2 iTexCoord : TEXCOORD0,
    float2 iScreenPos : TEXCOORD1,
    out float4 oColor : COLOR0)
{
    #ifdef LUMINANCE64
    float3 color = float3(0.0, 0.0, 0.0);
    color += tex2D(sDiffMap, iTexCoord + float2(0.0, 0.0) * cHDR128InvSize).rgb;
    color += tex2D(sDiffMap, iTexCoord + float2(0.0, 2.0) * cHDR128InvSize).rgb;
    color += tex2D(sDiffMap, iTexCoord + float2(2.0, 2.0) * cHDR128InvSize).rgb;
    color += tex2D(sDiffMap, iTexCoord + float2(2.0, 0.0) * cHDR128InvSize).rgb;
    color /= 4.0;
    oColor = float4(1e-5 + log(dot(color, LumWeights)), 1.0, 1.0, 1.0);
    #endif

    #ifdef LUMINANCE16
    oColor = float4(GatherAvgLum(sDiffMap, iTexCoord, cLum64InvSize), 1.0, 1.0, 1.0);
    #endif

    #ifdef LUMINANCE4
    oColor = float4(GatherAvgLum(sDiffMap, iTexCoord, cLum16InvSize), 1.0, 1.0, 1.0);
    #endif

    #ifdef LUMINANCE1
    float lum = GatherAvgLum(sDiffMap, iTexCoord, cLum4InvSize);
    oColor = float4(exp(lum), 1.0, 1.0, 1.0);
    #endif

    #ifdef ADAPTLUMINANCE
    float adaptedLum = tex2D(sDiffMap, iTexCoord).r;
    float lum = tex2D(sNormalMap, iTexCoord).r;
    lum = clamp(lum, cAutoExposureLumMinMax.x, cAutoExposureLumMinMax.y);
    oColor = float4(adaptedLum + (lum - adaptedLum) * (1.0 - exp(-cDeltaTimePS * cAutoExposureLumAdaptRate)), 1.0, 1.0, 1.0);
    #endif

    #ifdef APPLYLUMINANCE
    float3 color = tex2D(sDiffMap, iScreenPos).rgb;
    float adaptedLum = tex2D(sNormalMap, iTexCoord).r;
    oColor = float4(AdjustColorLum(color, adaptedLum, cAutoExposureMiddleGrey, cAutoExposureMaxWhite), 1.0);
    #endif
}

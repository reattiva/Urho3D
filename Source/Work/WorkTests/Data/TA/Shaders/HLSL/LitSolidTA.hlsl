#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"
#include "Fog.hlsl"

void VS(float4 iPos : POSITION,
    float3 iNormal : NORMAL,
    float2 iTexCoord : TEXCOORD0,
    #ifdef NORMALMAP
        float4 iTangent : TANGENT,
    #endif
    #ifndef NORMALMAP
        out float2 oTexCoord : TEXCOORD0,
    #else
        out float4 oTexCoord : TEXCOORD0,
        out float4 oTangent : TEXCOORD3,
    #endif
    out float3 oNormal : TEXCOORD1,
    out float4 oWorldPos : TEXCOORD2,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oNormal = GetWorldNormal(modelMatrix);
    oWorldPos = float4(worldPos, GetDepth(oPos));

    #ifdef NORMALMAP
        float3 tangent = GetWorldTangent(modelMatrix);
        float3 bitangent = cross(tangent, oNormal) * iTangent.w;
        oTexCoord = float4(GetTexCoord(iTexCoord), bitangent.xy);
        oTangent = float4(tangent, bitangent.z);
    #else
        oTexCoord = GetTexCoord(iTexCoord);
    #endif

    // Per-pixel forward lighting
    float4 projWorldPos = float4(worldPos.xyz, 1.0);
}

#ifdef COMPILEPS
Texture2DArray<float4> tDiffArray : register(t0);
SamplerState sDiffArray : register(s0);
#endif

void PS(
    #ifndef NORMALMAP
        float2 iTexCoord : TEXCOORD0,
    #else
        float4 iTexCoord : TEXCOORD0,
        float4 iTangent : TEXCOORD3,
    #endif
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,
    out float4 oColor : OUTCOLOR0,
    out float4 oTarget1 : OUTCOLOR1,
    out float4 oTarget2 : OUTCOLOR2
    )
{
    // Get two (of three) texture array layers and mix them over time
    #ifdef DIFFMAP
        float time = cElapsedTimePS * 0.2;
        float layer1 = floor(time) % 3.0;
        float layer2 = (layer1+1) % 3.0;
        float s = time % 1.0;
        s = saturate(2.0*s - 0.5);
            
        float3 texCoord1 = float3(iTexCoord.x, iTexCoord.y, layer1);
        float3 texCoord2 = float3(iTexCoord.x, iTexCoord.y, layer2);

        float4 diffInput1 = tDiffArray.Sample(sDiffArray, texCoord1);
        float4 diffInput2 = tDiffArray.Sample(sDiffArray, texCoord2);        
        float4 diffInput = lerp(diffInput1, diffInput2, s);
    
        float4 diffColor = cMatDiffColor * diffInput;
    #else
        float4 diffColor = cMatDiffColor;
    #endif

    // Get normal
    #ifdef NORMALMAP
        float3x3 tbn = float3x3(iTangent.xyz, float3(iTexCoord.zw, iTangent.w), iNormal);
        float3 normal = normalize(mul(DecodeNormal(Sample2D(NormalMap, iTexCoord.xy)), tbn));
    #else
        float3 normal = normalize(iNormal);
    #endif

    // Get fog factor
    float fogFactor = GetFogFactor(iWorldPos.w);

    // Per-pixel forward lighting
    float3 lightDir;
    float diff = GetDiffuse(normal, iWorldPos.xyz, lightDir);
    float3 lightColor = cLightColor.rgb;
    float3 finalColor = diff * lightColor * diffColor.rgb;

    #ifdef AMBIENT
        finalColor += cAmbientColor * diffColor.rgb;
        finalColor += cMatEmissiveColor;
        oColor = float4(GetFog(finalColor, fogFactor), diffColor.a);
    #else
        oColor = float4(GetLitFog(finalColor, fogFactor), diffColor.a);
    #endif
    oTarget1 = float4(oColor.r, 0.0, 0.0, 1.0);
    oTarget2 = float4(0.0, 0.0, oColor.b, 1.0);
}

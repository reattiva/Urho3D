#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"

cbuffer CustomUB : register(b6)
{
    float4x4 cGridViewProjMatrix0;
    float4x4 cGridViewProjMatrix1;
    float4x4 cGridViewProjMatrix2;
    float2 cGridCellSize;
    float4 cGridSnappedPosition;
    float cFactor;
};

void VS(
    float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float3 oFarRay : TEXCOORD1,
    out float4 oPos : OUTPOSITION
    )
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos); 
    oFarRay = GetFarRay(oPos);
}

#ifdef COMPILEPS

SamplerState sShadowMapN : register(s11);

float4 GetShadowPos(float4 projWorldPos, float depth)
{
    float4 shadowPos;

    if (depth < cShadowSplits.x)
        shadowPos = mul(projWorldPos, cLightMatricesPS[0]);
    else if (depth < cShadowSplits.y)
        shadowPos = mul(projWorldPos, cLightMatricesPS[1]);
    else if (depth < cShadowSplits.z)
        shadowPos = mul(projWorldPos, cLightMatricesPS[2]);
    else
        shadowPos = mul(projWorldPos, cLightMatricesPS[3]);

    return shadowPos;
}

#endif

void PS(
    float2 iScreenPos : TEXCOORD0,
    float3 iFarRay : TEXCOORD1,
    out float4 oColor : OUTCOLOR0
    )
{
// Show shadow map in the top right corner
#if 1
    const float size = 0.3;       
    if (iScreenPos.x > (1-size) && iScreenPos.y < size)
    {
        //float depth = Sample2DLod0(DepthBuffer, iScreenPos).r;
        //float4 radius = GetShadowPos(float4(1.0,1.0,0.0,1.0), depth) * cFactor;
        
        float2 center = float2(1-size/2.0, size/2.0);
        float radius = cShadowMapInvSize * cFactor;
        if (length(iScreenPos-center) < radius)
        //if (abs(iScreenPos.x-center.x) < radius.x && abs(iScreenPos.y-center.y) < radius.y)
        {
            oColor = float4(1.0,0.0,0.0, 1.0);
            return;
        }

        float3 uv = float3((iScreenPos.x - 1 + size) / size, iScreenPos.y / size, 0.98);
        // As comparison with uv.z
        //float inLight = SampleShadow(ShadowMap, uv).r;
        // As color
        float inLight = tShadowMap.Sample(sShadowMapN, uv.xy).r;
        inLight = (inLight - 0.94) / (1.0 - 0.94) * 0.8;
        oColor = float4(inLight, inLight, inLight, 1.0);
    }
    else
#endif
    {
// Show the projected shadow
#if 0
        float depth = Sample2DLod0(DepthBuffer, iScreenPos).r;
        float3 worldPos = iFarRay * depth + cCameraPosPS;
        float4 projWorldPos = float4(worldPos, 1.0);

        float inLight = GetShadowDeferred(projWorldPos, depth);

        float3 color = float3(inLight, inLight, inLight);
#else
        float3 color = Sample2D(DiffMap, iScreenPos).rgb; 
#endif
        oColor = float4(color, 1.0);
    }
}

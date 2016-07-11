
//=========================================
// GridVis: show colors in a texture array
//=========================================

#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"

//----------------------------------------------------
// Vertex shader
//----------------------------------------------------

void VS(
    float4 iPos : POSITION,
    out float2 oScreenPos : TEXCOORD0,
    out float3 oFarRay : TEXCOORD1,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
    oFarRay = GetFarRay(oPos);
}

//----------------------------------------------------
// Pixel shader
//----------------------------------------------------

#ifdef COMPILEPS

#define CELLSINV 1.0/(2.0*HALFCELLS)

Texture2DArray<float4> tArray : register(t1);
SamplerState sArray : register(s1);

cbuffer CustomUB : register(b6)
{
    float4x4 cGridViewProjMatrix0;
    float4x4 cGridViewProjMatrix1;
    float4x4 cGridViewProjMatrix2;
    float2 cGridCellSize;
    float4 cGridSnappedPosition;
    float4 cGlobalIllumParams;
    float cFactor;
};

void PS(
    float2 iScreenPos : TEXCOORD0,
    float3 iFarRay : TEXCOORD1,
    out float4 oColor : OUTCOLOR0)
{
    float depth = Sample2DLod0(DepthBuffer, iScreenPos).r;
    #ifdef HWDEPTH
        depth = ReconstructDepth(depth);
    #endif
    float3 worldPos = iFarRay * depth + cCameraPosPS;

    float3 color = Sample2D(DiffMap, iScreenPos).rgb;

#if 1
    float3 offset = (worldPos - cGridSnappedPosition.xyz) * cGridCellSize.y;
    offset += float3(HALFCELLS, HALFCELLS, HALFCELLS);
    offset = round(offset);

    if (offset.x >= 0.0 && offset.y >= 0.0 && offset.z >= 0.0 && 
        offset.x < HALFCELLS*2.0 && offset.y < HALFCELLS*2.0 && offset.z < HALFCELLS*2.0)
    {
        #ifndef CHANGEPLANE
        float3 texCoord = float3(offset.x * CELLSINV, offset.y * CELLSINV, offset.z);
        #else
        float3 texCoord = float3(offset.x * CELLSINV, offset.z * CELLSINV, offset.y);
        #endif
        oColor = float4(color * 0.0 + tArray.Sample(sArray, texCoord).rgb, 1.0);
    }
    else
        oColor = float4(color, 1.0);
#endif
}

#endif
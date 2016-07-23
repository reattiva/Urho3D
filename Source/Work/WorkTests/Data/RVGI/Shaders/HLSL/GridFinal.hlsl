
//===========================================================
// GridFinal: final SH-lighting
//===========================================================
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html

#include "Uniforms.hlsl"
#include "ScreenPos.hlsl"
//#include "Samplers.hlsl"

//-----------------
//  Vertex Shader
//-----------------

struct VS_Output
{
    float4 position : SV_POSITION;
    float2 texCoords : TEXCOORD0;
    float3 farRay : TEXCOORD1;
};

#ifdef COMPILEVS

// X + + + +
// + + + + +
// + + + + +
// + O + + +
// X + + + X
static const float4 positionTexCoords[3] =
{
    // xy=Pos, zw=UV
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
    float4(3.0f, -1.0f, 2.0f, 1.0f),
    float4(-1.0f, 3.0f, 0.0f, -1.0f)
};

VS_Output VS(uint vertexID: SV_VertexID)
{
    VS_Output output;
    float4 outputPositionTexCoords = positionTexCoords[vertexID];
    output.position = float4(outputPositionTexCoords.xy, 0.0f, 1.0f);
    output.texCoords = outputPositionTexCoords.zw;
    output.farRay = GetFarRay(output.position); //clipPos
    return output;
}

#endif

//-----------------
//  Pixel Shader
//-----------------

#ifdef COMPILEPS

#include "GI.hlsl"
#define PI 3.14159265f

#if defined(TEXTURE)
Texture2D tDiffMap: register(t3);
SamplerState sDiffMap : register(s3);
#endif
Texture2D tNormalMap : register(t4);
SamplerState sNormalMap : register(s4);
Texture2D tDepthBuffer : register(t5);
SamplerState sDepthBuffer : register(s5);

Texture2DArray customMap0: register(t0); // redSHCoeffs
Texture2DArray customMap1: register(t1); // greenSHCoeffs
Texture2DArray customMap2: register(t2); // blueSHCoeffs
//SamplerState customMap0Sampler: register(s0);
SamplerState customMap0Sampler: register(s0)
{
    Filter = None;
};

cbuffer CustomUB : register(b6)
{
    float4x4 cGridViewProjMatrix0;
    float4x4 cGridViewProjMatrix1;
    float4x4 cGridViewProjMatrix2;
    float2 cGridCellSize;
    float4 cGridSnappedPosition;
    float4 cGlobalIllumParams;
    float4 cFactors;
};

// After calculating the texCoords into the 2D texture arrays, the SH-coeffs are trilinearly sampled and
// finally a SH-lighting is done to generate the diffuse global illumination.
float3 GetDiffuseIllum(in float3 offset, in float4 surfaceNormalLobe, in Texture2DArray redSHCoeffsMap,
                       in Texture2DArray greenSHCoeffsMap, in Texture2DArray blueSHCoeffsMap)
{
    // Get texCoords into 2D texture arrays
    float3 texCoords = float3(16.5f, 16.5f, 16.0f) + offset;
    texCoords.xy /= 32.0f;
    if (texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0 || texCoords.z < 0.0 || texCoords.z > 32.0)
        return float3(-1.0, 0.0, 0.0);

    // Since hardware already does the filtering in each 2D texture slice, manually only the filtering into
    // the third dimension has to be done.
    int lowZ = floor(texCoords.z);
    int highZ = min(lowZ+1, 32-1);
    float highZWeight = texCoords.z-lowZ;
    float lowZWeight = 1.0f-highZWeight;
    float3 texCoordsLow = float3(texCoords.x, texCoords.y, lowZ);
    float3 texCoordsHigh = float3(texCoords.x, texCoords.y, highZ);

    // sample red/ green/ blue SH-coeffs trilinearly from the 2D texture arrays
    float4 redSHCoeffs = lowZWeight*redSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*redSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);
    float4 greenSHCoeffs = lowZWeight*greenSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*greenSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);
    float4 blueSHCoeffs = lowZWeight*blueSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*blueSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);

    // Do diffuse SH-lighting by simply calculating the dot-product between the SH-coeffs from the virtual
    // point lights and the surface SH-coeffs.
    float3 vDiffuse;
    vDiffuse.r = dot(redSHCoeffs, surfaceNormalLobe);
    vDiffuse.g = dot(greenSHCoeffs, surfaceNormalLobe);
    vDiffuse.b = dot(blueSHCoeffs, surfaceNormalLobe);

    return vDiffuse;
}

float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float4 PS(VS_Output input) : SV_TARGET
{
    // World space normal
    float4 normalInput = tNormalMap.SampleLevel(sNormalMap, input.texCoords, 0.0);
    float3 normal = normalize(normalInput.rgb * 2.0 - 1.0);      

    // Get surface SH-coeffs
    float4 surfaceNormalLobe = ClampedCosineCoeffs(normal);
    
    // World space position
    float depth = tDepthBuffer.SampleLevel(sDepthBuffer, input.texCoords, 0.0).r;
    float3 position = input.farRay * depth + cCameraPosPS;

    // Get offset -HALF..+HALF into grid = (positionWS - gridCenterWS) / gridCellSize
    float3 offset = (position.xyz - cGridSnappedPosition.xyz) * cGridCellSize.y;

    // Debug
    //return float4(normal, 1.0);
    //return float4(abs(normal), 1.0);
    //return float4(position, 1.0);
    //return float4(offset.x / HALFCELLS, offset.y / HALFCELLS, offset.z / HALFCELLS, 1.0);
    
#if defined(TEXTURE)
    float4 albedo = tDiffMap.SampleLevel(sDiffMap, input.texCoords, 0.0);
#endif

#if 0
    // Get offset 0..2*HALF
    offset += float3(HALFCELLS, HALFCELLS, HALFCELLS);
    offset = round(offset);
    // Get texture position 0..1
    float3 texCoord = float3(offset.x / (2*HALFCELLS), offset.y / (2*HALFCELLS), offset.z);

    float4 redSHCoeffs = customMap0.Sample(customMap0Sampler, texCoord);
    float4 greenSHCoeffs = customMap1.Sample(customMap0Sampler, texCoord);
    float4 blueSHCoeffs = customMap2.Sample(customMap0Sampler, texCoord);

    float3 vDiffuse;
	vDiffuse.r = dot(redSHCoeffs, surfaceNormalLobe);
	vDiffuse.g = dot(greenSHCoeffs, surfaceNormalLobe);
	vDiffuse.b = dot(blueSHCoeffs, surfaceNormalLobe);

    vDiffuse = max(vDiffuse, float3(0.0f, 0.0f, 0.0f)) * 9.0;
    vDiffuse /= PI;
    vDiffuse = pow(vDiffuse, cGlobalIllumParams.z);

    return float4(vDiffuse, 1.0);
#endif

    // Get diffuse global illumination (multiply SH normal with RGB SH saved in the grid)
    float3 diffuseIllum = GetDiffuseIllum(offset, surfaceNormalLobe, customMap0, customMap1, customMap2);

    if (diffuseIllum.r < 0.0)
        return float4(0.0,0.0,0.0, 1.0f);

    // cGlobalIllumParams.z = 0.45 = diffuse GI-contribution power
    diffuseIllum = max(diffuseIllum, float3(0.0f, 0.0f, 0.0f));
    diffuseIllum /= PI;
    diffuseIllum = pow(diffuseIllum, cGlobalIllumParams.z);

#ifdef TEXTURE
    float3 outputColor = diffuseIllum * albedo.rgb;
#else
    float3 outputColor = diffuseIllum;
#endif
    return float4(outputColor, 1.0f);
}

#endif
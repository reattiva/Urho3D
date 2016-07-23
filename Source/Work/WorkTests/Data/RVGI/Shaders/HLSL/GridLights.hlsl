
//===========================================================
// GridLights: turn illuminated voxel into virtual point light
//===========================================================
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html

#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"

//-----------------
//  Vertex Shader
//-----------------

struct VS_Output
{
    float2 screenPos : TEXCOORD0;
    float3 farRay : TEXCOORD1;
    float4 pos : POSITION;
    uint instanceID : INSTANCE_ID;
};

#ifdef COMPILEVS

static const float2 positions[3] =
{
    float2(-1.0f, -1.0f),
    float2(3.0f, -1.0f),
    float2(-1.0f, 3.0f)
};

VS_Output VS(uint vertexID: SV_VertexID, uint instanceID: SV_InstanceID)
{
    VS_Output output;
    output.pos = float4(positions[vertexID], 0.0, 1.0);
    output.screenPos = GetScreenPosPreDiv(output.pos);
    output.farRay = GetFarRay(output.pos);
    output.instanceID = instanceID;
    return output;
}

#endif


//-----------------
// Geometry Shader
//-----------------

struct GS_Output
{
    float2 screenPos : TEXCOORD0;
    float3 farRay : TEXCOORD1;
    float4 pos : SV_POSITION;
    uint rtIndex : SV_RenderTargetArrayIndex;
};

#ifdef COMPILEGS

[maxvertexcount(3)]
void GS(triangle VS_Output input[3], inout TriangleStream<GS_Output> outputStream)
{
    [unroll]
    for(uint i=0; i<3; i++)
    {
        GS_Output output;
        output.screenPos = input[i].screenPos;
        output.farRay = input[i].farRay;
        output.pos = input[i].pos;
        output.rtIndex = input[0].instanceID; // write 3 instances of primitive into 3 slices of 2D texture array
        outputStream.Append(output);
    }

    outputStream.RestartStrip();
}

#endif


//-----------------
//  Pixel Shader
//-----------------

#ifdef COMPILEPS

#include "GI.hlsl"

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

//--------------------
// Shadow 1

#define PCF_NUM_SAMPLES 16

// poisson disk samples
static const float2 filterKernel[PCF_NUM_SAMPLES] =
{
	float2(-0.94201624f, -0.39906216f),
	float2(0.94558609f, -0.76890725f),
	float2(-0.094184101f, -0.92938870f),
	float2(0.34495938f, 0.29387760f),
	float2(-0.91588581f, 0.45771432f),
	float2(-0.81544232f, -0.87912464f),
	float2(-0.38277543f, 0.27676845f),
	float2(0.97484398f, 0.75648379f),
	float2(0.44323325f, -0.97511554f),
	float2(0.53742981f, -0.47373420f),
	float2(-0.26496911f, -0.41893023f),
	float2(0.79197514f, 0.19090188f),
	float2(-0.24188840f, 0.99706507f),
	float2(-0.81409955f, 0.91437590f),
	float2(0.19984126f, 0.78641367f),
	float2(0.14383161f, -0.14100790f)
};

float GetKernelShadow1(float4 projWorldPos, float depth, out float flag)
{
    float4 shadowPos;
    float2 radius = cShadowMapInvSize * 400.0 * cGridCellSize.x * (1.0 + cFactors.x * 0.2);
    float4 limits;

    if (depth < cShadowSplits.x)
    {
        limits = float4(0.0, 0.0, 0.5, 0.5);
        shadowPos = mul(projWorldPos, cLightMatricesPS[0]);
        radius *= cShadowProjs[0].xy;
    }
    else if (depth < cShadowSplits.y)
    {
        limits = float4(0.5, 0.0, 1.0, 0.5);
        shadowPos = mul(projWorldPos, cLightMatricesPS[1]);
        radius *= cShadowProjs[1].xy;
    }
    else if (depth < cShadowSplits.z)
    {
        limits = float4(0.0, 0.5, 0.5, 1.0);
        shadowPos = mul(projWorldPos, cLightMatricesPS[2]);
        radius *= cShadowProjs[2].xy;
    }
    else
    {
        limits = float4(0.5, 0.5, 1.0, 1.0);
        shadowPos = mul(projWorldPos, cLightMatricesPS[3]);
        radius *= cShadowProjs[3].xy;
    }
    #ifdef D3D11
        shadowPos.xyz /= shadowPos.w;
    #endif

    //shadowPos.z -= 0.0015 + cFactors.y * 0.0001;
    shadowPos.z -= 0.0012 + cFactors.y * 0.0001;

    flag = 0.0;
    if (shadowPos.x < limits.x || shadowPos.y < limits.y || shadowPos.x > limits.z || shadowPos.y > limits.w)
    {
        flag = 1.0;
        return 0.0;
    }

    float shadowTerm = 0.0f;
    [unroll]
    for(uint i=0; i<PCF_NUM_SAMPLES; ++i)
    {
        float2 offset = filterKernel[i] * radius;
        float3 samplePos = float3(shadowPos.xy + offset, shadowPos.z);
        shadowTerm += SampleShadow(ShadowMap, samplePos).r;  
    }
    shadowTerm /= PCF_NUM_SAMPLES;
    return shadowTerm;
}

//--------------------
// Shadow 2

#define NUM_SAMPLES 7
static const float3 samples[7] =
{
	float3( 0.0,  0.0,  0.0),
	float3( 1.0,  0.0,  0.0),
	float3(-1.0,  0.0,  0.0),
	float3( 0.0,  1.0,  0.0),
	float3( 0.0, -1.0,  0.0),
	float3( 0.0,  0.0,  1.0),
	float3( 0.0,  0.0, -1.0)
};

float4 GetShadowPos(float4 projWorldPos, float depth)
{
    if (depth < cShadowSplits.x)
        return mul(projWorldPos, cLightMatricesPS[0]);
    else if (depth < cShadowSplits.y)
        return mul(projWorldPos, cLightMatricesPS[1]);
    else if (depth < cShadowSplits.z)
        return mul(projWorldPos, cLightMatricesPS[2]);
    else
        return mul(projWorldPos, cLightMatricesPS[3]);
}

float GetKernelShadow2(float4 projWorldPos, float depth)
{
    float shadowTerm = 0.0;
    [unroll]
    for(uint i = 0; i < NUM_SAMPLES; ++i)
    {
        float4 samplePos = projWorldPos;
        // Ad hoc fix for the house example, reduce radius to get the ceiling black and the outside floor white
        samplePos.xyz += samples[i] * cGridCellSize.x * (0.82 + cFactors.x * 0.1);
        //samplePos.xyz += samples[i] * cGridCellSize.x;
        float4 shadowPos = GetShadowPos(samplePos, depth);
        #ifdef D3D11
            shadowPos.xyz /= shadowPos.w;
        #endif
        //shadowPos.z -= 0.0018 + cFactors.y * 0.0001;
        shadowPos.z -= 0.0010 + cFactors.y * 0.0001; // for the house example
        shadowTerm += round(SampleShadow(ShadowMap, shadowPos).r);  
    }
    shadowTerm /= NUM_SAMPLES;
    return shadowTerm;
}

//--------------------
// PS

#define CELLSINV 1.0/(2.0*HALFCELLS)

StructuredBuffer<Voxel> BufferGrid: register(t6);

struct PS_Output
{
    float4 fragColor0: SV_TARGET0;
    float4 fragColor1: SV_TARGET1;
    float4 fragColor2: SV_TARGET2;
};
    
PS_Output PS(GS_Output input)
{
    PS_Output output;
    
    // Get index of current voxel
    int3 voxelPos = int3(input.pos.xy, input.rtIndex);
    #ifdef CHANGEPLANE
    voxelPos = voxelPos.xzy;
    #endif
    int gridIndex = GetGridIndex(voxelPos);

	// Get voxel data
    Voxel voxel = BufferGrid[gridIndex];

#if 1
    // Early out if voxel has no geometry info
    if ((voxel.colorOcclusionMask & (1<<31u)) == 0)
        discard;
#endif

	// Decode color of voxel
	float3 albedo = DecodeColor(voxel.colorOcclusionMask);

    // Get the normal of the tetrahedron in the voxel which is more parallel to the direction specified,
    // and quantify it by its dot product.
    // Surface on light should have a bigger dot product, si the direction should be -LightDirection.
	float nDotL;
	float3 normal = GetClosestNormal(voxel.normalMasks, cLightDirPS, nDotL);

	// Get world-space position of voxel (WS)
	int3 offset = voxelPos - int3(HALFCELLS, HALFCELLS, HALFCELLS);
	float3 position = (float3(offset.x, offset.y, offset.z) * cGridCellSize.x) + cGridSnappedPosition.xyz;
    float4 projWorldPos = float4(position, 1.0);

    // Get depth (used to select the shadow split)
    float4 viewPos = float4(mul(projWorldPos, cCameraViewPS), 1.0);
    float depth = viewPos.z / cFarClipPS;

    // Get shadow: 0 in shadow, 1 in light; re-using shadowMap from direct illumination
#ifdef SHADOW
    float flag;
#ifndef TOGGLE
    float shadowTerm = GetKernelShadow1(projWorldPos, depth, flag);
#else
    float shadowTerm = GetKernelShadow2(projWorldPos, depth);
#endif
    //float shadowTerm = GetShadowDeferred(projWorldPos, depth);
#else
    float shadowTerm = 1.0;
#endif

#if defined(SPOTLIGHT)
    float4 spotPos = mul(projWorldPos, cLightMatricesPS[0]);
    float3 lightColor = spotPos.w > 0.0 ? Sample2DProj(LightSpotMap, spotPos).rgb * cLightColor.rgb : 0.0;
#else
    float3 lightColor = cLightColor.rgb;
#endif

	// Compute diffuse illumination
	float3 vDiffuse = albedo * lightColor * saturate(nDotL) * shadowTerm;

	// Turn illuminated voxel into virtual point light, represented by second order spherical harmonics coeffs
	float4 coeffs = ClampedCosineCoeffs(normal);
	float3 flux = vDiffuse;
	float4 redSHCoeffs = coeffs * flux.r;
	float4 greenSHCoeffs = coeffs * flux.g;
	float4 blueSHCoeffs = coeffs * flux.b;

    // Debug
	//output.fragColor0 = float4(albedo, 1.0);
    //output.fragColor0 = float4(depth, depth, depth, 1.0);
	//output.fragColor0 = float4(abs(normal), 1.0);
    //output.fragColor0 = float4(nDotL, nDotL, nDotL, 1.0);
	//output.fragColor0 = float4(shadowTerm, shadowTerm, shadowTerm, 1.0);
	//output.fragColor0 = float4(shadowTerm, 0.0, flag, 1.0);
    //output.fragColor0 = float4(vDiffuse, 1.0);
    
	// Output red/green/blue SH-coeffs 
	output.fragColor0 = redSHCoeffs;
	output.fragColor1 = greenSHCoeffs;
	output.fragColor2 = blueSHCoeffs;

	return output;
}

#endif
        



//===========================================================
// GridFill: save voxel color and normals in the grid buffer
//===========================================================
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html

#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"
#include "Fog.hlsl"

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

//----------------------------------------------------
// Vertex shader
//----------------------------------------------------

#ifdef COMPILEVS

struct VS_Output
{
    float2 texCoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

VS_Output VS(
    float4 iPos : POSITION,
    float3 iNormal : NORMAL,
    float2 iTexCoord : TEXCOORD0
#ifdef NORMALMAP
    ,float4 iTangent : TANGENT
#endif
#ifdef INSTANCED
    ,float4x3 iModelInstance : TEXCOORD4
#endif
    )
{
    float4x3 modelMatrix = iModelMatrix;

    VS_Output output;
    output.worldPos = GetWorldPos(modelMatrix); // =mul(iPos, modelMatrix)
    output.normal = GetWorldNormal(modelMatrix); // =normalize(mul(iNormal, (float3x3)modelMatrix))
    output.texCoord = GetTexCoord(iTexCoord);
    return output;
}
#endif

//----------------------------------------------------
// Geometry shader
//----------------------------------------------------

#ifdef COMPILEGS

//#define BLOAT

static const float3x3 viewDirectionMatrix = 
{
    float3(0.0f, 0.0f, -1.0f), // front view
    float3(-1.0f, 0.0f, 0.0f), // left view
    float3(0.0f, -1.0f, 0.0f)  // bottom view
};

float4x4 GetMaxViewMatrix(in float3 normal)
{
    float3 dotProducts = abs(mul(viewDirectionMatrix, normal));
    float maximum = max(max(dotProducts.x, dotProducts.y), dotProducts.z);

    if (maximum == dotProducts.x)
        return cGridViewProjMatrix0;

    if (maximum == dotProducts.y)
        return cGridViewProjMatrix1;

    return cGridViewProjMatrix2;
}

struct GS_Input
{
    float2 texCoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

struct GS_Output
{
    float2 texCoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float4 pos : OUTPOSITION;
};

[maxvertexcount(3)]
void GS(triangle GS_Input input[3], inout TriangleStream<GS_Output> outputStream)
{
    // Triangle face normal
    float3 faceNormal = normalize(input[0].normal + input[1].normal + input[2].normal);

    // Find ViewProj matrix of the camera which has the best view on the triangle
    float4x4 orthoViewProj = GetMaxViewMatrix(faceNormal);

    // Pass through for UV, normal, worldPos from the VS; set vertex clip position
    GS_Output outputs[3];
    [unroll]
    for (uint i=0; i<3; ++i)
    {
        GS_Output output;
        output.texCoord = input[i].texCoord;
        output.normal = input[i].normal;
        output.worldPos = input[i].worldPos;
#if 1
        // Get clip position for the triangle
        output.pos = mul(float4(output.worldPos, 1.0), orthoViewProj);
#else
        // Debug ViewProj matrices (enable CCW culling for this)
        output.pos = mul(float4(output.worldPos, 1.0), cGridViewProjMatrix0);
#endif
        outputs[i] = output;
    }

#ifdef BLOAT
    // Bloat triangle in normalized device space with the texel size of the currently bound
    // render-target. In this way pixels, which would have been discarded due to the low
    // resolution of the currently bound render-target, will still be rasterized.
    float2 side0N = normalize(outputs[1].pos.xy - outputs[0].pos.xy);
    float2 side1N = normalize(outputs[2].pos.xy - outputs[1].pos.xy);
    float2 side2N = normalize(outputs[0].pos.xy - outputs[2].pos.xy);
    const float texelSize = 1.0f/64.0f;
    outputs[0].pos.xy += normalize(side2N-side0N) * texelSize;
    outputs[1].pos.xy += normalize(side0N-side1N) * texelSize;
    outputs[2].pos.xy += normalize(side1N-side2N) * texelSize;
#endif

    
    [unroll]
    for (uint j=0; j<3; ++j)
        outputStream.Append(outputs[j]);

    outputStream.RestartStrip();
}
#endif

//----------------------------------------------------
// Pixel shader
//----------------------------------------------------

#ifdef COMPILEPS

#include "GI.hlsl"

RWStructuredBuffer<Voxel> BufferGrid: register(u4);

struct PS_Input
{
    float2 texCoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    //float4 pos : POSITION;
};

#if TERRAIN
Texture2D tWeightMap0 : register(t0);
Texture2D tDetailMap1 : register(t1);
Texture2D tDetailMap2 : register(t2);
Texture2D tDetailMap3 : register(t3);
SamplerState sWeightMap0 : register(s0);
SamplerState sDetailMap1 : register(s1);
SamplerState sDetailMap2 : register(s2);
SamplerState sDetailMap3 : register(s3);
#endif

#if 0

float4 PS(PS_Input input) : OUTCOLOR0
{
#if TERRAIN
    float3 weights = Sample2D(WeightMap0, input.texCoord.xy).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    float4 diffColor = cMatDiffColor * (
        weights.r * Sample2D(DetailMap1, input.texCoord.xy) +
        weights.g * Sample2D(DetailMap2, input.texCoord.xy) +
        weights.b * Sample2D(DetailMap3, input.texCoord.xy)
    );
#else
    float4 diffColor = Sample2D(DiffMap, input.texCoord.xy);
#endif
    #if 0
        if (length(input.worldPos - cGridSnappedPosition.xyz) < 1.0)
            diffColor = float4(0.0, 0.0, 1.0, 1.0);
    #elif 0
        diffColor.r += 0.5;
    #endif
    return diffColor;
}

#else

// Normalized directions of 4 faces of a regular tetrahedron
// down-forward, down-backward, left-up, right-up
static const float3 faceVectors[4] =
{
	float3(0.0f, -0.57735026f, 0.81649661f),
	float3(0.0f, -0.57735026f, -0.81649661f),
	float3(-0.81649661f, 0.57735026f, 0.0f),
	float3(0.81649661f, 0.57735026f, 0.0f)
};

// Get the face (as an index 0..3) of the tetrahedron above which is more parallel to the normal,
// get also the dot product to quantify this parallelism so we can select the max
uint GetNormalIndex(in float3 normal, out float dotProduct)
{
	float4x3 faceMatrix;
	faceMatrix[0] = faceVectors[0];
	faceMatrix[1] = faceVectors[1];
	faceMatrix[2] = faceVectors[2];
	faceMatrix[3] = faceVectors[3];
	float4 dotProducts = mul(faceMatrix, normal);
	float maximum = max(max(dotProducts.x, dotProducts.y), max(dotProducts.z, dotProducts.w));
	uint index;
	if(maximum == dotProducts.x)
		index = 0;
	else if(maximum == dotProducts.y)
		index = 1;
	else if(maximum == dotProducts.z)
		index = 2;
	else
		index = 3;

	dotProduct = dotProducts[index];
	return index;
}

void PS(PS_Input input)
{
    // --- Voxel Color ---

    // Get surface color
#if defined(TERRAIN)
    float3 weights = Sample2D(WeightMap0, input.texCoord.xy).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    float4 diffColor = cMatDiffColor * (
        weights.r * Sample2D(DetailMap1, input.texCoord.xy) +
        weights.g * Sample2D(DetailMap2, input.texCoord.xy) +
        weights.b * Sample2D(DetailMap3, input.texCoord.xy)
    );
#elif defined(DIFFMAP)
    float4 diffColor = cMatDiffColor * Sample2D(DiffMap, input.texCoord.xy);
#else
    float4 diffColor = cMatDiffColor;
#endif
    float3 base = diffColor.rgb;

    // From sRGB to linear RGB
    //float3 baseLinear = lerp(base/12.92f, pow((base+0.055f)/1.055f, 2.4f), base>0.04045f);
    float3 baseLinear = base * (base * (base * 0.305306011 + 0.682171111) + 0.012522878);
    //float3 baseLinear = base;

    // Encode color from float3 to uint 00FFFFFF, 24 bits R8G8B8 (note: all floats must be <= 1.0)
    uint colorOcclusionMask = EncodeColor(baseLinear);

    // Since voxels are a simplified representation of the actual scene, high frequency information
    // gets lost. In order to amplify color bleeding in the final global illumination output, colors
    // with high difference in their color channels (called here contrast) are preferred. By writing
    // the contrast value (0-255) in the highest 8 bit of the color-mask, automatically colors with
    // high contrast will dominate, since we write the results with an InterlockedMax into the voxel-
    // grids. The contrast value is calculated in sRGB space.
    float contrast = length(base.rrg - base.gbb) / (sqrt(2.0f) + base.r + base.g + base.b);

    // Encode contrast (must be <= 1) to 7F000000, 7 bits
    uint iContrast = uint(contrast * 127.0);
    colorOcclusionMask |= iContrast << 24u;

    // Encode occlusion into highest bit 80000000, 1 bit. This is used to mark voxel with objects inside.
    colorOcclusionMask |= 1 << 31u;

    // --- Voxel Normal ---

    // Encode normal from float3 to uint 07FFFFFF, 27 bits 9X9Y9Z
    float3 normal = normalize(input.normal);
	uint normalMask = EncodeNormal(normal);

    // The voxel has 4 normals, to select which normal to write to, get the index of the normal of the tetrahedron 
    // (Back, Front, Up, Right) more parallel to our normal, and their dot product
    float dotProduct;
	uint normalIndex = GetNormalIndex(normal, dotProduct);

    // Encode the dot product in F8000000, 5 bits
	uint iDotProduct = uint(saturate(dotProduct) * 31.0f);
	normalMask |= iDotProduct<<27u;
    
    // --- Buffer atomic write ---

    // Distance in voxel units of the pixel from the grid center
    float3 offset = (input.worldPos - cGridSnappedPosition.xyz) * cGridCellSize.y;
    // Distance from corner, we have low precision, so we round the offset to have a stable integer
    offset += float3(HALFCELLS, HALFCELLS, HALFCELLS);
    offset = round(offset);
    // Get voxel position
    int3 voxelPos = int3(offset.x, offset.y, offset.z);

    // Only output voxels that are inside the boundaries of the grid.
    if (voxelPos.x >= 0 && voxelPos.y >= 0 && voxelPos.z >= 0 && 
        voxelPos.x < HALFCELLS*2 && voxelPos.y < HALFCELLS*2 && voxelPos.z < HALFCELLS*2)
    {
        // Get index into the voxel-grid
        int gridIndex = GetGridIndex(voxelPos);

        // To avoid raise conditions between multiple threads, that write into the same location, 
        // atomic functions have to be used.

        // Output occlusion flag + contrast + color
        InterlockedMax(BufferGrid[gridIndex].colorOcclusionMask, colorOcclusionMask);

        // Output dot product + normal, according to normal index
        InterlockedMax(BufferGrid[gridIndex].normalMasks[normalIndex], normalMask);
    }

}
#endif
#endif

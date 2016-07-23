
//=========================================
// GridVis: show colors in the grid buffer
//=========================================

#include "Uniforms.hlsl"
#include "Transform.hlsl"
#include "Samplers.hlsl"
#include "ScreenPos.hlsl"

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

StructuredBuffer<Voxel> BufferGrid: register(t6);

#endif

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

#if 0
    // f=-1.999..-1.000 -> i=-1
    // f=-0.999..+0.999 -> i=0
    // f=+1.000..+1.999 -> i=1
    float f = -1.999;
    int i = int(f);
    if (iScreenPos.x < 0.1 && i == -1)
    {
        oColor = float4(1.0,0.0,0.0, 1.0);
        return;
    }
#endif

#if 0
    // Display all the colors in the grid buffer. Colors are shown in slices, the top-left slice
    // is the X-Y slice farthest from the camera, the bottom-right is the closest.
    const float CELLS = HALFCELLS * 2.0;
    const float COLUMNS = 8.0;
    const float ROWS = CELLS / COLUMNS;
    const float SIZE = 1 / (COLUMNS * CELLS);
    const float SIZEINV = (COLUMNS * CELLS);
    const float STARTY = 1.0 - ROWS / COLUMNS; // 1 - ROWS * CELLS * SIZE

    float layerX;
    float cellX = modf(iScreenPos.x * COLUMNS, layerX);
    float layerY;
    float cellY = modf((iScreenPos.y - STARTY) * COLUMNS, layerY); // COLUMNS = ROWS / (1 - STARTY)

    float layer = CELLS - 1.0 - (layerX + layerY * COLUMNS);
    int3 voxelPos = int3(cellX * CELLS, (1.0-cellY) * CELLS, layer);

    if (voxelPos.x >= 0 && voxelPos.y >= 0 && voxelPos.z >= 0 && 
        voxelPos.x < HALFCELLS*2 && voxelPos.y < HALFCELLS*2 && voxelPos.z < HALFCELLS*2)
    {
        int gridIndex = GetGridIndex(voxelPos);

        Voxel voxel = BufferGrid[gridIndex];
        oColor = float4(DecodeColor(voxel.colorOcclusionMask), 1.0f);
        return;
    }
    else
        oColor = float4(color, 1.0);
#endif

#if 1
    // Project the colors in the grid buffers to the scene
    float3 offset = (worldPos - cGridSnappedPosition.xyz) * cGridCellSize.y;
    offset += float3(HALFCELLS, HALFCELLS, HALFCELLS);
    offset = round(offset);
    int3 voxelPos2 = int3(offset.x, offset.y, offset.z);

    if (voxelPos2.x >= 0 && voxelPos2.y >= 0 && voxelPos2.z >= 0 && 
        voxelPos2.x < HALFCELLS*2 && voxelPos2.y < HALFCELLS*2 && voxelPos2.z < HALFCELLS*2)
    {
        int gridIndex = GetGridIndex(voxelPos2);

        Voxel voxel = BufferGrid[gridIndex];
        color = DecodeColor(voxel.colorOcclusionMask);
        //color = DecodeNormal(voxel.normalMasks[3]);
        //float dotProduct;
        //color = abs(GetClosestNormal(voxel.normalMasks, float3(1.0,0.0,0.0), dotProduct));
    }

    oColor = float4(color, 1.0);
#endif
}

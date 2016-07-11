
//===========================================================
// GridClear: clear the grid buffer
//===========================================================

#include "GI.hlsl"

RWStructuredBuffer<Voxel> BufferGrid: register(u4);

[numthreads(4, 4, 4)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	int3 voxelPos = dispatchThreadID.xyz;
	int gridIndex = GetGridIndex(voxelPos);

	Voxel voxel;
	voxel.colorOcclusionMask = 0;
	voxel.normalMasks = uint4(0, 0, 0, 0);

	BufferGrid[gridIndex] = voxel;
}

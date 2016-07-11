
//===========================================================
// GridLightPropagate: propagate light to adjacent cells
//===========================================================
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html

#include "GI.hlsl"

Texture2DArray<float4> customMap0: register(t0);  // redSHCoeffs
Texture2DArray<float4> customMap1: register(t1);  // greenSHCoeffs
Texture2DArray<float4> customMap2: register(t2);  // blueSHCoeffs

#ifdef USE_OCCLUSION
StructuredBuffer<Voxel> BufferGrid: register(t6);
#endif

// Dover specificare l'esatto nome qui, uguale a quello dell'array definito nel
// render path è un problema: non possiamo swappare gli array.
#ifdef OUTARRAYS02
RWTexture2DArray<float4> array0: register(u0);  // redSHCoeffs
RWTexture2DArray<float4> array1: register(u1);  // greenSHCoeffs
RWTexture2DArray<float4> array2: register(u2);  // blueSHCoeffs
#endif
#ifdef OUTARRAYS35
RWTexture2DArray<float4> array3: register(u0);  // redSHCoeffs
RWTexture2DArray<float4> array4: register(u1);  // greenSHCoeffs
RWTexture2DArray<float4> array5: register(u2);  // blueSHCoeffs
#endif

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

// Solid angles measures the area of the slice of a unit sphere subtended by a face of a 
// cell, the sphere is centered on the adjacent cell.
// We use them to quantify how much light a face of a cell receive from the adjacent cell,
// according to the size and position of the face the light received is different.
// A cell has 6 faces, with respect to the adjacent cell we have:
// - the near face, it is oriented in the opposite direction of the light emitted by the
//   adjacent cell, so no light is received and we set the solid angle to 0;
// - the far face, it is distant 1.5 from the center of the adjacent cell, the area of a
//   unit sphere cutted by this face is SOLID_ANGLE_FAR;
// - 4 side faces, the area of a unit sphere cutted by each of these faces is SOLID_ANGLE_SIDE,
//   it don't seems but this area is bigger that the one of the far face.
// The solid angles are calculated as: SA=Area/R^2. We normalize them so the solid angle of 
// a sphere is 1: NSA=Area/(4*pi*R^2).

// How to calculate these angles:
// - we use this parameterization of a sphere of radius R:
//    r(u,v) = < R*sin(u), R*cos(u)*sin(v), R*cos(u)*cos(v) >
// - the surface area is:
//    S = DoubleIntegral(|r_u x r_v| dA) = DoubleIntegral(R^2*cos(u) dv du)
//   better use a unit sphere so S = solid angle:
//    S = DoubleIntegral(cos(u) dv du)
// - for a quarter of the far face, v goes from 0 to atan(1/3) and u from 0 to atan(cos(v)/3),
//   the surface is:
//    Sfar = 4 * DoubleIntegral(v=0..atan(1/3), u=0..atan(cos(v)/3), cos(u) du dv) =
//         = 4 * Integral(v=0..atan(1/3), sin(atan(cos(v)/3)) dv) = 0.400669684646239
// - normalized:
//    SOLID_ANGLE_FAR = 0.400669684646239/(4*pi) = 0.0318842804292599
//    Octave: quad(@(x) 4*sin(atan(cos(x)/3)), 0, atan(1/3)) / (4*pi)
// - for half of a side face, v goes from atan(1/3) to pi/4 and u from 0 to atan(sin(v)),
//   the surface is:
//    Sside = 2 * DoubleIntegral(v=atan(1/3)..pi/4, u=0..atan(sin(v)), cos(u) du dv) =
//          = 2 * Integral(v=atan(1/3)..pi/4, sin(atan(sin(v))) dv) = 0.423431354436739
//   note: Sside = (Snear - Sfar)/4
// - normalized:
//    SOLID_ANGLE_SIDE = 0.423431354436739/(4*pi) = 0.0336955965593517
//    Octave: quad(@(x) 2*sin(atan(sin(x))), atan(1/3), pi/4) / (4*pi)

// Normalized solid angles of the faces of a cell with respect to the adjacent cell center.
#define SOLID_ANGLE_FAR  0.0318842778f
#define SOLID_ANGLE_SIDE 0.0336955972f

// directions to 6 neighbor cell centers
static float3 directions[6] =
{
    float3(0.0f, 0.0f, 1.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f)
};

// SH-coeffs for six faces
static float4 faceCoeffs[6] =
{
    float4(0.8862269521f, 0.0f, 1.0233267546f, 0.0f),  // ClampedCosineCoeffs(directions[0])
    float4(0.8862269521f, 0.0f, 0.0f, -1.0233267546f), // ClampedCosineCoeffs(directions[1])
    float4(0.8862269521f, 0.0f, -1.0233267546f, 0.0f), // ClampedCosineCoeffs(directions[2])
    float4(0.8862269521f, 0.0f, 0.0f, 1.0233267546f),  // ClampedCosineCoeffs(directions[3])
    float4(0.8862269521f, -1.0233267546f, 0.0f, 0.0f), // ClampedCosineCoeffs(directions[4])
    float4(0.8862269521f, 1.0233267546, 0.0f, 0.0f)    // ClampedCosineCoeffs(directions[5])
};

[numthreads(4, 4, 4)]
void CS(uint3 id : SV_DispatchThreadID)
{
    // Grid-position of current cell
    int3 elementPos = int3(id.xyz);

    // Initialize SH-coeffs with current cell own contribution
    float4 sumRedSHCoeffs = customMap0.Load(int4(elementPos, 0));
    float4 sumGreenSHCoeffs = customMap1.Load(int4(elementPos, 0));
    float4 sumBlueSHCoeffs = customMap2.Load(int4(elementPos, 0));

    // Add contribution from the 6 adjacent cells
    [unroll]
    for (uint i=0; i<6; i++)
    {
        // Grid-position of neighbor cell
        int3 samplePos = elementPos + int3(directions[i]);

        // Skip if cell out of bounds
        if ((samplePos.x < 0) || (samplePos.x >= 32) || (samplePos.y < 0) || (samplePos.y >= 32) || (samplePos.z < 0) || (samplePos.z >= 32))
            continue;

        // Load SH-coeffs for neighbor cell
        const float4 redSHCoeffs = customMap0.Load(int4(samplePos, 0)); 
        const float4 greenSHCoeffs = customMap1.Load(int4(samplePos, 0));
        const float4 blueSHCoeffs = customMap2.Load(int4(samplePos, 0));

#ifdef USE_OCCLUSION
        // Get voxel of neighbor cell
        int gridIndex = GetGridIndex(samplePos);
        Voxel voxel = BufferGrid[gridIndex];

        // If voxel contains geometry info, find closest normal to current direction. In this way the highest
        // occlusion can be generated. Then get SH-coeffs for retrieved normal.
        float4 occlusionCoeffs = float4(0.0f, 0.0f, 0.0f, 0.0f);
        if ((voxel.colorOcclusionMask & (1<<31u)) != 0)
        {
            float dotProduct;
            float3 occlusionNormal = GetClosestNormal(voxel.normalMasks, -directions[i], dotProduct);
            occlusionCoeffs = ClampedCosineCoeffs(occlusionNormal);
        }
#endif

        // x = flux amplifier, y = occlusion amplifier, z = diffuse GI-contribution power
        //const float4 cGlobalIllumParams = float4(2.55, 1.25, 0.45, 0.0);

        // For each face of the current cell
        // Evaluate the SH approximation of the intensity coming from the neighboring cell to this face
        [unroll]
        for (uint j=0; j<6; j++)
        {
            // Get current cell face position
            float3 facePosition = directions[j] * 0.5f;

            // Get direction from current neighbor cell center to face of current cell
            // Possible distances: 0.5(near face), 1.5(far face), 1.118 (side faces)
            float3 neighborCellCenter = directions[i];
            float3 dir = facePosition - neighborCellCenter;
            float fLength = length(dir);
            dir /= fLength;

            // Get corresponding solid angle
            float solidAngle = 0.0f;
            if (fLength > 0.5f)
                solidAngle = (fLength>=1.5f) ? SOLID_ANGLE_FAR : SOLID_ANGLE_SIDE;

            // Get SH-coeffs for direction pointing to the face
            float4 dirSH = SH(dir);

            // Calculate flux from neigbor cell to face of current cell
            float3 flux;
            flux.r = dot(redSHCoeffs,   dirSH);
            flux.g = dot(greenSHCoeffs, dirSH);
            flux.b = dot(blueSHCoeffs,  dirSH);
            flux = max(0.0f, flux) * solidAngle * cGlobalIllumParams.x;

#ifdef USE_OCCLUSION
            // apply occlusion
            float occlusion = 1.0f - saturate(cGlobalIllumParams.y * dot(occlusionCoeffs, dirSH));
            flux *= occlusion;
#endif

            // add contribution to SH-coeffs sums
            float4 coeffs = faceCoeffs[j];
            sumRedSHCoeffs += coeffs * flux.r;
            sumGreenSHCoeffs += coeffs * flux.g;
            sumBlueSHCoeffs += coeffs * flux.b;
        }
    }

    // write out generated red/ green/ blue SH-coeffs
#ifdef OUTARRAYS02
    array0[elementPos] = sumRedSHCoeffs;
    array1[elementPos] = sumGreenSHCoeffs;
    array2[elementPos] = sumBlueSHCoeffs;
#endif
#ifdef OUTARRAYS35
    array3[elementPos] = sumRedSHCoeffs;
    array4[elementPos] = sumGreenSHCoeffs;
    array5[elementPos] = sumBlueSHCoeffs;
#endif
}

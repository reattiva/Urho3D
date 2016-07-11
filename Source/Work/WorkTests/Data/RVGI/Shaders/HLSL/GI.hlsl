
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html

struct Voxel
{
    // encoded color + occlusion
    uint colorOcclusionMask;
    // encoded normals
    uint4 normalMasks;
};

// Get index into a CELLSxCELLSxCELLS grid for the specified position
int GetGridIndex(in int3 position)
{
    return ((position.y * HALFCELLS*HALFCELLS*4) + (position.z * HALFCELLS*2) + position.x);
}

// Encode specified color (range 0.0f-1.0f), so that each channel is
// stored in 8 bits of an unsigned integer.
uint EncodeColor(in float3 color)
{
    uint3 iColor = uint3(color*255.0f);
    uint colorMask = (iColor.r<<16u) | (iColor.g<<8u) | iColor.b;
    return colorMask;
}

// Decode specified mask into a float3 color (range 0.0f-1.0f).
float3 DecodeColor(in uint colorMask)
{
    float3 color;
    color.r = (colorMask>>16u) & 0x000000ff;
    color.g = (colorMask>>8u) & 0x000000ff;
    color.b = colorMask & 0x000000ff;
    color /= 255.0f;
    return color;
}

#if 1
// Encode specified normal (normalized) into an unsigned integer. Each axis of
// the normal is encoded into 9 bits (1 for the sign/ 8 for the value).
uint EncodeNormal(in float3 normal)
{
    int3 iNormal = int3(normal*255.0f);
    uint3 iNormalSigns;
    iNormalSigns.x = (iNormal.x>>5) & 0x04000000;
    iNormalSigns.y = (iNormal.y>>14) & 0x00020000;
    iNormalSigns.z = (iNormal.z>>23) & 0x00000100;
    iNormal = abs(iNormal);
    uint normalMask = iNormalSigns.x | (iNormal.x<<18) | iNormalSigns.y | (iNormal.y<<9) | iNormalSigns.z | iNormal.z;
    return normalMask;
}

// Decode specified mask into a float3 normal (normalized).
float3 DecodeNormal(in uint normalMask)
{
    int3 iNormal;
    iNormal.x = (normalMask>>18) & 0x000000ff;
    iNormal.y = (normalMask>>9) & 0x000000ff;
    iNormal.z = normalMask & 0x000000ff;
    int3 iNormalSigns;
    iNormalSigns.x = (normalMask>>25) & 0x00000002;
    iNormalSigns.y = (normalMask>>16) & 0x00000002;
    iNormalSigns.z = (normalMask>>7) & 0x00000002;
    iNormalSigns = 1-iNormalSigns;
    float3 normal = float3(iNormal)/255.0f;
    normal *= iNormalSigns;
    return normal;
}

#else

uint EncodeNormal(in float3 normal)
{
    int3 iNormal = int3((normal+1.0) * 255.5);
    uint normalMask = (iNormal.x<<18) | (iNormal.y<<9) | iNormal.z;
    return normalMask;
}

float3 DecodeNormal(in uint normalMask)
{
    int3 iNormal;
    iNormal.x = (normalMask>>18) & 0x1ff;
    iNormal.y = (normalMask>>9) & 0x1ff;
    iNormal.z = normalMask & 0x1ff;
    float3 normal = float3(iNormal) / 255.5 - 1.0;
    return normal;
}

#endif

// Determine which of the 4 specified normals (encoded as normalMasks) is closest to
// the specified direction. The function returns the closest normal and as output
// parameter the corresponding dot-product.
float3 GetClosestNormal(in uint4 normalMasks, in float3 direction, out float dotProduct)
{
    float4x3 normalMatrix;
    normalMatrix[0] = DecodeNormal(normalMasks.x); // da uint32 a float3
    normalMatrix[1] = DecodeNormal(normalMasks.y);
    normalMatrix[2] = DecodeNormal(normalMasks.z);
    normalMatrix[3] = DecodeNormal(normalMasks.w);
    // Use matrix * vactor to get 4 dot products
    float4 dotProducts = mul(normalMatrix, direction);

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
    return normalMatrix[index];
}

// First two bands of SH for specified direction
float4 SH(in float3 dir)
{
	float4 result;
	result.x =  0.2820947918f;          //  1/(2*sqrt(PI)) [first band]
	result.y = -0.4886025119f * dir.y;  // -sqrt(3/(4*PI)) * y [second band]
	result.z =  0.4886025119f * dir.z;  //  sqrt(3/(4*PI)) * z
	result.w = -0.4886025119f * dir.x;  // -sqrt(3/(4*PI)) * x
	return result;
}

// A clamped cosine lobe function oriented in Z direction is used and expressed
// as spherical harmonics. Since the function has rotational symmetry around the
// Z axis, the SH projection results in zonal harmonics. The rotation of zonal
// harmonics can be done simpler as for general spherical harmonics. The below
// function returns zonal harmonics, rotated into the specified direction.
float4 ClampedCosineCoeffs(in float3 dir)
{
    float4 coeffs;
    coeffs.x =  0.8862269262f;          // PI/(2*sqrt(PI))
    coeffs.y = -1.0233267079f * dir.y;  // -((2.0f*PI)/3.0f)*sqrt(3/(4*PI)) * y
    coeffs.z =  1.0233267079f * dir.z;  //  ((2.0f*PI)/3.0f)*sqrt(3/(4*PI)) * z
    coeffs.w = -1.0233267079f * dir.x;  // -((2.0f*PI)/3.0f)*sqrt(3/(4*PI)) * x
    return coeffs;
}

float3 absem(float3 value)
{
    float div = -0.5;
    if (value.x < 0.0)
        value.x *= div; 
    if (value.y < 0.0)
        value.y *= div; 
    if (value.z < 0.0)
        value.z *= div; 
    return value;
}


RWTexture2DArray<float4> Array: register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    int3 pos = int3(id.xyz);
    float3 div = float3(id.x / 32.0, id.y / 32.0, id.z / 4.0);
    float4 color = float4(div.x, div.y, div.z, 1.0);
    Array[pos] = color;
}

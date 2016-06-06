#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"

//-----------------
//  Vertex Shader
//-----------------

static const float2 positions[3] =
{
    float2(-1.0f, -1.0f),
    float2(3.0f, -1.0f),
    float2(-1.0f, 3.0f)
};

struct VS_Output
{
    float2 position : POSITION;
    uint instanceID : INSTANCE_ID;
};

VS_Output VS(uint vertexID: SV_VertexID, uint instanceID: SV_InstanceID)
{
    VS_Output output;
    output.position = positions[vertexID];
    output.instanceID = instanceID;
    return output;
}

//-----------------
// Geometry Shader
//-----------------


struct GS_Output
{
    float4 position : SV_POSITION;
    float2 screenPos : TEXCOORD0;
    uint rtIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3)]
void GS(triangle VS_Output input[3], inout TriangleStream<GS_Output> outputStream)
{
    [unroll]
    for(uint i=0; i<3; i++)
    {
        GS_Output output;
        output.position = float4(input[i].position, 0.0f, 1.0f);
        output.screenPos = float2(input[i].position.x + 1.0, -input[i].position.y + 1.0) * 0.5;
        output.rtIndex = input[0].instanceID; // write 3 instances of primitive into 3 slices of 2D texture array
        outputStream.Append(output);
    }

    outputStream.RestartStrip();
}

//-----------------
//  Pixel Shader
//-----------------

#ifdef COMPILEPS

float4 PS(GS_Output input) : SV_TARGET
{
#if 1
    float4 output = Sample2D(DiffMap, input.screenPos);
    
    if (input.rtIndex == 1)
        output.r += 0.3;
    else if (input.rtIndex == 2)
        output.g += 0.3;
    else if (input.rtIndex == 3)
        output.b += 0.3;

    return output;
#else
    return Sample2D(DiffMap, input.position.xy / 1024.0);
#endif

}

#endif

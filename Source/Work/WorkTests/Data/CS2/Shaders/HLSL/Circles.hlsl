
//
// Adaptation for Urho3D of "Compute Shader Filters" by Marco Alamia:
// http://www.codinglabs.net/tutorial_compute_shaders_filters.aspx
//

struct Pixel
{
    int colour;
};

StructuredBuffer<Pixel> Buffer0 : register(t0);
RWStructuredBuffer<Pixel> BufferOut : register(u0);

cbuffer CustomUB : register(b0)
{
    float4 cCircleColor;
}

float4 readPixel(int x, int y)
{
    float4 output;
    uint index = (x + y * 1024);

    output.x = (float)(((Buffer0[index].colour ) & 0x000000ff)      ) / 255.0f;
    output.y = (float)(((Buffer0[index].colour ) & 0x0000ff00) >> 8 ) / 255.0f;
    output.z = (float)(((Buffer0[index].colour ) & 0x00ff0000) >> 16) / 255.0f;
    output.w = (float)(((Buffer0[index].colour ) & 0xff000000) >> 24) / 255.0f;

    return output;
}

float4 readOutputPixel(int x, int y)
{
    float4 output;
    uint index = (x + y * 1024);

    output.x = (float)(((BufferOut[index].colour ) & 0x000000ff)      ) / 255.0f;
    output.y = (float)(((BufferOut[index].colour ) & 0x0000ff00) >> 8 ) / 255.0f;
    output.z = (float)(((BufferOut[index].colour ) & 0x00ff0000) >> 16) / 255.0f;
    output.w = (float)(((BufferOut[index].colour ) & 0xff000000) >> 24) / 255.0f;

    return output;
}

void writeToPixel(int x, int y, float4 colour)
{
    uint index = (x + y * 1024);

    int ired   = (int)(clamp(colour.r,0,1) * 255);
    int igreen = (int)(clamp(colour.g,0,1) * 255) << 8;
    int iblue  = (int)(clamp(colour.b,0,1) * 255) << 16;
    int ialpha = (int)(clamp(colour.a,0,1) * 255) << 24;

    BufferOut[index].colour = ired + igreen + iblue + ialpha;
}

[numthreads(32, 32, 1)]
void CS( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;

    float4 pixel = readPixel(dispatchThreadID.x, dispatchThreadID.y);

    //if( readOutputPixel(x,y).a < 0.5 )
        writeToPixel(dispatchThreadID.x, dispatchThreadID.y, pixel);

    GroupMemoryBarrierWithGroupSync();

    if( dot(pixel.rgb,float3(1,1,1)) > 2.79 )
    {
        for( float alpha = 0; alpha<360; alpha+=1 )
        {
            int clampedX = clamp(x + cos(alpha)*15, 0, 1024);
            int clampedY = clamp(y + sin(alpha)*15, 0, 768);//336);
            //writeToPixel( clampedX, clampedY, float4(0.2,0,0.2,1.0) );
            writeToPixel( clampedX, clampedY, cCircleColor );
        }
    }
}

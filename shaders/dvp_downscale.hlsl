
cbuffer cbDefault : register(b0)
{
    float4 rtSize;
    matrix invViewProjection;
    matrix viewProjection;
};

Texture2D<float>    inputDepth  : register(t0);
RWTexture2D<float>  outputDepth : register(u0);

float3 project(float3 p, matrix mat)
{
    float4 pt = mul(float4(p.xyz, 1.0), mat);
    pt /= pt.w;
    return mad(pt.xyz, 0.5, 0.5);
}

float3 unproject(float3 p, matrix mat)
{
    float4 pt = mul(float4(mad(p.xyz, 2.0, -1.0), 1.0), mat);
    pt /= pt.w;
    return pt.xyz;
}

[numthreads(16, 16, 1)]
void cs_main(uint3 dispatchID : SV_DispatchThreadID)
{
    float2 uv = dispatchID.xy / rtSize.xy;

    float   depth   = inputDepth.Load(uint3(dispatchID.xy, 0));

    float3  rpos    = unproject(float3(uv.xy, depth), invViewProjection);
    float3  ppos    = project(rpos, viewProjection);

    int2    outUV   = int2(ceil(ppos.xy * rtSize.xy));
    int2    deltaUV = outUV - dispatchID.xy;

    outputDepth[outUV]              = ppos.z;
    outputDepth[outUV + deltaUV]    = depth;

    GroupMemoryBarrierWithGroupSync();

    [unroll] for (int i = -2; i < 2; ++i) {
        [unroll] for (int j = -2; j < 2; ++j) {
            outputDepth[outUV.xy + int2(i, j)] = min(inputDepth[outUV.xy + int2(i, j)], depth);
        }
    }
}

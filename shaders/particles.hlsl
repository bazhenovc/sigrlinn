
//#define USE_OIT 1

struct ParticleData
{
    float4 position;
    float4 velocity;
    float4 params;
};

struct VS_INPUT
{
    uint vertexID   : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color    : TEXCOORD0;
    float4 params   : TEXCOORD1;
};

struct GS_OUTPUT
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float4 color    : TEXCOORD1;
    float4 params   : TEXCOORD2;
};

cbuffer cbImmutable
{
    static float3 g_positions[4] =
    {
        float3( -1, 1, 0 ),
        float3( 1, 1, 0 ),
        float3( -1, -1, 0 ),
        float3( 1, -1, 0 ),
    };

    static float2 g_texcoords[4] = 
    { 
        float2(0,0), 
        float2(1,0),
        float2(0,1),
        float2(1,1),
    };
};

cbuffer ConstantBuffer: register(c0)
{
    row_major matrix    mvp;
    float3              cameraPosition;
};

SamplerState                    samplerDefault  : register(s0);
StructuredBuffer<ParticleData>  particleDataIn  : register(b0);
Texture2D                       particleTexture : register(t1);

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    ParticleData pdata = particleDataIn[input.vertexID];
    
    output.position = pdata.position;
    output.params   = pdata.params;

    float fade = rcp(pdata.velocity.w);

    output.color = float4(pdata.params.z, 0, 0, 1.0) * fade;

    return output;
}

[maxvertexcount(4)]
void gs_main(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> stream)
{
    GS_OUTPUT output;

    for (int i = 0; i < 4; ++i) {
        float3 position = g_positions[i] * input[0].params.y + input[0].position;
        output.position = mul(float4(position, 1.0F), mvp);
        output.uv       = g_texcoords[i];
        output.params   = input[0].params;
        output.color    = input[0].color;

        stream.Append(output);
    }
    stream.RestartStrip();
}

#ifdef USE_OIT
struct ListNode
{
    uint packedColor;
    uint depthAndCoverage;
    uint nextNodeID;
};

uint packColor(float4 color)
{
    return
        (uint(color.r * 255) << 24) | 
        (uint(color.g * 255) << 16) |
        (uint(color.b * 255) << 8)  |
        (uint(color.a * 255));
}

globallycoherent RWTexture2D<uint>              headBuffer : register(u1);
globallycoherent RWStructuredBuffer<ListNode>   listBuffer : register(u2);

[earlydepthstencil]
float4 ps_main(GS_OUTPUT input, uint coverage : SV_Coverage) : SV_Target
{
    float4 color = particleTexture.Sample(samplerDefault, input.uv);

    uint prevNodeID = 0;
    uint nextNodeID = listBuffer.IncrementCounter();
    if (nextNodeID == 0xffffffff)
        return float4(0, 0, 0, 0);

    uint2 upos = uint2(input.position.xy);

    InterlockedExchange(headBuffer[upos], nextNodeID, prevNodeID);

    uint depth = f32tof16(input.position.w);

    ListNode node;
    node.packedColor        = packColor(color);
    node.depthAndCoverage   = depth | (coverage << 16);
    node.nextNodeID         = prevNodeID;

    listBuffer[nextNodeID]  = node;

    return color;
}

#else
float4 ps_main(GS_OUTPUT input) : SV_Target
{
    float4 color = particleTexture.Sample(samplerDefault, input.uv);
    clip(color.a - 0.2);

    return color;
}
#endif

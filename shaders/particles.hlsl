
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
};

struct GS_OUTPUT
{
    float4 position : SV_Position;
    float4 color    : TEXCOORD0;
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
    row_major matrix mvp;
}

StructuredBuffer<ParticleData> particleDataIn : register(b0);

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    ParticleData pdata = particleDataIn[input.vertexID];
    
    output.position = pdata.position;

    float fade = rcp(pdata.velocity.w);

    output.color = float4(1.0F, 0, 0, 1.0) * fade;

    return output;
}

[maxvertexcount(4)]
void gs_main(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> stream)
{
    GS_OUTPUT output;

    for (int i = 0; i < 4; ++i) {
        float3 position = g_positions[i] * 0.1F + input[0].position;
        output.position = mul(float4(position, 1.0F), mvp);
        output.color    = input[0].color;

        stream.Append(output);
    }
    stream.RestartStrip();
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    return input.color;
}

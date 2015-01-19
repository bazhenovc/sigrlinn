
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

struct ConstantData
{
    float4x4 mvp;
    float4x4 invView;

    float4   strideOffset[(2048 - 128) / 16];
};
StructuredBuffer<ConstantData> g_ConstantBuffer;

struct ParticleData
{
    float4 position;
    float4 velocity;
};
StructuredBuffer<ParticleData> g_VertexBuffer;
StructuredBuffer<uint>         g_IndexBuffer;

struct VS_INPUT
{
    uint instanceID : SV_InstanceID;
    uint vertexID   : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    //float4 color;
};

struct GS_OUTPUT
{
    //float2 texcoord : TEXCOORD0;
    //float4 color    : COLOR;
    float4 position : SV_Position;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.texcoord = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.position = float4(output.texcoord * float2(2,-2) + float2(-1,1), 0, 1);

    return output;
}

[maxvertexcount(4)]
void gs_main(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> stream)
{
    GS_OUTPUT output;

    for (int i = 0; i < 4; ++i) {
        float3 position = g_positions[i] * 10.0F;
        position = float3(0, 0, 0);// + input[0].position;
        output.position = float4(position, 1.0);//mul(float4(position, 1.0F), g_ConstantBuffer[0].mvp);

        stream.Append(output);
    }
    stream.RestartStrip();
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    return float4(1, 1, 1, 1);
}

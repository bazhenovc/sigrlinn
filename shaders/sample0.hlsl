// vertex
struct VertexData
{
    float3 position;
    float2 texcoord0;
    float2 texcoord1;
    float3 normal;
};
StructuredBuffer<VertexData> g_VertexBuffer;
StructuredBuffer<uint>       g_IndexBuffer;

// pipeline state
#define DRAW 0
#define DRAW_INDEXED 1
struct ConstantData
{
    uint4    internalData;

    float4x4 World;
    float4x4 View;
    float4x4 Projection;

    float4   strideOffset[(2048 - 200) / 16];
};
StructuredBuffer<ConstantData> g_ConstantBuffer;

//=============================================================================
struct VS_OUTPUT
{
    float4 position  : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float3 normal    : NORMAL;
};

struct VS_INPUT
{
    uint instanceID : SV_InstanceID;
    uint vertexID   : SV_VertexID;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
    uint instanceID = input.instanceID;
    uint vertexID   = input.vertexID;

    uint vbID     = g_ConstantBuffer[instanceID].internalData[0];
    uint ibID     = g_ConstantBuffer[instanceID].internalData[1];
    uint drawType = g_ConstantBuffer[instanceID].internalData[2];
    
    VertexData vdata;
    [branch] if (drawType == DRAW_INDEXED) vdata = g_VertexBuffer[vbID + g_IndexBuffer[ibID + vertexID]];
    else     if (drawType == DRAW)         vdata = g_VertexBuffer[vbID + vertexID];

    float4 v_position = float4(vdata.position, 1.0);

    VS_OUTPUT output = (VS_OUTPUT)0;
    output.position = mul(v_position,      g_ConstantBuffer[instanceID].World);
    output.position = mul(output.position, g_ConstantBuffer[instanceID].View);
    output.position = mul(output.position, g_ConstantBuffer[instanceID].Projection);

    output.texcoord0 = vdata.texcoord0;
    output.texcoord1 = vdata.texcoord1;
    output.normal    = mul(vdata.normal,   g_ConstantBuffer[instanceID].World);

    return output;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    return float4(input.normal, 1.0);
}

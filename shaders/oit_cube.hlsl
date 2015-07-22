cbuffer ConstantBuffer: register(b0)
{
    row_major matrix mvp;
}

struct VS_INPUT
{
    float3 position    : POSITION;
    float2 texcoord0   : TEXCOORDA;
    float2 texcoord1   : TEXCOORDB;
    float3 normal      : NORMAL;
    uint   boneIDs     : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
    uint   vertexColor : VCOLOR;
};

struct VS_OUTPUT
{
    float4 position  : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float3 normal    : TEXCOORD2;
    float4 color     : TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    float4 position = float4(input.position, 1.0);
    output.position = mul(position, mvp);

    output.texcoord0 = input.texcoord0;
    output.texcoord1 = input.texcoord1;
    output.normal    = input.normal;

    output.color     = float4(float(0.5).xxx * abs(input.normal), 0.5);

    return output;
}

struct ListNode
{
    uint    packedColor;
    float   depth;
    uint    nextNodeID;
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
float4 ps_main(VS_OUTPUT input) : SV_Target
{
    float4 color = input.color;

    uint2 upos = uint2(input.position.xy);

    uint prevNodeID = 0;
    uint nextNodeID = listBuffer.IncrementCounter();
    if (nextNodeID == 0xffffffff)
        discard;

    InterlockedExchange(headBuffer[upos], nextNodeID, prevNodeID);

    ListNode node;
    node.packedColor    = packColor(color);
    node.depth          = input.position.w;
    node.nextNodeID     = prevNodeID;

    listBuffer[nextNodeID] = node;

    return color;
}

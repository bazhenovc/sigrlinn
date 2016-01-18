// vertex
struct VertexData
{
    float3 position;
    float2 texcoord0;
    float2 texcoord1;
    float3 normal;
    uint   boneIDs;
    float4 boneWeights;
    uint   vertexColor;
};
StructuredBuffer<VertexData> g_VertexBuffer : register(t0);
StructuredBuffer<uint>       g_IndexBuffer  : register(t1);

// pipeline state
#define DRAW 0
#define DRAW_INDEXED 1
struct ConstantData
{
    uint4   internalData;
    matrix  mvp;
    float4  boundingBoxMin;
    float4  boundingBoxMax;
};
StructuredBuffer<ConstantData> g_ConstantBuffer : register(t2);

// samplers and textures
Texture2D    textureDiffuse : register(t3);
SamplerState samplerLinear  : register(s0);

//=============================================================================
struct VS_OUTPUT
{
    float4 position    : SV_POSITION;
    float2 texcoord0   : TEXCOORD0;
    float2 texcoord1   : TEXCOORD1;
    float3 normal      : TEXCOORD2;
    uint   boneIDs     : TEXCOORD3;
    float4 boneWeights : TEXCOORD4;
    uint   vertexColor : TEXCOORD5;
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

    ConstantData cdata = g_ConstantBuffer[instanceID];

    uint vbID     = cdata.internalData[0];
    uint ibID     = cdata.internalData[1];
    uint drawType = cdata.internalData[2];

    VertexData vdata;
    [branch] if (drawType == DRAW_INDEXED) vdata = g_VertexBuffer[vbID + g_IndexBuffer[ibID + vertexID]];
    else     if (drawType == DRAW)         vdata = g_VertexBuffer[vbID + vertexID];

    float4 v_position = float4(vdata.position, 1.0);

    // XXX: texcoords are inverted for some reason, fix the exporter!
    vdata.texcoord0.y = 1.0 - vdata.texcoord0.y;
    vdata.texcoord1.y = 1.0 - vdata.texcoord1.y;

    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(v_position, cdata.mvp);
    output.texcoord0   = vdata.texcoord0;
    output.texcoord1   = vdata.texcoord1;
    output.normal      = vdata.normal;//mul(vdata.normal,   g_ConstantBuffer[instanceID].World);
    output.boneIDs     = vdata.boneIDs;
    output.boneWeights = vdata.boneWeights;
    output.vertexColor = vdata.vertexColor;

    return output;
}

float4 unpackVertexColor(int vcolor)
{
    return fmod(float4(vcolor / 262144.0, vcolor / 4096.0, vcolor / 64.0, vcolor), 64.0);
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    float4 color = textureDiffuse.Sample(samplerLinear, input.texcoord0);
    clip(color.a < 0.1 ? -1 : 1);
    return color;
    //return float4(input.texcoord0, 0, 1);
    //return unpackVertexColor(input.vertexColor);
}

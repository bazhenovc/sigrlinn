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

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    float4 color = input.color;

    return color;
}


// pipeline state
struct ConstantData
{
	float4x4 World;
	float4x4 View;
	float4x4 Projection;

	float4   strideOffset[(2048 - 192) / 16];
};
StructuredBuffer<ConstantData> g_ConstantBuffer : register(b0);

//SamplerState g_SamplerDefault : register(s0);

// per-object data
//Texture2D g_DiffuseTexture[120] : register(t1);

//
struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color    : COLOR0;
};

struct VS_INPUT
{
	float4 position : POSITION;
	float4 color    : COLOR;

	uint   id       : SV_InstanceID;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.position = mul(input.position,  g_ConstantBuffer[input.id].World);
	output.position = mul(output.position, g_ConstantBuffer[input.id].View);
	output.position = mul(output.position, g_ConstantBuffer[input.id].Projection);
	output.color    = input.color;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
	int id = 0;
	float2 texcoord = float2(0, 0);

	//float4 color = g_DiffuseTexture[id].Sample(g_SamplerDefault, texcoord);
	return input.color;// + color;
}

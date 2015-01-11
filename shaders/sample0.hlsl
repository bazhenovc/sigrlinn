cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color    : COLOR0;
};

VS_OUTPUT vs_main(float4 position : POSITION, float4 color : COLOR)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.position = mul(position, World);
	output.position = mul(output.position, View);
	output.position = mul(output.position, Projection);
	output.color = color;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
	return input.color;
}

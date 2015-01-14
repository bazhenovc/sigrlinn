// vertex
struct VertexData
{
	float4 position;
	float4 color;
};
StructuredBuffer<VertexData> g_VertexBuffer;
StructuredBuffer<uint>       g_IndexBuffer;

// pipeline state
struct ConstantData
{
	float4x4 World;
	float4x4 View;
	float4x4 Projection;

	float4   strideOffset[(2048 - 192) / 16];
};
StructuredBuffer<ConstantData> g_ConstantBuffer;

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
	uint instanceID : SV_InstanceID;
	uint vertexID   : SV_VertexID;
};

#define DRAW_INDEXED 0
#define DRAW 1

VS_OUTPUT vs_main(VS_INPUT input)
{
	uint instanceID = input.instanceID;
	uint vertexID   = input.vertexID;
	uint drawType   = DRAW_INDEXED;
	
	VertexData vdata;
	[branch] if (drawType == DRAW_INDEXED) vdata = g_VertexBuffer[g_IndexBuffer[vertexID]];
	else     if (drawType == DRAW)         vdata = g_VertexBuffer[vertexID];

	VS_OUTPUT output = (VS_OUTPUT)0;
	output.position = mul(vdata.position,  g_ConstantBuffer[instanceID].World);
	output.position = mul(output.position, g_ConstantBuffer[instanceID].View);
	output.position = mul(output.position, g_ConstantBuffer[instanceID].Projection);
	output.color    = vdata.color;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
	int id = 0;
	float2 texcoord = float2(0, 0);

	//float4 color = g_DiffuseTexture[id].Sample(g_SamplerDefault, texcoord);
	return input.color;// + color;
}

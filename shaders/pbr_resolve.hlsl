
struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT vs_main(in uint id : SV_VertexID)
{
    //See: https://web.archive.org/web/20140719063725/http://www.altdev.co/2011/08/08/interesting-vertex-shader-trick/
    //   1  
    //( 0, 2)
    //[-1, 3]   [ 3, 3]
    //    .
    //    |`.
    //    |  `.  
    //    |    `.
    //    '------`
    //   0         2
    //( 0, 0)   ( 2, 0)
    //[-1,-1]   [ 3,-1]
    //ID=0 -> Pos=[-1,-1], Tex=(0,0)
    //ID=1 -> Pos=[-1, 3], Tex=(0,2)
    //ID=2 -> Pos=[ 3,-1], Tex=(2,0)

    VS_OUTPUT output = (VS_OUTPUT)0;

    output.texcoord.x = (id == 2) ?  2.0 :  0.0;
    output.texcoord.y = (id == 1) ?  2.0 :  0.0;

    output.position = float4(output.texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 1.0, 1.0);

    return output;
}

SamplerState samplerDefault;
Texture2D    gbuffer0;
Texture2D    gbuffer1;

float4 ps_main(VS_OUTPUT input) : SV_Target0
{
    return gbuffer0.Sample(samplerDefault, input.texcoord);
}
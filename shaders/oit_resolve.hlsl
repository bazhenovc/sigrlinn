

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

// OIT resolve PS
#define kMaxPixels 8

struct ListNode
{
    uint    packedColor;
    float   depth;
    uint    nextNodeID;
};

struct NodeData
{
    uint    packedColor;
    float   depth;
};

RWTexture2D<uint>               headBuffer  : register(u1);
RWStructuredBuffer<ListNode>    listBuffer  : register(u2);

float4 unpackColor(uint color)
{
    float4 output;
    output.r = float((color >> 24)  & 0x000000ff) / 255.0f;
    output.g = float((color >> 16)  & 0x000000ff) / 255.0f;
    output.b = float((color >> 8)   & 0x000000ff) / 255.0f;
    output.a = float((color >> 0)   & 0x000000ff) / 255.0f;
    return saturate(output);
}

void insertionSort(uint startIndex, inout NodeData sortedFragments[kMaxPixels], out int counter)
{
    counter = 0;
    uint index = startIndex;

    for (int i = 0; i < kMaxPixels; i++) {
        if (index != 0xffffffff) {
            sortedFragments[counter].packedColor = listBuffer[index].packedColor;
            sortedFragments[counter].depth       = listBuffer[index].depth;
            counter++;
            index = listBuffer[index].nextNodeID;          
        }
    }
    
    for (int k = 1; k < kMaxPixels; k++)  {
        int j = k;
        NodeData t = sortedFragments[k];
        
        while (sortedFragments[j - 1].depth < t.depth)  {
            sortedFragments[j] = sortedFragments[j - 1];
            j--;
            if (j <= 0)
                break;
        }
        
        if (j != k)
            sortedFragments[j] = t;
    }
}

float4 ps_main(VS_OUTPUT input) : SV_Target0
{
    uint2 upos = uint2(input.position.xy);
    uint index = headBuffer[upos];
    clip(index == 0xffffffff ? -1 : 1);

    float3 color = float3(0, 0, 0);
    float alpha = 1;
    
    NodeData sortedFragments[kMaxPixels];

    [unroll(kMaxPixels)] for (int j = 0; j < kMaxPixels; j++)
        sortedFragments[j] = (NodeData)0;

    int counter = 0;
    insertionSort(index, sortedFragments, counter);

    [unroll(kMaxPixels)] for (int i = 0; i < counter; i++) {
        float4 c = unpackColor(sortedFragments[i].packedColor);
        //alpha *= (1.0 - c.a);
        //color = lerp(color, c.rgb, c.a);
        //color = color * c + c;
        //color = color * 1.0 + c * c;
        color = color + (1.0 - c.a) * (c - color);
    }

    return float4(color, alpha);
    //return float(alpha).xxxx;
}

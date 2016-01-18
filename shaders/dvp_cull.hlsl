
#define kBlockSize    128

cbuffer cbDefault : register(b0)
{
    uint    numItems;
    uint    maxDrawCallCount;
    uint2   reserved;
};

struct ConstantData
{
    uint4  internalData;
    matrix mvp;
    float4 boundingBoxMin;
    float4 boundingBoxMax;
};

StructuredBuffer<ConstantData>      initialBuffer   : register(t0);
RWStructuredBuffer<ConstantData>    finalBuffer     : register(u0);
RWStructuredBuffer<uint4>           indirectBuffer  : register(u1);

[numthreads(kBlockSize, 1, 1)]
void cs_main(
    uint3 groupID       : SV_GroupID,
    uint3 dispatchID    : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex
)
{
    uint    itemsPerGroup           = numItems / kBlockSize;
    uint    itemsPerThread          = itemsPerGroup / kBlockSize;
    uint    itemCount               = 0;

    [loop] for (uint i = 0; i < itemsPerThread; ++i) {
        uint idx = i + itemsPerThread * groupIndex + itemsPerGroup * groupID;

        ConstantData cdata = initialBuffer[idx];

        {
            itemCount = indirectBuffer.IncrementCounter();
            finalBuffer[itemCount] = initialBuffer[itemCount];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // Write out indirect args
    [branch] if (dispatchID.x == 0) {
        indirectBuffer[0] = uint4(maxDrawCallCount, indirectBuffer.IncrementCounter() - 1, 0, 0);
    }
}

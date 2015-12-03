
#define kBlockSize    128

struct ConstantData
{
    uint4               internalData;
    row_major matrix    mvp;
    float4              boundingBoxMin;
    float4              boundingBoxMax;
};

StructuredBuffer<ConstantData>      initialBuffer   : register(t0);
RWStructuredBuffer<ConstantData>    finalBuffer     : register(u0);
RWStructuredBuffer<uint>            indirectBuffer  : register(u1);

groupshared uint globalInstanceCounter;

[numthreads(kBlockSize, 1, 1)]
void cs_main(
    uint3 groupID       : SV_GroupID,
    uint3 dispatchID    : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex
)
{
    uint localInstanceCounter = 0;

    [loop] for (uint i = 0; i < kBlockSize; ++i) {
        uint idx = i * kBlockSize + groupIndex;

        ConstantData data = initialBuffer[idx];

        [branch] if ((idx % 3) == 0) {
            finalBuffer[localInstanceCounter * kBlockSize + groupIndex] = data;
            localInstanceCounter++;
        }
    }

    InterlockedAdd(globalInstanceCounter, localInstanceCounter);

    GroupMemoryBarrierWithGroupSync();
}

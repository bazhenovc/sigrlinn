
#define kGroupSize    1
#define kBlockSize    128

cbuffer cbDefault : register(b0)
{
    float   numItems;
    uint    maxDrawCallCount;
    uint2   reserved;
    matrix  viewProjection;
};

struct LogialBufferData
{
    uint4   drawCallData;
    matrix  modelview;
};

StructuredBuffer<LogialBufferData>      initialBuffer           : register(t0);
StructuredBuffer<uint>                  occlusionDataBuffer     : register(t1);

RWStructuredBuffer<LogialBufferData>    finalBuffer             : register(u0);
RWStructuredBuffer<uint4>               indirectRenderBuffer    : register(u1);

[numthreads(kBlockSize, 1, 1)]
void cs_main(
    uint3 groupID       : SV_GroupID,
    uint3 dispatchID    : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex
)
{
    float   itemsPerGroup           = ceil(numItems / kGroupSize);
    uint    itemsPerThread          = ceil(itemsPerGroup / kBlockSize);
    uint    itemCount               = 0;

    [loop] for (uint i = 0; i < itemsPerThread; ++i) {
        uint idx = i + itemsPerThread * groupIndex + itemsPerGroup * groupID;

        // Occlusion test
        uint objectVisible = occlusionDataBuffer[idx];
        [branch] if (objectVisible == 1)
        {
            itemCount = indirectRenderBuffer.IncrementCounter();
            finalBuffer[itemCount] = initialBuffer[idx];
        }
    }

    /// TODO: group memory barrier SHOULD be enough, BUT does not work on NV for some reason
    //GroupMemoryBarrierWithGroupSync();
    AllMemoryBarrierWithGroupSync();

    // Write out indirect args
    [branch] if (dispatchID.x == 0) {
        int counter = indirectRenderBuffer.IncrementCounter();
        indirectRenderBuffer[0] = uint4(maxDrawCallCount, max(counter - 1, 0), 0, 0);
        //indirectRenderBuffer[0] = uint4(maxDrawCallCount, numItems, 0, 0);
        //indirectRenderBuffer[0] = uint4(0, 0, 0, 0);
    }
}

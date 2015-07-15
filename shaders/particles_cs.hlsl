
#define kBlockSize    256
#define kMaxParticles 25600

struct ParticleData
{
    float4 position;
    float4 velocity;
    float4 params;
};

StructuredBuffer<ParticleData>   oldParticleBuffer : register(b0);
RWStructuredBuffer<ParticleData> newParticleBuffer : register(b1);

groupshared float4 sharedPosition[kBlockSize];

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand(inout uint seed, float start, float end)
{
    float r = end / 4294967296.0;
    seed = wang_hash(seed);
    return start + seed * r;
}

float3 rand3(inout uint seed, float3 start, float3 end)
{
    return float3(
        rand(seed, start.x, end.x),
        rand(seed, start.y, end.y),
        rand(seed, start.z, end.z)
    );
}

[numthreads(kBlockSize, 1, 1)]
void cs_main(
    uint3 groupID       : SV_GroupID,
    uint3 dispatchID    : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex
)
{
    uint randomSeed = dispatchID.x;

    ParticleData pdata = oldParticleBuffer[dispatchID.x];
    const float3 acceleration = float3(0, -0.1, 0);

    pdata.velocity.xyz += acceleration.xyz * 0.1;
    pdata.position.xyz += pdata.velocity.xyz * 0.1;
    pdata.velocity.w = length(pdata.velocity.xyz);

    pdata.params.x -= length(pdata.velocity.xyz) * 0.1 + 0.1;
    [flatten] if (pdata.params.x <= 0.0) {
        pdata.position.xyz = rand3(randomSeed, float3(-20, 0, 5), float3(50, 50, 50));
        pdata.velocity.xyz = rand3(randomSeed, float3(-2.0, -2.0, -2.0), float3(5.0, 5.0, 5.0));
        pdata.params.x     = rand(randomSeed, 20.0, 50.0);
    }

    newParticleBuffer[dispatchID.x] = pdata;
}

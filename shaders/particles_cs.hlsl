
#define kBlockSize    128

cbuffer ConstantBuffer: register(c0)
{
    row_major matrix    mvp;
    float3              cameraPosition;
};

struct ParticleData
{
    float4 position;
    float4 velocity;
    float4 params;      // {life, size, color, 0}
};

StructuredBuffer<ParticleData>   oldParticleBuffer      : register(b0);
RWStructuredBuffer<ParticleData> newParticleBuffer      : register(b1);
RWStructuredBuffer<ParticleData> groundParticleBuffer   : register(b2);

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
    const float3 acceleration = float3(0, -0.1, 0);

    [loop] for (uint i = 0; i < kBlockSize; ++i) {
        uint idx = i * kBlockSize + groupIndex;

        uint randomSeed = idx;

        ParticleData pdata = oldParticleBuffer[idx];

        pdata.velocity.xyz += acceleration.xyz * 0.1;
        pdata.position.xyz += pdata.velocity.xyz * 0.5;
        pdata.velocity.w = length(pdata.velocity.xyz);

        pdata.params.x -= length(pdata.velocity.xyz) * 0.5 + 0.1;
        pdata.params.y  = 0.3;
        pdata.params.z  = 1.0;

        //[branch] if (pdata.params.x <= 0.0) {
        [branch] if (pdata.position.y <= 0.0) {
            ParticleData groundParticle = pdata;
            groundParticle.params.x = 1.0;
            groundParticle.params.y = 1.6;
            groundParticle.params.z = 0.0;

            groundParticleBuffer[idx] = groundParticle;

            pdata.position.xyz = cameraPosition + rand3(randomSeed, float3(-20, 20, -20), float3(50, 70, 50));
            //pdata.velocity.xyz = rand3(randomSeed, float3(-2.0, -2.0, -2.0), float3(5.0, 5.0, 5.0));
            pdata.velocity.xyz = float3(0.0, 0.0, 0.0);
            pdata.params.x     = pdata.position.y * 10; //rand(randomSeed, 20.0, 50.0);
        }

        newParticleBuffer[idx] = pdata;
    }
}

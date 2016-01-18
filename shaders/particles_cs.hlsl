
#define kBlockSize    128
#define kMaxParticles (500 * 1024)

cbuffer ConstantBuffer: register(b0)
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

StructuredBuffer<ParticleData>   oldParticleBuffer      : register(t0);
RWStructuredBuffer<ParticleData> newParticleBuffer      : register(u0);
RWStructuredBuffer<ParticleData> groundParticleBuffer   : register(u1);

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

float2 rand2(inout uint seed, float2 start, float2 end)
{
    return float2(
        rand(seed, start.x, end.x),
        rand(seed, start.y, end.y)
    );
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
    const float3    acceleration = float3(0, -0.1, 0);

    const uint      numParticles        = kMaxParticles;
    const uint      particlesPerGroup   = numParticles / kBlockSize;
    const uint      particlesPerThread  = particlesPerGroup / kBlockSize;

    const float3    boxSize             = float3(1, 1, 5);
    const float3    halfBoxSize         = boxSize * 0.5;

    [loop] for (uint i = 0; i < particlesPerThread; ++i) {
        uint idx = i + particlesPerThread * groupIndex + particlesPerGroup * groupID;

        uint randomSeed = idx;

        ParticleData pdata = oldParticleBuffer[idx];

        pdata.velocity.xyz += acceleration.xyz * 0.0015 * pdata.velocity.w;
        pdata.position.xyz += pdata.velocity.xyz * 0.5;
        //pdata.velocity.w = length(pdata.velocity.xyz);

        pdata.params.x -= length(pdata.velocity.xyz) * 0.5 + 0.1;
        pdata.params.y  = 0.005;
        pdata.params.z  = 1.0;

        //[branch] if (pdata.params.x <= 0.0) {
        [branch] if (pdata.position.y <= 0.0) {
            ParticleData groundParticle = pdata;
            groundParticle.params.x = 1.0;
            groundParticle.params.y = 0.005;
            groundParticle.params.z = 0.0;

            groundParticle.position.y = 0.0;

            groundParticleBuffer[idx] = groundParticle;

            pdata.position.xz  = rand2(randomSeed, float2(0.0, 0.0), boxSize.xz) - halfBoxSize.xz;
            pdata.position.y   = rand(randomSeed, boxSize.y * 0.75, boxSize.y);

            pdata.velocity.xyz = float3(0.0, 0.0, 0.0);
            pdata.velocity.w   = rand(randomSeed, 0.5, 1.0);
            pdata.params.x     = pdata.position.y * 10; //rand(randomSeed, 20.0, 50.0);
        }

        newParticleBuffer[idx] = pdata;
    }
}

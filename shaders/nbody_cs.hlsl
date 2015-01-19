
#define kBlockSize 128

static const float softeningSquared = 0.0012500000*0.0012500000;
static const float gravity          = 6.67300e-11f * 10000.0f;
static const float particleMass     = gravity * 10000.0f * 10000.0f;

void solveBodyBody(inout float3 ai, float4 bj, float4 bi, float mass)
{
    float3 r = bj.xyz - bi.xyz;

    float distSqr = dot(r, r);
    distSqr += softeningSquared;

    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube =  invDist * invDist * invDist;

    float s = mass * invDistCube;

    ai += r * s;
}

struct ParticleData
{
    float4 position;
    float4 velocity;
};

StructuredBuffer<ParticleData>   oldParticleBuffer;// : register(b0);
RWStructuredBuffer<ParticleData> newParticleBuffer;// : register(b1);

groupshared float4 sharedPosition[kBlockSize];

[numthreads(kBlockSize, 1, 1)]
void cs_main(
    uint3 groupID       : SV_GroupID,
    uint3 dispatchID    : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex
)
{
    float4 position     = oldParticleBuffer[dispatchID.x].position;
    float4 velocity     = oldParticleBuffer[dispatchID.x].velocity;
    float3 acceleration = float3(0, 0, 0);
    float mass          = particleMass;

    const int dimx = 128;

#if 0
    [loop]
    for (uint tile = 0; tile < dimx; tile++) {

        sharedPosition[groupIndex] = oldParticleBuffer[tile * kBlockSize + groupIndex].position;

        GroupMemoryBarrierWithGroupSync();

        [unroll]
        for (uint counter = 0; counter < kBlockSize; counter += 8) {
            solveBodyBody(acceleration, sharedPosition[counter + 0], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 1], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 3], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 4], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 5], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 6], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 2], position, mass);
            solveBodyBody(acceleration, sharedPosition[counter + 7], position, mass);
        }
        GroupMemoryBarrierWithGroupSync();
    }
#endif
    velocity.xyz += acceleration.xyz * 0.1;
    //velocity.xyz *= damp

    newParticleBuffer[dispatchID.x].position = float4(0, 0, 0, 1);
    newParticleBuffer[dispatchID.x].velocity = float4(velocity.xyz, length(acceleration));
}

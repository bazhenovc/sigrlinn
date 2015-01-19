
#include "common/app.hh"

#include <memory>

#include <DirectXMath.h>
using namespace DirectX;

class NBodyApplication : public Application
{
public:

    struct Particle
    {
        XMFLOAT4 position = XMFLOAT4(0.0F, 0.0F, 0.0F, 1.0F);
        XMFLOAT4 velocity = XMFLOAT4(1.0F, 1.0F, 0.2F, 1.0F);
    };

    struct ConstantBuffer
    {
        uint32_t sgfxInternalData[4]; // first 4 ints are ALWAYS reserved for sgfx internal data
                                      // at this time: VB+IB+DrawCallType+0

        XMMATRIX mvp;
        XMMATRIX invView;
    };

    enum
    {
        kNumParticles = 128 * 128
    };

    Particle*                 particles;
    sgfx::RWBufferHandle      particlesRWBuffer;
    sgfx::BufferHandle        particlesBuffer;
    sgfx::ComputeShaderHandle csHandle;

    sgfx::VertexShaderHandle   vsHandle;
    sgfx::GeometryShaderHandle gsHandle;
    sgfx::PixelShaderHandle    psHandle;
    sgfx::SurfaceShaderHandle  ssHandle;

    sgfx::PipelineStateHandle pipelineState;
    sgfx::DrawQueueHandle     drawQueue;

    XMMATRIX                  g_View;
    XMMATRIX                  g_Projection;

public:

    void* operator new(size_t size)
    {
        return _mm_malloc(size, 32);
    }

    void operator delete(void* ptr)
    {
        _mm_free(ptr);
    }

    virtual void loadSampleData() override
    {
        // Initialize the view matrix
        XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f );
        XMVECTOR At  = XMVectorSet( 0.0f, 0.0f, 10.0f, 0.0f );
        XMVECTOR Up  = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
        g_View = XMMatrixLookAtLH( Eye, At, Up );

        // Initialize the projection matrix
        g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );

        // setup Sigrlinn
        sgfx::initD3D11(g_pd3dDevice, g_pImmediateContext, g_pSwapChain);

        csHandle = loadCS("shaders/nbody_cs.hlsl");
        vsHandle = loadVS("shaders/nbody.hlsl");
        gsHandle = loadGS("shaders/nbody.hlsl");
        psHandle = loadPS("shaders/nbody.hlsl");

        if (vsHandle != sgfx::VertexShaderHandle::invalidHandle()
            && gsHandle != sgfx::GeometryShaderHandle::invalidHandle()
            && psHandle != sgfx::PixelShaderHandle::invalidHandle()) {
            ssHandle = sgfx::linkSurfaceShader(
                vsHandle,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                psHandle
            );
        }

        if (ssHandle != sgfx::SurfaceShaderHandle::invalidHandle()) {
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CCW;

            desc.blendState.blendDesc.blendEnabled                  = false;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All; // not implemented
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp                       = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha                 = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha                 = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha                  = sgfx::BlendOp::Add;

            desc.depthStencilState.depthEnabled                     = true;
            desc.depthStencilState.writeMask                        = sgfx::DepthWriteMask::All;
            desc.depthStencilState.depthFunc                        = sgfx::DepthFunc::Less;

            desc.depthStencilState.stencilEnabled                   = false;
            desc.depthStencilState.stencilRef                       = 0;
            desc.depthStencilState.stencilReadMask                  = 0;
            desc.depthStencilState.stencilWriteMask                 = 0;

            desc.depthStencilState.frontFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.frontFaceStencilDesc.failOp      = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.passOp      = sgfx::StencilOp::Keep;

            desc.depthStencilState.backFaceStencilDesc.stencilFunc  = sgfx::StencilFunc::Always;
            desc.depthStencilState.backFaceStencilDesc.failOp       = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.depthFailOp  = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.passOp       = sgfx::StencilOp::Keep;

            desc.shader = ssHandle;

            pipelineState = sgfx::createPipelineState(desc);
            if (pipelineState != sgfx::PipelineStateHandle::invalidHandle()) {
                drawQueue = sgfx::createDrawQueue(pipelineState);
            } else {
                OutputDebugString("Failed to create pipeline state!");
            }
        }

        particles = new Particle[kNumParticles];

        particlesRWBuffer = sgfx::createRWBuffer(nullptr, kNumParticles * sizeof(Particle), sizeof(Particle));
        particlesBuffer   = sgfx::createBuffer(sgfx::BufferType::Vertex, particles, kNumParticles * sizeof(Particle), sizeof(Particle));
    }

    virtual void releaseSampleData() override
    {
        OutputDebugString("Cleanup\n");
        delete [] particles;
        sgfx::releaseVertexShader(vsHandle);
        sgfx::releaseGeometryShader(gsHandle);
        sgfx::releasePixelShader(psHandle);
        sgfx::releaseSurfaceShader(ssHandle);
        sgfx::releasePipelineState(pipelineState);
        sgfx::releaseDrawQueue(drawQueue);
        sgfx::releaseComputeShader(csHandle);
        sgfx::releaseRWBuffer(particlesRWBuffer);
        sgfx::releaseBuffer(particlesBuffer);
        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        // update constants
        ConstantBuffer constants;
        //constants.mvp     = XMMatrixTranspose(g_View * g_Projection);
        //constants.invView = XMMatrixTranspose(XMMatrixInverse(nullptr, g_View));
        // dispatch compute
        //sgfx::copyRWBuffer(particlesRWBuffer, particlesBuffer);

        //sgfx::setComputeShaderBuffer(0, particlesBuffer);
        //sgfx::setComputeShaderBuffer(0, particlesRWBuffer);
        //sgfx::dispatchComputeShader(csHandle, 128, 1, 1);

        // draw particles
        sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::PointList);
        sgfx::setConstants(drawQueue, 0, &constants, sizeof(constants));
        sgfx::setVertexBuffer(drawQueue, particlesBuffer);
        sgfx::draw(drawQueue, 3, 0);

        sgfx::submit(drawQueue);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new NBodyApplication;
}

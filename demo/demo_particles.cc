/// The MIT License (MIT)
///
/// Copyright (c) 2015 Kirill Bazhenov
/// Copyright (c) 2015 BitBox, Ltd.
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
#include "common/app.hh"
#include "common/textureloader.hh"

#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class ParticlesApplication : public Application
{
public:

    enum
    {
        kMaxParticles   = 500 * 1024,
        kBlockSize      = 128,
        kMaxOITPixels   = 8
    };

    struct ParticleBuffer
    {
        sgfx::BufferHandle bufferIn;
        sgfx::BufferHandle bufferOut;

        inline ~ParticleBuffer()
        {
            release();
        }

        inline void release()
        {
            sgfx::releaseBuffer(bufferIn);  bufferIn    = sgfx::BufferHandle::invalidHandle();
            sgfx::releaseBuffer(bufferOut); bufferOut   = sgfx::BufferHandle::invalidHandle();
        }

        inline void init()
        {
            bufferIn = sgfx::createBuffer(
                sgfx::BufferFlags::StructuredBuffer | sgfx::BufferFlags::CPUWrite, nullptr,
                kMaxParticles * sizeof(ParticleData), sizeof(ParticleData)
            );
            bufferOut = sgfx::createBuffer(
                sgfx::BufferFlags::GPUWrite, nullptr,
                kMaxParticles * sizeof(ParticleData), sizeof(ParticleData)
            );
        }

        inline void update()
        {
            sgfx::copyResource(bufferOut, bufferIn);
        }
    };

#ifdef USE_OIT
    struct OITListNode
    {
        uint32_t    packedColor;
        float       depth;
        uint32_t    nextNodeID;
    };
#endif

    struct ParticleData
    {
        float position[4];
        float velocity[4];
        float params[4];
    };

    struct ConstantBuffer
    {
        float mvp[16];
        float cameraPosition[4];
    };

#ifdef USE_OIT
    sgfx::TextureHandle         oitHeadBuffer;
    sgfx::BufferHandle          oitListBuffer;

    sgfx::PipelineStateHandle   oitPipelineState;
    sgfx::DrawQueueHandle       oitDrawQueue;
    sgfx::VertexShaderHandle    oitVS;
    sgfx::PixelShaderHandle     oitPS;
    sgfx::SurfaceShaderHandle   oitSS;
#endif

    sgfx::TextureHandle         snowflakeTexture;
    sgfx::SamplerStateHandle    samplerState;

    ParticleBuffer              fallingParticles;
    ParticleBuffer              groundParticles;

    sgfx::VertexShaderHandle    vsHandle;
    sgfx::GeometryShaderHandle  gsHandle;
    sgfx::PixelShaderHandle     psHandle;
    sgfx::SurfaceShaderHandle   ssHandle;

    sgfx::ConstantBufferHandle  constantBuffer;

    sgfx::PipelineStateHandle   pipelineState;
    sgfx::DrawQueueHandle       drawQueue;

    sgfx::ComputeShaderHandle   csHandle;
    sgfx::ComputeQueueHandle    csQueue;

    sgfx::Texture2DHandle       colorBuffer;
    sgfx::Texture2DHandle       depthStencilBuffer;
    sgfx::RenderTargetHandle    renderTarget;

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
        // setup Sigrlinn
        sgfx::initD3D11(g_pd3dDevice, g_pImmediateContext, g_pSwapChain);

        // create sampler state
        sgfx::SamplerStateDescriptor samplerDesc;
        samplerDesc.filter          = sgfx::TextureFilter::MinMagMip_Linear;
        samplerDesc.addressU        = sgfx::AddressMode::Clamp;
        samplerDesc.addressV        = sgfx::AddressMode::Clamp;
        samplerDesc.addressW        = sgfx::AddressMode::Clamp;
        samplerDesc.lodBias         = 0.0F;
        samplerDesc.maxAnisotropy   = 1;
        samplerDesc.comparisonFunc  = sgfx::ComparisonFunc::Never;
        samplerDesc.borderColor     = 0xFFFFFFFF;
        samplerDesc.minLod          = -3.402823466e+38F;
        samplerDesc.maxLod          = 3.402823466e+38F;

        samplerState = sgfx::createSamplerState(samplerDesc);

        // load textures
        snowflakeTexture = loadDDS("data/textures/snowflake.dds");

        // create compute stuff
        csHandle = loadCS("shaders/particles_cs.hlsl");
        csQueue  = sgfx::createComputeQueue(csHandle);

        fallingParticles.init();
        groundParticles.init();

        // create render target
        colorBuffer        = sgfx::getBackBuffer();
        depthStencilBuffer = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::D24S8, 1, sgfx::TextureFlags::DepthStencil
        );

        sgfx::RenderTargetDescriptor renderTargetDesc;
        renderTargetDesc.numColorTextures    = 1;
        renderTargetDesc.colorTextures[0]    = colorBuffer;
        renderTargetDesc.depthStencilTexture = depthStencilBuffer;

        renderTarget = sgfx::createRenderTarget(renderTargetDesc);

#ifdef USE_OIT
        // create OIT textures
        oitHeadBuffer = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::R32U, 1, sgfx::TextureFlags::GPUWrite
        );
        oitListBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::GPUWrite | sgfx::BufferFlags::GPUCounter, nullptr, width * height * kMaxOITPixels * sizeof(OITListNode), sizeof(OITListNode)
        );

        // bind OIT textures to the RT
        sgfx::setResourceRW(renderTarget, 0, oitHeadBuffer);
        sgfx::setResourceRW(renderTarget, 1, oitListBuffer);

        // create OIT pipeline
        oitVS = loadVS("shaders/oit_resolve.hlsl");
        oitPS = loadPS("shaders/oit_resolve.hlsl");

        if (oitVS != sgfx::VertexShaderHandle::invalidHandle() &&
            oitPS != sgfx::PixelShaderHandle::invalidHandle()) {
            oitSS = sgfx::linkSurfaceShader(
                oitVS,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                oitPS
            );
        }

        if (oitSS != sgfx::SurfaceShaderHandle::invalidHandle()) {
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CW;

            desc.blendState.blendDesc.blendEnabled                  = false;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp                       = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha                 = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha                 = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha                  = sgfx::BlendOp::Add;

            desc.depthStencilState.depthEnabled                     = false;
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

            desc.shader       = oitSS;
            desc.vertexFormat = sgfx::VertexFormatHandle::invalidHandle();

            oitPipelineState = sgfx::createPipelineState(desc);
            if (oitPipelineState != sgfx::PipelineStateHandle::invalidHandle()) {
                oitDrawQueue = sgfx::createDrawQueue(oitPipelineState);
            } else {
                OutputDebugString("Failed to create OIT pipeline state!");
            }
        }
#endif

        // constant buffer
        constantBuffer = sgfx::createConstantBuffer(nullptr, sizeof(ConstantBuffer));

        // mesh data
        vsHandle = loadVS("shaders/particles.hlsl");
        gsHandle = loadGS("shaders/particles.hlsl");
        psHandle = loadPS("shaders/particles.hlsl");

        if (vsHandle != sgfx::VertexShaderHandle::invalidHandle() &&
            psHandle != sgfx::PixelShaderHandle::invalidHandle()  &&
            gsHandle != sgfx::GeometryShaderHandle::invalidHandle()) {
            ssHandle = sgfx::linkSurfaceShader(
                vsHandle,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                gsHandle,
                psHandle
            );
        }

        if (ssHandle != sgfx::SurfaceShaderHandle::invalidHandle()) {
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CW;

            desc.blendState.blendDesc.blendEnabled                  = false;//true;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::SrcAlpha;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::OneMinusSrcAlpha;
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

            desc.shader       = ssHandle;
            desc.vertexFormat = sgfx::VertexFormatHandle::invalidHandle();

            pipelineState = sgfx::createPipelineState(desc);
            if (pipelineState != sgfx::PipelineStateHandle::invalidHandle()) {
                drawQueue = sgfx::createDrawQueue(pipelineState);
            } else {
                OutputDebugString("Failed to create pipeline state!");
            }
        }
    }

    virtual void releaseSampleData() override
    {
        OutputDebugString("Cleanup\n");

#ifdef USE_OIT
        sgfx::releaseTexture(oitHeadBuffer);
        sgfx::releaseBuffer(oitListBuffer);

        sgfx::releasePipelineState(oitPipelineState);
        sgfx::releaseDrawQueue(oitDrawQueue);
        sgfx::releaseVertexShader(oitVS);
        sgfx::releasePixelShader(oitPS);
        sgfx::releaseSurfaceShader(oitSS);
#endif

        sgfx::releaseTexture(snowflakeTexture);
        sgfx::releaseSamplerState(samplerState);

        fallingParticles.release();
        groundParticles.release();

        sgfx::releaseConstantBuffer(constantBuffer);
        sgfx::releaseVertexShader(vsHandle);
        sgfx::releaseGeometryShader(gsHandle);
        sgfx::releasePixelShader(psHandle);
        sgfx::releaseSurfaceShader(ssHandle);
        sgfx::releasePipelineState(pipelineState);
        sgfx::releaseDrawQueue(drawQueue);

        sgfx::releaseComputeShader(csHandle);
        sgfx::releaseComputeQueue(csQueue);

        sgfx::releaseRenderTarget(renderTarget);
        sgfx::releaseTexture(colorBuffer);
        sgfx::releaseTexture(depthStencilBuffer);

        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        // Update our time
        static float t = 0.0f;

        static ULONGLONG dwTimeStart = 0;
        ULONGLONG dwTimeCur = GetTickCount64();
        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;
        t = (dwTimeCur - dwTimeStart) / 1000.0f;
        dwTimeStart = dwTimeCur;

        static glm::vec3 cameraPosition = glm::vec3(0.0F, 0.3F, -3.0F);
        //cameraPosition.z -= t * 0.5F;

        glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.001f, 100.0f);
        glm::mat4 view       = glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        //glm::mat4 world      = glm::rotate(glm::mat4(1.0F), t, glm::vec3(0.0F, 1.0F, 0.0F));

        glm::mat4 mvp = projection * view;// * world;

        ConstantBuffer constants;
        std::memcpy(constants.mvp,              glm::value_ptr(mvp),            sizeof(constants.mvp));
        std::memcpy(constants.cameraPosition,   glm::value_ptr(cameraPosition), sizeof(constants.cameraPosition));
        sgfx::updateConstantBuffer(constantBuffer, &constants);

#ifdef USE_OIT
        // clear from the previous frame and set RT
        sgfx::clearTextureRW(oitHeadBuffer, 0xffffffff);
        sgfx::clearBufferRW(oitListBuffer, 0xffffffff);
#endif

        sgfx::clearRenderTarget(renderTarget, 0xFF000000);
        sgfx::clearDepthStencil(renderTarget, 1.0F, 0);
        sgfx::setRenderTarget(renderTarget);
        sgfx::setViewport(width, height, 0.0F, 1.0F);

        // dispatch compute queue
        sgfx::beginPerfEvent(L"ParticleSimulate");
        {
            sgfx::setConstantBuffer(csQueue, 0, constantBuffer);
            sgfx::setResource(csQueue, 0, fallingParticles.bufferIn);
            sgfx::setResourceRW(csQueue, 0, fallingParticles.bufferOut);
            sgfx::setResourceRW(csQueue, 1, groundParticles.bufferOut);
            sgfx::submit(csQueue, kMaxParticles / kBlockSize, 1, 1);
        }
        sgfx::endPerfEvent();

        sgfx::beginPerfEvent(L"ParticleCopy");
        {
            fallingParticles.update();
            groundParticles.update();
        }
        sgfx::endPerfEvent();

        // actually draw some stuff
        sgfx::beginPerfEvent(L"ParticleDraw");
        {
            sgfx::setSamplerState(drawQueue, 0, samplerState);

            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::PointList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setResource(drawQueue, 0, fallingParticles.bufferIn);
            sgfx::setResource(drawQueue, 1, snowflakeTexture);
            sgfx::draw(drawQueue, kMaxParticles, 0);

            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::PointList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setResource(drawQueue, 0, groundParticles.bufferIn);
            sgfx::setResource(drawQueue, 1, snowflakeTexture);
            sgfx::draw(drawQueue, kMaxParticles, 0);

            sgfx::submit(drawQueue);
        }

#ifdef USE_OIT
        // resolve OIT
        sgfx::beginPerfEvent(L"ParticleResolve");
        {
            sgfx::clearRenderTarget(renderTarget, 0xFF000000);

            sgfx::setPrimitiveTopology(oitDrawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::draw(oitDrawQueue, 3, 0);

            sgfx::submit(oitDrawQueue);
        }
        sgfx::endPerfEvent();
#endif

        sgfx::present(1);
        sgfx::endPerfEvent();
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new ParticlesApplication;
}

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

#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class ParticlesApplication : public Application
{
public:

    enum
    {
        kMaxParticles = 25600
    };

    struct ParticleData
    {
        float position[4];
        float velocity[4];
        float params[4];
    };

    struct ConstantBuffer
    {
        float mvp[16];
    };

    sgfx::BufferHandle          particlesBufferIn;
    sgfx::BufferHandle          particlesBufferOut;

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

        // create compute stuff
        csHandle = loadCS("shaders/particles_cs.hlsl");
        csQueue  = sgfx::createComputeQueue(csHandle);

        particlesBufferIn   = sgfx::createBuffer(
            sgfx::BufferFlags::StructuredBuffer | sgfx::BufferFlags::CPUWrite, nullptr,
            kMaxParticles * sizeof(ParticleData), sizeof(ParticleData)
        );
        particlesBufferOut  = sgfx::createBuffer(
            sgfx::BufferFlags::GPUWrite, nullptr,
            kMaxParticles * sizeof(ParticleData), sizeof(ParticleData)
        );

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

            desc.blendState.blendDesc.blendEnabled                  = false;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All;
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

        sgfx::releaseBuffer(particlesBufferIn);
        sgfx::releaseBuffer(particlesBufferOut);

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

        glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.01f, 100.0f);
        glm::mat4 view       = glm::lookAt(glm::vec3(0.0F, 1.0F, -5.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        //glm::mat4 world      = glm::rotate(glm::mat4(1.0F), t, glm::vec3(0.0F, 1.0F, 0.0F));

        glm::mat4 mvp = projection * view;// * world;

        ConstantBuffer constants;
        std::memcpy(constants.mvp, glm::value_ptr(mvp), sizeof(constants.mvp));
        sgfx::updateConstantBuffer(constantBuffer, &constants);

        // dispatch compute queue
        {
            sgfx::setResource(csQueue, 0, particlesBufferIn);
            sgfx::setResourceRW(csQueue, 0, particlesBufferOut);
            sgfx::submit(csQueue, 128, 1, 1);
        }

        sgfx::copyResource(particlesBufferOut, particlesBufferIn);

        // actually draw some stuff
        {
            sgfx::clearRenderTarget(renderTarget, 0xFFFFFFF);
            sgfx::clearDepthStencil(renderTarget, 1.0F, 0);
            sgfx::setRenderTarget(renderTarget);
            sgfx::setViewport(width, height, 0.0F, 1.0F);

            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::PointList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setResource(drawQueue, 0, particlesBufferIn);
            sgfx::draw(drawQueue, kMaxParticles, 0);

            sgfx::submit(drawQueue);
        }
        sgfx::present(1);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new ParticlesApplication;
}

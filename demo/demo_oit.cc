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
#include "common/meshloader.hh"

#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define USE_OIT

class OITApplication : public Application
{
public:

    struct ConstantBuffer
    {
        float mvp[16];
    };

    enum
    {
        kMaxOITPixels   = 8
    };

#ifdef USE_OIT
    struct OITListNode
    {
        uint32_t    packedColor;
        float       depth;
        uint32_t    nextNodeID;
    };
#endif

#ifdef USE_OIT
    sgfx::TextureHandle         oitHeadBuffer;
    sgfx::BufferHandle          oitListBuffer;

    sgfx::PipelineStateHandle   oitPipelineState;
    sgfx::DrawQueueHandle       oitDrawQueue;
    sgfx::VertexShaderHandle    oitVS;
    sgfx::PixelShaderHandle     oitPS;
    sgfx::SurfaceShaderHandle   oitSS;
#endif

    sgfx::VertexShaderHandle  vsHandle;
    sgfx::PixelShaderHandle   psHandle;
    sgfx::SurfaceShaderHandle ssHandle;

    MeshData                   meshData;

    sgfx::BufferHandle         modelVertexBuffer;
    sgfx::BufferHandle         modelIndexBuffer;
    sgfx::ConstantBufferHandle constantBuffer;
    sgfx::VertexFormatHandle   vertexFormat;

    sgfx::PipelineStateHandle pipelineState;
    sgfx::DrawQueueHandle     drawQueue;

    sgfx::Texture2DHandle     colorBuffer;
    sgfx::Texture2DHandle     depthStencilBuffer;
    sgfx::RenderTargetHandle  renderTarget;

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
        const size_t stride1 = 0;                             // Position
        const size_t stride2 = stride1 + 3 * sizeof(float);   // uv0
        const size_t stride3 = stride2 + 2 * sizeof(float);   // uv1
        const size_t stride4 = stride3 + 2 * sizeof(float);   // normal
        const size_t stride5 = stride4 + 3 * sizeof(float);   // boneIDs
        const size_t stride6 = stride5 + 4 * sizeof(uint8_t); // boneWeights
        const size_t stride7 = stride6 + 4 * sizeof(float);   // color
        sgfx::VertexElementDescriptor vfElements[] = {
            { "POSITION",    0, sgfx::DataFormat::RGB32F,  0, stride1 },
            { "TEXCOORDA",   0, sgfx::DataFormat::RG32F,   0, stride2 },
            { "TEXCOORDB",   0, sgfx::DataFormat::RG32F,   0, stride3 },
            { "NORMAL",      0, sgfx::DataFormat::RGB32F,  0, stride4 },
            { "BONEIDS",     0, sgfx::DataFormat::R32U,    0, stride5 },
            { "BONEWEIGHTS", 0, sgfx::DataFormat::RGBA32F, 0, stride6 },
            { "VCOLOR",      0, sgfx::DataFormat::R32U,    0, stride7 }
        };
        size_t vfSize = sizeof(vfElements) / sizeof(sgfx::VertexElementDescriptor);

        vertexFormat = loadVF(vfElements, vfSize, "shaders/oit_cube.hlsl");

        meshData.read("data/meshes/dragon/dragon1.mesh");

        modelVertexBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::VertexBuffer, 
            meshData.getVertices().data(),
            sizeof(MeshData::Vertex) * meshData.getVertices().size(),
            sizeof(MeshData::Vertex)
        );

        modelIndexBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::IndexBuffer,
            meshData.getIndices().data(),
            sizeof(uint32_t) * meshData.getIndices().size(),
            sizeof(uint32_t)
        );

#ifdef USE_OIT
        vsHandle = loadVS("shaders/oit_cube.hlsl");
        psHandle = loadPS("shaders/oit_cube.hlsl");
#else
        vsHandle = loadVS("shaders/oit_simple.hlsl");
        psHandle = loadPS("shaders/oit_simple.hlsl");
#endif

        if (vsHandle != sgfx::VertexShaderHandle::invalidHandle() && psHandle != sgfx::PixelShaderHandle::invalidHandle()) {
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
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CW;

#ifdef USE_OIT
            desc.blendState.blendDesc.blendEnabled                  = false;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp                       = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha                 = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha                 = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha                  = sgfx::BlendOp::Add;
#else
            desc.blendState.blendDesc.blendEnabled                  = true;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::SrcColor;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::OneMinusSrcAlpha;
            desc.blendState.blendDesc.blendOp                       = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha                 = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha                 = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha                  = sgfx::BlendOp::Add;
#endif

            desc.depthStencilState.depthEnabled                     = true;
            desc.depthStencilState.writeMask                        = sgfx::DepthWriteMask::Zero;
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
            desc.vertexFormat = vertexFormat;

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

        sgfx::releaseBuffer(modelVertexBuffer);
        sgfx::releaseBuffer(modelIndexBuffer);
        sgfx::releaseConstantBuffer(constantBuffer);
        sgfx::releaseVertexFormat(vertexFormat);
        sgfx::releaseVertexShader(vsHandle);
        sgfx::releasePixelShader(psHandle);
        sgfx::releaseSurfaceShader(ssHandle);
        sgfx::releasePipelineState(pipelineState);
        sgfx::releaseDrawQueue(drawQueue);

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
        glm::mat4 view       = glm::lookAt(glm::vec3(0.0F, 0.0F, -3.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        glm::mat4 world      = glm::scale(glm::mat4(1.0F), glm::vec3(2.5F)) * glm::rotate(glm::mat4(1.0F), t, glm::vec3(0.0F, 1.0F, 0.0F));

        glm::mat4 mvp = projection * view * world;

        ConstantBuffer constants;
        std::memcpy(constants.mvp, glm::value_ptr(mvp), sizeof(constants.mvp));
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

        // draw cubes to the OIT buffer
        sgfx::beginPerfEvent(L"OITRender");
        {
            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setVertexBuffer(drawQueue, modelVertexBuffer);
            sgfx::setIndexBuffer(drawQueue, modelIndexBuffer);
            sgfx::drawIndexed(drawQueue, static_cast<uint32_t>(meshData.getIndices().size()), 0, 0);

            sgfx::submit(drawQueue);
        }
        sgfx::endPerfEvent();

#ifdef USE_OIT
        // resolve OIT
        sgfx::beginPerfEvent(L"OITResolve");
        {
            sgfx::clearRenderTarget(renderTarget, 0xFF000000);

            sgfx::setPrimitiveTopology(oitDrawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::draw(oitDrawQueue, 3, 0);

            sgfx::submit(oitDrawQueue);
        }
        sgfx::endPerfEvent();
#endif

        sgfx::present(1);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new OITApplication;
}

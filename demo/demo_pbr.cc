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

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace GfxUtils
{
    inline void Release(sgfx::VertexShaderHandle obj)   { sgfx::releaseVertexShader(obj); }
    inline void Release(sgfx::HullShaderHandle obj)     { sgfx::releaseHullShader(obj); }
    inline void Release(sgfx::DomainShaderHandle obj)   { sgfx::releaseDomainShader(obj); }
    inline void Release(sgfx::GeometryShaderHandle obj) { sgfx::releaseGeometryShader(obj); }
    inline void Release(sgfx::PixelShaderHandle obj)    { sgfx::releasePixelShader(obj); }
    inline void Release(sgfx::SurfaceShaderHandle obj)  { sgfx::releaseSurfaceShader(obj); }
    inline void Release(sgfx::PipelineStateHandle obj)  { sgfx::releasePipelineState(obj); }
    inline void Release(sgfx::BufferHandle obj)         { sgfx::releaseBuffer(obj); }
    inline void Release(sgfx::ConstantBufferHandle obj) { sgfx::releaseConstantBuffer(obj); }
    inline void Release(sgfx::SamplerStateHandle obj)   { sgfx::releaseSamplerState(obj); }
    inline void Release(sgfx::TextureHandle obj)        { sgfx::releaseTexture(obj); }
    inline void Release(sgfx::RenderTargetHandle obj)   { sgfx::releaseRenderTarget(obj); }
    inline void Release(sgfx::DrawQueueHandle obj)      { sgfx::releaseDrawQueue(obj); }
    inline void Release(sgfx::VertexFormatHandle obj)   { sgfx::releaseVertexFormat(obj); }
}

template <typename T>
struct iGfxObject final
{
private:
    T gfxObject;

public:
    inline          iGfxObject()                        {}
    inline explicit iGfxObject(T newObject) : gfxObject(newObject) {}
    inline          iGfxObject(const iGfxObject& other) = delete;
    inline          ~iGfxObject()                       { GfxUtils::Release(gfxObject); }

    inline iGfxObject& operator=(T newObject)             { GfxUtils::Release(gfxObject); gfxObject = newObject; return *this; }
    inline iGfxObject& operator=(const iGfxObject& other) = delete;

    inline operator T()             { return gfxObject; }
    inline operator const T() const { return gfxObject; }

    inline bool valid() const { return gfxObject.value != 0; }
};

typedef iGfxObject<sgfx::VertexShaderHandle>   iGfxVertexShader;
typedef iGfxObject<sgfx::HullShaderHandle>     iGfxHullShader;
typedef iGfxObject<sgfx::DomainShaderHandle>   iGfxDomainShader;
typedef iGfxObject<sgfx::GeometryShaderHandle> iGfxGeometryShader;
typedef iGfxObject<sgfx::PixelShaderHandle>    iGfxPixelShader;
typedef iGfxObject<sgfx::SurfaceShaderHandle>  iGfxSurfaceShader;
typedef iGfxObject<sgfx::PipelineStateHandle>  iGfxPipelineState;
typedef iGfxObject<sgfx::BufferHandle>         iGfxBuffer;
typedef iGfxObject<sgfx::ConstantBufferHandle> iGfxConstantBuffer;
typedef iGfxObject<sgfx::SamplerStateHandle>   iGfxSamplerState;
typedef iGfxObject<sgfx::TextureHandle>        iGfxTexture;
typedef iGfxObject<sgfx::RenderTargetHandle>   iGfxRenderTarget;
typedef iGfxObject<sgfx::DrawQueueHandle>      iGfxDrawQueue;
typedef iGfxObject<sgfx::VertexFormatHandle>   iGfxVertexFormat;

class Mesh
{
private:

    iGfxBuffer vertexBuffer;
    iGfxBuffer indexBuffer;
    uint32_t   numIndices;

public:

    inline Mesh()                        {}
    inline Mesh(const std::string& path) { load(path); }
    inline ~Mesh()                       {}

    void load(const std::string& path)
    {
        MeshData data;
        data.read(path);

        vertexBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::VertexBuffer,
            data.getVertices().data(),
            data.getVertices().size() * sizeof(MeshData::Vertex),
            sizeof(MeshData::Vertex)
        );
        indexBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::IndexBuffer,
            data.getIndices().data(),
            data.getIndices().size() * sizeof(MeshData::Index),
            sizeof(MeshData::Index)
        );
        numIndices = static_cast<uint32_t>(data.getIndices().size());
    }

    inline sgfx::BufferHandle getVertexBuffer() const { return vertexBuffer; }
    inline sgfx::BufferHandle getIndexBuffer()  const { return indexBuffer;  }
    inline uint32_t           getNumIndices()   const { return numIndices;   }
};

struct Material
{
    iGfxTexture albedo;
    iGfxTexture gloss;
    iGfxTexture normal;
    iGfxTexture spec;

    inline Material()  {}
    inline ~Material() {}
};

class DeferredScene
{
private:

    struct ConstantBuffer
    {
        float mvp[16];
    };

    Mesh     akMesh;     // our AK mesh
    Material akMaterial; // mesh material

    iGfxSamplerState samplerState;
    iGfxVertexFormat vertexFormat;

    // constant buffer
    iGfxConstantBuffer constantBuffer;

    // gbuffer pass data
    iGfxVertexShader  vertexShaderGB;
    iGfxPixelShader   pixelShaderGB;
    iGfxSurfaceShader surfaceShaderGB;
    iGfxPipelineState pipelineStateGB;
    iGfxDrawQueue     drawQueueGB;

    // gbuffer
    iGfxTexture      rtColorBuffer0GB;
    iGfxTexture      rtColorBuffer1GB;
    iGfxTexture      rtDepthBufferGB;
    iGfxRenderTarget renderTargetGB;

    // deferred resolve data
    iGfxVertexShader  vertexShaderDS;
    iGfxPixelShader   pixelShaderDS;
    iGfxSurfaceShader surfaceShaderDS;
    iGfxPipelineState pipelineStateDS;
    iGfxDrawQueue     drawQueueDS;

    iGfxTexture      rtBackBufferDS;
    iGfxRenderTarget renderTargetDS;

public:

    inline DeferredScene() {}
    inline ~DeferredScene()
    {
    }

    void load(uint32_t width, uint32_t height, Application* app)
    {
        akMesh.load("data/ak/AKS74U1.mesh");

        // create sampler state
        sgfx::SamplerStateDescriptor samplerDesc;
        samplerDesc.filter = sgfx::TextureFilter::MinMagMip_Linear;
        samplerDesc.addressU = sgfx::AddressMode::Clamp;
        samplerDesc.addressV = sgfx::AddressMode::Clamp;
        samplerDesc.addressW = sgfx::AddressMode::Clamp;
        samplerDesc.lodBias = 0.0F;
        samplerDesc.maxAnisotropy = 1;
        samplerDesc.comparisonFunc = sgfx::ComparisonFunc::Never;
        samplerDesc.borderColor = 0xFFFFFFFF;
        samplerDesc.minLod = -3.402823466e+38F;
        samplerDesc.maxLod = 3.402823466e+38F;

        samplerState = sgfx::createSamplerState(samplerDesc);

        // create vertex format
        const size_t stride1 = 0;                             // Position
        const size_t stride2 = stride1 + 3 * sizeof(float);   // uv0
        const size_t stride3 = stride2 + 2 * sizeof(float);   // uv1
        const size_t stride4 = stride3 + 2 * sizeof(float);   // normal
        const size_t stride5 = stride4 + 3 * sizeof(float);   // boneIDs
        const size_t stride6 = stride5 + 4 * sizeof(uint8_t); // boneWeights
        const size_t stride7 = stride6 + 4 * sizeof(float);   // color
        sgfx::VertexElementDescriptor vfElements[] = {
            { "POSITION",    0, sgfx::DataFormat::RGB32F,  0, stride1, sgfx::VertexElementType::PerVertex },
            { "TEXCOORDA",   0, sgfx::DataFormat::RG32F,   0, stride2, sgfx::VertexElementType::PerVertex },
            { "TEXCOORDB",   0, sgfx::DataFormat::RG32F,   0, stride3, sgfx::VertexElementType::PerVertex },
            { "NORMAL",      0, sgfx::DataFormat::RGB32F,  0, stride4, sgfx::VertexElementType::PerVertex },
            { "BONEIDS",     0, sgfx::DataFormat::R32U,    0, stride5, sgfx::VertexElementType::PerVertex },
            { "BONEWEIGHTS", 0, sgfx::DataFormat::RGBA32F, 0, stride6, sgfx::VertexElementType::PerVertex },
            { "VCOLOR",      0, sgfx::DataFormat::R32U,    0, stride7, sgfx::VertexElementType::PerVertex }
        };
        size_t vfSize = sizeof(vfElements) / sizeof(sgfx::VertexElementDescriptor);

        vertexFormat = app->loadVF(vfElements, vfSize, "shaders/pbr_gbuffer.hlsl");

        // load shaders
        vertexShaderGB = app->loadVS("shaders/pbr_gbuffer.hlsl");
        pixelShaderGB  = app->loadPS("shaders/pbr_gbuffer.hlsl");

        vertexShaderDS = app->loadVS("shaders/pbr_resolve.hlsl");
        pixelShaderDS  = app->loadPS("shaders/pbr_resolve.hlsl");

        if (vertexShaderGB.valid() && pixelShaderGB.valid()) {
            surfaceShaderGB = sgfx::linkSurfaceShader(
                vertexShaderGB,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                pixelShaderGB
            );
        }

        if (vertexShaderDS.valid() && pixelShaderDS.valid()) {
            surfaceShaderDS = sgfx::linkSurfaceShader(
                vertexShaderDS,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                pixelShaderDS
            );
        }

        // create draw queues
        if (surfaceShaderGB.valid()) { // gbuffer draw queue
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection = sgfx::CounterDirection::CW;

            desc.blendState.blendDesc.blendEnabled = false;
            desc.blendState.blendDesc.writeMask = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha = sgfx::BlendOp::Add;

            desc.depthStencilState.depthEnabled = true;
            desc.depthStencilState.writeMask = sgfx::DepthWriteMask::All;
            desc.depthStencilState.depthFunc = sgfx::DepthFunc::Less;

            desc.depthStencilState.stencilEnabled = false;
            desc.depthStencilState.stencilRef = 0;
            desc.depthStencilState.stencilReadMask = 0;
            desc.depthStencilState.stencilWriteMask = 0;

            desc.depthStencilState.frontFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.frontFaceStencilDesc.failOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.passOp = sgfx::StencilOp::Keep;

            desc.depthStencilState.backFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.backFaceStencilDesc.failOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.passOp = sgfx::StencilOp::Keep;

            desc.shader       = surfaceShaderGB;
            desc.vertexFormat = vertexFormat;

            pipelineStateGB = sgfx::createPipelineState(desc);
            if (pipelineStateGB.valid()) {
                drawQueueGB = sgfx::createDrawQueue(pipelineStateGB);
            } else {
                OutputDebugString("Failed to create pipeline state!");
            }
        }

        if (surfaceShaderDS.valid()) { // resolve draw queue
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection = sgfx::CounterDirection::CW;

            desc.blendState.blendDesc.blendEnabled = false;
            desc.blendState.blendDesc.writeMask = sgfx::ColorWriteMask::All;
            desc.blendState.blendDesc.srcBlend = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha = sgfx::BlendOp::Add;

            desc.depthStencilState.depthEnabled = true;
            desc.depthStencilState.writeMask = sgfx::DepthWriteMask::All;
            desc.depthStencilState.depthFunc = sgfx::DepthFunc::Less;

            desc.depthStencilState.stencilEnabled = false;
            desc.depthStencilState.stencilRef = 0;
            desc.depthStencilState.stencilReadMask = 0;
            desc.depthStencilState.stencilWriteMask = 0;

            desc.depthStencilState.frontFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.frontFaceStencilDesc.failOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.passOp = sgfx::StencilOp::Keep;

            desc.depthStencilState.backFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.backFaceStencilDesc.failOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.passOp = sgfx::StencilOp::Keep;

            desc.shader = surfaceShaderDS;
            desc.vertexFormat = sgfx::VertexFormatHandle::invalidHandle();

            pipelineStateDS = sgfx::createPipelineState(desc);
            if (pipelineStateDS.valid()) {
                drawQueueDS = sgfx::createDrawQueue(pipelineStateDS);
            }
            else {
                OutputDebugString("Failed to create pipeline state!");
            }
        }

        // create constant buffer
        constantBuffer = sgfx::createConstantBuffer(nullptr, sizeof(ConstantBuffer));

        // create gbuffer
        rtColorBuffer0GB = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::RGBA8, 1, sgfx::TextureFlags::RenderTarget
        );
        rtColorBuffer1GB = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::RGBA8, 1, sgfx::TextureFlags::RenderTarget
        );
        rtDepthBufferGB = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::D32F, 1, sgfx::TextureFlags::DepthStencil
        );

        sgfx::RenderTargetDescriptor rtDescDS;
        rtDescDS.colorTextures[0] = rtColorBuffer0GB;
        rtDescDS.colorTextures[1] = rtColorBuffer1GB;
        rtDescDS.depthStencilTexture = rtDepthBufferGB;
        rtDescDS.numColorTextures = 2;

        renderTargetGB = sgfx::createRenderTarget(rtDescDS);

        // create render target
        rtBackBufferDS = sgfx::getBackBuffer();

        sgfx::RenderTargetDescriptor renderTargetDesc;
        renderTargetDesc.numColorTextures = 1;
        renderTargetDesc.colorTextures[0] = rtBackBufferDS;

        renderTargetDS = sgfx::createRenderTarget(renderTargetDesc);
    }

    void render(uint32_t width, uint32_t height)
    {
        // update our time
        static float t = 0.0f;

        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;
        t = (dwTimeCur - dwTimeStart) / 1000.0f;

        // fill gbuffer
        sgfx::clearRenderTarget(renderTargetGB, 0x00000000);
        sgfx::clearDepthStencil(renderTargetGB, 1.0F, 0);
        sgfx::setRenderTarget(renderTargetGB);
        sgfx::setViewport(width, height, 0.0F, 1.0F);
        {
            glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.01f, 100.0f);
            glm::mat4 view = glm::lookAt(glm::vec3(0.0F, 1.0F, -25.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
            glm::mat4 world = glm::rotate(glm::mat4(1.0F), t, glm::vec3(0.0F, 1.0F, 0.0F));

            glm::mat4 mvp = projection * view * world;

            ConstantBuffer constants;
            std::memcpy(constants.mvp, glm::value_ptr(mvp), sizeof(constants.mvp));
            sgfx::updateConstantBuffer(constantBuffer, &constants);

            // draw our scene
            {
                sgfx::setPrimitiveTopology(drawQueueGB, sgfx::PrimitiveTopology::TriangleList);
                sgfx::setConstantBuffer(drawQueueGB, 0, constantBuffer);
                sgfx::setVertexBuffer(drawQueueGB, akMesh.getVertexBuffer());
                sgfx::setIndexBuffer(drawQueueGB, akMesh.getIndexBuffer());
                sgfx::drawIndexed(drawQueueGB, akMesh.getNumIndices(), 0, 0);

                sgfx::submit(drawQueueGB);
            }
        }

        // resolve gbuffer
        sgfx::clearRenderTarget(renderTargetDS, 0xFFFFFFF);
        sgfx::setRenderTarget(renderTargetDS);
        sgfx::setViewport(width, height, 0.0F, 1.0F);
        {
            sgfx::setSamplerState(drawQueueDS, 0, samplerState);

            // resolve gbuffer
            {
                sgfx::setPrimitiveTopology(drawQueueDS, sgfx::PrimitiveTopology::TriangleList);
                sgfx::setResource(drawQueueDS, 0, rtColorBuffer0GB);
                sgfx::setResource(drawQueueDS, 1, rtColorBuffer1GB);
                sgfx::setResource(drawQueueDS, 2, rtDepthBufferGB);
                sgfx::draw(drawQueueDS, 3, 0);

                sgfx::submit(drawQueueDS);
            }
        }

        // present frame
        sgfx::present();
    }
};

class CubeApplication : public Application
{
public:
    DeferredScene* scene;

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

        scene = new DeferredScene;
        scene->load(width, height, this);
    }

    virtual void releaseSampleData() override
    {
        OutputDebugString("Cleanup\n");
        delete scene;
        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        scene->render(width, height);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new CubeApplication;
}

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
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <WildMagic/wm_bspline.hh>

// this sample is shamelessly based on WildMagics SamplePhysics/FreeFormDeformation sample

class FFDApplication : public Application
{
public:

    struct ConstantBuffer
    {
        float mvp[16];
    };

    sgfx::VertexShaderHandle  vsHandle;
    sgfx::PixelShaderHandle   psHandle;
    sgfx::SurfaceShaderHandle ssHandle;

    MeshData                                    meshData;
    std::unique_ptr<WM::BSplineVolume<float>>   spline;
    std::vector<glm::vec3>                      splineParameters;
    glm::vec3                                   vmin = glm::vec3(FLT_MAX);
    glm::vec3                                   vmax = glm::vec3(-FLT_MAX);

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

    enum 
    {
        kQuantity = 4,
        kDegree   = 3
    };

    void randomizeSpline()
    {
        glm::vec3 range = vmax - vmin;
        glm::vec3 D     = range / float(kQuantity - 1);

        glm::vec3 controlPoint;
        srand(unsigned int(time(nullptr)));
        for (int i = 0; i < kQuantity; ++i) {
            controlPoint.x = vmin.x + D.x * i;
            for (int j = 0; j < kQuantity; ++j) {
                controlPoint.y = vmin.y + D.y * j;
                for (int k = 0; k < kQuantity; ++k) {
                    controlPoint.z = vmin.z + D.z * k;

                    glm::vec4 oldControlPoint = glm::vec4(spline->GetControlPoint(i, j, k), 1.0F);
                    glm::vec3 newControlPoint = glm::rotateX(oldControlPoint, float(j));

                    spline->SetControlPoint(i, j, k, newControlPoint);
                }
            }
        }
    }

    void updateModel()
    {
        for (size_t i = 0; i < meshData.getVertices().size(); ++i) {
            meshData.getVertices()[i].position = spline->GetPosition(splineParameters[i]);
        }

        sgfx::releaseBuffer(modelVertexBuffer);
        sgfx::releaseBuffer(modelIndexBuffer);

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
    }

    virtual void loadSampleData() override
    {
        setWindowTitle("FreeFormDeformation - press space to randomly deform the mesh");

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

        // load mesh and generate spline
        {
            //meshData.read("data/meshes/dragon/dragon1.mesh");
            meshData.read("data/meshes/connector020.mesh");

            // create bspline volume
            spline = std::make_unique<WM::BSplineVolume<float>>(kQuantity, kQuantity, kQuantity,
                kDegree, kDegree, kDegree);

            std::vector<MeshData::Index>&   indices     = meshData.getIndices();
            std::vector<MeshData::Vertex>&  vertices    = meshData.getVertices();

            for (size_t i = 0; i < vertices.size(); ++i) {

                vmin = glm::min(vmin, vertices[i].position);
                vmax = glm::max(vmax, vertices[i].position);
            }

            // tile mesh
            {
                std::vector<MeshData::Vertex>   newVertices = vertices;
                std::vector<MeshData::Index>    newIndices  = indices;

                for (auto& vertex: newVertices) {
                    vertex.position.y -= vmax.y;
                }

                for (auto& index: newIndices) {
                    index += MeshData::Index(vertices.size());
                }

                vertices.resize(vertices.size() * 2);
                indices.resize(indices.size() * 2);

                std::memcpy(vertices.data() + newVertices.size(), newVertices.data(), newVertices.size() * sizeof(MeshData::Vertex));
                std::memcpy(indices.data() + newIndices.size(), newIndices.data(), newIndices.size() * sizeof(MeshData::Index));

                vmin.y -= vmax.y;
            }

            // tile mesh again
            {
                std::vector<MeshData::Vertex>   newVertices = vertices;
                std::vector<MeshData::Index>    newIndices  = indices;

                for (auto& vertex: newVertices) {
                    vertex.position.x -= vmax.x;
                }

                for (auto& index: newIndices) {
                    index += MeshData::Index(vertices.size());
                }

                vertices.resize(vertices.size() * 2);
                indices.resize(indices.size() * 2);

                std::memcpy(vertices.data() + newVertices.size(), newVertices.data(), newVertices.size() * sizeof(MeshData::Vertex));
                std::memcpy(indices.data() + newIndices.size(), newIndices.data(), newIndices.size() * sizeof(MeshData::Index));

                vmin.x -= vmax.x;
            }

            // generate control points
            glm::vec3 range = vmax - vmin;
            glm::vec3 D     = range / float(kQuantity - 1);

            glm::vec3 controlPoint;
            for (int i = 0; i < kQuantity; ++i) {
                controlPoint.x = vmin.x + D.x * i;
                for (int j = 0; j < kQuantity; ++j) {
                    controlPoint.y = vmin.y + D.y * j;
                    for (int k = 0; k < kQuantity; ++k) {
                        controlPoint.z = vmin.z + D.z * k;

                        spline->SetControlPoint(i, j, k, controlPoint);
                    }
                }
            }

            // compute UVW values
            glm::vec3 invRange = 1.0F / range;
            splineParameters.resize(vertices.size());
            
            for (size_t i = 0; i < splineParameters.size(); ++i) {
                splineParameters[i] = (vertices[i].position - vmin) * invRange;
            }

            // update mesh data
            updateModel();
        }

        vsHandle = loadVS("shaders/oit_simple.hlsl");
        psHandle = loadPS("shaders/oit_simple.hlsl");

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
        // randomize mesh if necessary
        static bool doRandomize = false;
        if (GetAsyncKeyState(VK_SPACE)) {
            randomizeSpline();
            updateModel();

            Sleep(500);
        }

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

        sgfx::clearRenderTarget(renderTarget, 0xFF000000);
        sgfx::clearDepthStencil(renderTarget, 1.0F, 0);
        sgfx::setRenderTarget(renderTarget);
        sgfx::setViewport(width, height, 0.0F, 1.0F);

        // draw
        {
            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setVertexBuffer(drawQueue, modelVertexBuffer);
            sgfx::setIndexBuffer(drawQueue, modelIndexBuffer);
            sgfx::drawIndexed(drawQueue, static_cast<uint32_t>(meshData.getIndices().size()), 0, 0);

            sgfx::submit(drawQueue);
        }

        sgfx::present(1);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new FFDApplication;
}

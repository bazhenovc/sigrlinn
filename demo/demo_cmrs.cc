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
#define GLM_FORCE_PURE 1

#include "common/app.hh"

#include <memory>
#include <vector>
#include <array>
#include <stack>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>


// Continuous Multi Resolution Surfaces

template <typename T>
class Surface
{
public:

    using Vertex = T;

    struct Patch
    {
        float       density     = 0.0F;     // quad area
        uint32_t    vertices[4] = { 0 };    // indices to vertices that define this patch
        uint32_t    children[4] = { 0 };    // child patch indices
        uint32_t    midpoint    = 0;
        uint32_t    levels      = 0;
    };

    std::vector<Vertex> vertices;
    std::vector<Patch>  patches;

    void generateSubdivisionPlane(float size, int subdivisions)
    {
        enum
        {
            kReservedVertices   = 1536 * 1024 * 1024,
            kReservedPatches    = 1024 * 1024 * 1024
        };

        vertices.reserve(kReservedVertices / sizeof(T));
        patches.reserve(kReservedPatches / sizeof(Patch));

        Vertex initialVertices[4];
        initialVertices[0].position = {  size, 0.0F,  size };
        initialVertices[1].position = { -size, 0.0F,  size };
        initialVertices[2].position = { -size, 0.0F, -size };
        initialVertices[3].position = {  size, 0.0F, -size };

        Patch initialPatch;
        initialPatch.density        = size * size;
        initialPatch.vertices[0]    = 0;
        initialPatch.vertices[1]    = 1;
        initialPatch.vertices[2]    = 2;
        initialPatch.vertices[3]    = 3;
        initialPatch.levels         = subdivisions;

        patches.push_back(initialPatch);
        vertices.push_back(initialVertices[0]);
        vertices.push_back(initialVertices[1]);
        vertices.push_back(initialVertices[2]);
        vertices.push_back(initialVertices[3]);

        std::stack<uint32_t> patchesStack;
        patchesStack.push(0);

        while (!patchesStack.empty()) {

            uint32_t parentID = patchesStack.top();
            Patch parentPatch = patches[parentID];
            patchesStack.pop();

            std::array<Vertex, 5>   edges;
            std::array<Vertex, 5>   points;
            std::array<uint32_t, 5> indices;

            edges[0].position = vertices[parentPatch.vertices[1]].position - vertices[parentPatch.vertices[0]].position;
            edges[1].position = vertices[parentPatch.vertices[2]].position - vertices[parentPatch.vertices[3]].position;
            edges[2].position = vertices[parentPatch.vertices[3]].position - vertices[parentPatch.vertices[0]].position;
            edges[3].position = vertices[parentPatch.vertices[2]].position - vertices[parentPatch.vertices[1]].position;

            points[0].position = edges[0].position * 0.5f + vertices[parentPatch.vertices[0]].position;
            points[1].position = edges[1].position * 0.5f + vertices[parentPatch.vertices[3]].position;
            points[2].position = edges[2].position * 0.5f + vertices[parentPatch.vertices[0]].position;
            points[3].position = edges[3].position * 0.5f + vertices[parentPatch.vertices[1]].position;

            edges[4].position = points[1].position - points[0].position;
            points[4].position = edges[4].position * 0.5f + points[0].position;

            for (int i = 0; i < 5; ++i) {
                indices[i] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(points[i]);
            }

            assert(vertices.size() <= std::numeric_limits<uint32_t>::max() && "Fatal: out of bounds!");
            assert(patches.size() <= std::numeric_limits<uint32_t>::max() && "Fatal: out of bounds!");

            patches[parentID].midpoint = indices[4];

            std::array<uint32_t, 16> newPatchVertices = {
                parentPatch.vertices[0], indices[0], indices[4], indices[2],
                indices[0], parentPatch.vertices[1], indices[3], indices[4],
                indices[4], indices[3], parentPatch.vertices[2], indices[1],
                indices[2], indices[4], indices[1], parentPatch.vertices[3]
            };

            for (int i = 0; i < 4; ++i) {
                Patch newPatch;
                newPatch.density        = patches[parentID].density * 0.25F;
                newPatch.vertices[0]    = newPatchVertices[i * 4 + 0];
                newPatch.vertices[1]    = newPatchVertices[i * 4 + 1];
                newPatch.vertices[2]    = newPatchVertices[i * 4 + 2];
                newPatch.vertices[3]    = newPatchVertices[i * 4 + 3];
                newPatch.levels         = patches[parentID].levels - 1;

                uint32_t patchID = static_cast<uint32_t>(patches.size());
                patches.push_back(newPatch);
                patches[parentID].children[i] = patchID;

                if (newPatch.levels > 0)
                    patchesStack.push(patchID);
            }
        }
    }

    void generateSineWave()
    {
        for (size_t i = 0; i < vertices.size(); ++i) {
            vertices[i].position.y = std::max(sin(vertices[i].position.z), sin(vertices[i].position.x)) * 1.3F;
        }
    }

    void generateContours(glm::vec3 cameraPos, float densityFactor, std::vector<Patch>& outPatches) const
    {
        std::stack<uint32_t> patchStack;
        patchStack.push(0);

        while (!patchStack.empty()) {
            uint32_t id = patchStack.top();
            patchStack.pop();

            const Patch& patch = patches[id];
            float factor = glm::distance2(cameraPos, vertices[patch.midpoint].position) / patch.density;

            if (factor >= densityFactor || patch.levels == 0) { // found the match
                outPatches.push_back(patch);
            } else { // not found, recurse deeper
                 patchStack.push(patch.children[0]);
                 patchStack.push(patch.children[1]);
                 patchStack.push(patch.children[2]);
                 patchStack.push(patch.children[3]);
            }
        }
    }

    void tessellate(glm::vec3 cameraPos, float density, std::vector<uint32_t>& outIndexBuffer) const
    {
        std::vector<Patch> patches;
        generateContours(cameraPos, density, patches);

        for (Patch patch: patches) {
            outIndexBuffer.push_back(patch.vertices[0]);
            outIndexBuffer.push_back(patch.vertices[1]);
            outIndexBuffer.push_back(patch.vertices[2]);

            outIndexBuffer.push_back(patch.vertices[0]);
            outIndexBuffer.push_back(patch.vertices[2]);
            outIndexBuffer.push_back(patch.vertices[3]);
        }
    }
};

class CMRSApplication : public Application
{
public:

    struct CommonVertex
    {
        enum { MaxBones = 4 };
        glm::vec3   position;
        glm::vec3   normal;
    };

    struct ConstantBuffer
    {
        float mvp[16];
    };

    sgfx::VertexShaderHandle    vsHandle;
    sgfx::PixelShaderHandle     psHandle;
    sgfx::SurfaceShaderHandle   ssHandle;

    sgfx::BufferHandle          vertexBuffer;
    sgfx::BufferHandle          indexBuffer;
    sgfx::ConstantBufferHandle  constantBuffer;
    sgfx::VertexFormatHandle    vertexFormat;

    sgfx::PipelineStateHandle   pipelineState;
    sgfx::DrawQueueHandle       drawQueue;

    sgfx::Texture2DHandle       colorBuffer;
    sgfx::Texture2DHandle       depthStencilBuffer;
    sgfx::RenderTargetHandle    renderTarget;

    std::vector<uint32_t>       surfaceIndexBuffer;
    size_t                      surfaceIndexBufferSize = 0;
    Surface<CommonVertex>       surface;

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

        // constant buffer
        constantBuffer = sgfx::createConstantBuffer(nullptr, sizeof(ConstantBuffer));

        // mesh data
        sgfx::VertexElementDescriptor vfElements[] = {
            { "POSITION",    0, sgfx::DataFormat::RGB32F,  0, 0 * sizeof(float) },
            { "NORMAL",      0, sgfx::DataFormat::RGB32F,  0, 3 * sizeof(float) },
        };
        size_t vfSize = sizeof(vfElements) / sizeof(sgfx::VertexElementDescriptor);

        vertexFormat = loadVF(vfElements, vfSize, "shaders/cmrs.hlsl");

        surface.generateSubdivisionPlane(256.0F, 12);
        surface.generateSineWave();

        vertexBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::VertexBuffer,
            surface.vertices.data(), sizeof(CommonVertex) * surface.vertices.size(),
            sizeof(CommonVertex));

        //indexBuffer  = sgfx::createBuffer(sgfx::BufferFlags::IndexBuffer, cubeIndices, sizeof(uint32_t) * indicesSize, sizeof(uint32_t));

        vsHandle = loadVS("shaders/cmrs.hlsl");
        psHandle = loadPS("shaders/cmrs.hlsl");

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

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Wireframe;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CCW;

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
        sgfx::releaseBuffer(vertexBuffer);
        sgfx::releaseBuffer(indexBuffer);
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
        dwTimeStart = dwTimeCur;

        static glm::vec3 cameraPosition = glm::vec3(0.0F, 2.5F, -130);

        static float cameraAngle = 0.0F;

        float cameraSpeed = 2.0F;
        if (GetAsyncKeyState(VK_SHIFT)) cameraSpeed = 10.0F;

        if (GetAsyncKeyState(VK_CONTROL)) {
            if (GetAsyncKeyState(VK_LEFT))      cameraAngle += t * 0.5F;
            if (GetAsyncKeyState(VK_RIGHT))     cameraAngle -= t * 0.5F;
        } else {
            if (GetAsyncKeyState(VK_UP))        cameraPosition.z += t * cameraSpeed;
            if (GetAsyncKeyState(VK_DOWN))      cameraPosition.z -= t * cameraSpeed;
            if (GetAsyncKeyState(VK_LEFT))      cameraPosition.x += t * cameraSpeed;
            if (GetAsyncKeyState(VK_RIGHT))     cameraPosition.x -= t * cameraSpeed;
            if (GetAsyncKeyState(VK_HOME))      cameraPosition.y += t * cameraSpeed;
            if (GetAsyncKeyState(VK_END))       cameraPosition.y -= t * cameraSpeed;
        }

        static float distFactor = 100.0F;
        if (GetAsyncKeyState(VK_INSERT)) distFactor += 1.0F;
        if (GetAsyncKeyState(VK_DELETE)) distFactor -= 1.0F;

        glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.1f, 50000.0f);
        glm::mat4 view       = glm::lookAt(cameraPosition, cameraPosition + glm::vec3(sin(cameraAngle), -1.0F, cos(cameraAngle)), glm::vec3(0.0F, 1.0F, 0.0F));
        //glm::mat4 world      = glm::rotate(glm::mat4(1.0F), 0.0F, glm::vec3(0.0F, 1.0F, 0.0F));

        glm::mat4 mvp = projection * view;// * world;

        ConstantBuffer constants;
        std::memcpy(constants.mvp, glm::value_ptr(mvp), sizeof(constants.mvp));
        sgfx::updateConstantBuffer(constantBuffer, &constants);

        // tessellate shape and fill IB
        //if (surfaceIndexBufferSize == 0)
        {
            surfaceIndexBuffer.clear();
            surface.tessellate(cameraPosition, distFactor, surfaceIndexBuffer);

            // recreate buffer if needed
            if (surfaceIndexBuffer.size() > surfaceIndexBufferSize) {
                sgfx::releaseBuffer(indexBuffer);
                surfaceIndexBufferSize = surfaceIndexBuffer.size();

                indexBuffer = sgfx::createBuffer(
                    sgfx::BufferFlags::IndexBuffer | sgfx::BufferFlags::CPUWrite,
                    nullptr, sizeof(uint32_t) * surfaceIndexBufferSize,
                    sizeof(uint32_t));
            }

            // map and update
            void* data = sgfx::mapBuffer(indexBuffer, sgfx::MapType::Write);
            if (data != nullptr) {
                std::memcpy(data, surfaceIndexBuffer.data(), surfaceIndexBuffer.size() * sizeof(uint32_t));

                sgfx::unmapBuffer(indexBuffer);
            }

            // display stats
            std::ostringstream oss;
            oss << "Vertices: " << surfaceIndexBuffer.size() << " / " << surface.vertices.size();
            setWindowTitle(oss.str().c_str());
        }

        // actually draw some stuff
        {
            sgfx::clearRenderTarget(renderTarget, 0xFFFFFFF);
            sgfx::clearDepthStencil(renderTarget, 1.0F, 0);
            sgfx::setRenderTarget(renderTarget);
            sgfx::setViewport(width, height, 0.0F, 1.0F);

            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
            sgfx::setVertexBuffer(drawQueue, vertexBuffer);
            sgfx::setIndexBuffer(drawQueue, indexBuffer);
            sgfx::drawIndexed(drawQueue, static_cast<uint32_t>(surfaceIndexBuffer.size()), 0, 0);

            sgfx::submit(drawQueue);
        }
        sgfx::present(1);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new CMRSApplication;
}


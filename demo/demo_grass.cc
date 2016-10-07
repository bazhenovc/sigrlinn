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
#include "common/textureloader.hh"

#include <vector>
#include <memory>
#include <string>
#include <stack>
#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

bool listFiles(std::string path, std::string mask, std::vector<std::string>& files) {
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA ffd;
    std::string spec;
    std::stack<std::string> directories;

    directories.push(path);
    files.clear();

    while (!directories.empty()) {
        path = directories.top();
        spec = path + "/" + mask;
        directories.pop();

        hFind = FindFirstFileA(spec.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            return false;
        }

        do {
            if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    directories.push(path + "/" + ffd.cFileName);
                } else {
                    files.push_back(path + "/" + ffd.cFileName);
                }
            }
        } while (FindNextFileA(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            FindClose(hFind);
            return false;
        }

        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return true;
}

struct BoundingBox final
{
    glm::vec4 min;
    glm::vec4 max;
};

struct LogicalMeshBuffer final
{
    uint8_t* data             = nullptr;
    uint32_t dataSize         = 0;
    uint32_t dataFormatStride = 0;
    uint32_t physicalAddress  = 0;
};

struct LogicalMesh final
{
    LogicalMeshBuffer indexBuffer;
    LogicalMeshBuffer vertexBuffer;
    uint32_t          count;

    LogicalMesh() {}
    LogicalMesh(MeshData* data)
    {
        vertexBuffer.data             = reinterpret_cast<uint8_t*>(data->getVertices().data());
        vertexBuffer.dataFormatStride = sizeof(MeshData::Vertex);
        vertexBuffer.dataSize         = static_cast<uint32_t>(data->getVertices().size() * sizeof(MeshData::Vertex));

        indexBuffer.data              = reinterpret_cast<uint8_t*>(data->getIndices().data());
        indexBuffer.dataFormatStride  = sizeof(MeshData::Index);
        indexBuffer.dataSize          = static_cast<uint32_t>(data->getIndices().size() * sizeof(MeshData::Index));

        count = static_cast<uint32_t>(data->getIndices().size());
    }
};

struct PhysicalMeshBuffer final
{
    util::BufferHandle physicalBuffer;
    size_t             physicalDataSize;
    bool               isDirty = false;

    typedef std::vector<LogicalMeshBuffer*> PageArray;
    PageArray allPages;

    inline void allocate(LogicalMeshBuffer* logicalBuffer)
    {
        allPages.push_back(logicalBuffer);
        isDirty = true;
    }

    inline void rebuildPages()
    {
        if (!isDirty) return;

        size_t vfStride = allPages[0]->dataFormatStride;
        physicalDataSize = 0;
        for (LogicalMeshBuffer* logicalBuffer: allPages) // calculate total size
            physicalDataSize += logicalBuffer->dataSize; // TODO: find a better place for this

        size_t numElements = physicalDataSize / vfStride;

        sgfx::releaseBuffer(physicalBuffer);
        physicalBuffer = sgfx::createBuffer(
            sgfx::BufferFlags::CPUWrite | sgfx::BufferFlags::StructuredBuffer,
            nullptr,
            physicalDataSize,
            vfStride
        );

        uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sgfx::mapBuffer(physicalBuffer, sgfx::MapType::Write));
        if (dataPtr != nullptr) {
            uint32_t pageOffset = 0;
            for (size_t i = 0; i < allPages.size(); ++i) {
                LogicalMeshBuffer* logicalBuffer = allPages[i];
                // copy logical data to the mapped physical data
                std::memcpy(dataPtr + pageOffset, logicalBuffer->data, logicalBuffer->dataSize);
                // calculate physical address
                logicalBuffer->physicalAddress = pageOffset / logicalBuffer->dataFormatStride;
                // calculate offset
                pageOffset += logicalBuffer->dataSize;
            }

            sgfx::unmapBuffer(physicalBuffer);
        }

        isDirty = false;
    }
};

struct GrassObject final
{
    struct DVPBuffer
    {
        uint32_t    drawCallData[4];
        float       modelview[16];
    };

    enum
    {
        kDVPBufferSize = sizeof(DVPBuffer)
    };
    static_assert((kDVPBufferSize % 16) == 0, "Error: wrong buffer size");

    glm::vec3           position;
    BoundingBox         boundingBox;

    LogicalMeshBuffer*  vertexBuffer;
    LogicalMeshBuffer*  indexBuffer;

    uint32_t            count;

    GrassObject() {}

    GrassObject(LogicalMesh* data)
    {
        vertexBuffer = &data->vertexBuffer;
        indexBuffer  = &data->indexBuffer;
        count        = data->count;

        buildBoundingBox();
    }

    ~GrassObject()
    {}

    void buildBoundingBox()
    {
        glm::vec4 min = glm::vec4(0, 0, 0, 1);
        glm::vec4 max = glm::vec4(0, 0, 0, 1);

        MeshData::Vertex* vertices = reinterpret_cast<MeshData::Vertex*>(vertexBuffer->data);
        for (size_t i = 0; i < (vertexBuffer->dataSize / vertexBuffer->dataFormatStride); ++i) {
            min.x = std::min(min.x, vertices[i].position[0]);
            min.y = std::min(min.y, vertices[i].position[1]);
            min.z = std::min(min.z, vertices[i].position[2]);

            max.x = std::max(max.x, vertices[i].position[0]);
            max.y = std::max(max.y, vertices[i].position[1]);
            max.z = std::max(max.z, vertices[i].position[2]);
        }

        boundingBox.min = min;
        boundingBox.max = max;
    }
};

struct DVPGrassManager final
{
    std::vector<MeshData*>      loadedMeshes;
    std::vector<LogicalMesh*>   logicalMeshes;
    std::vector<GrassObject>    grassObjects;
    PhysicalMeshBuffer          physicalVertexBuffer;
    PhysicalMeshBuffer          physicalIndexBuffer;

    util::SamplerStateHandle    samplerState;
    util::TextureHandle         grassTexture;

    util::BufferHandle          initialInstanceBuffer;
    util::BufferHandle          finalInstanceBuffer;
    util::BufferHandle          occlusionDataBuffer;
    util::BufferHandle          indirectRenderBuffer;
    util::BufferHandle          indirectRenderBufferCPU;
    
    uint32_t                    numInstances = 0;
    uint32_t                    maxDrawCallCount = 0;

    struct CullCSConstantBuffer
    {
        float       numItems;
        uint32_t    maxDrawCallCount;
        uint32_t    reserved[2];
    } cullCSConstantData;
    util::ConstantBufferHandle cullCSConstantBuffer;

    struct RenderConstantBuffer
    {
        float       viewProjection[16];
    } renderConstantData;
    util::ConstantBufferHandle renderConstantBuffer;

    enum
    {
        kObjectSizeSquared = 128
    };

    DVPGrassManager()
    {
        sgfx::SamplerStateDescriptor samplerDesc;
        samplerDesc.filter         = sgfx::TextureFilter::MinMagMip_Linear;
        samplerDesc.addressU       = sgfx::AddressMode::Clamp;
        samplerDesc.addressV       = sgfx::AddressMode::Clamp;
        samplerDesc.addressW       = sgfx::AddressMode::Clamp;
        samplerDesc.lodBias        = 0.0F;
        samplerDesc.maxAnisotropy  = 1;
        samplerDesc.comparisonFunc = sgfx::ComparisonFunc::Never;
        samplerDesc.borderColor    = 0xFFFFFFFF;
        samplerDesc.minLod         = -3.402823466e+38F;
        samplerDesc.maxLod         = 3.402823466e+38F;

        samplerState = sgfx::createSamplerState(samplerDesc);
    }

    ~DVPGrassManager()
    {
        for (MeshData* data: loadedMeshes)
            delete data;
        for (LogicalMesh* mesh: logicalMeshes) {
            delete mesh; // TODO: release
        }
    }

    void loadTexture(const std::string& path)
    {
        grassTexture = loadDDS(path);
    }

    void loadDataSet(const std::string& path)
    {
        std::vector<std::string> files;
        if (listFiles(path, "*.mesh", files)) {

            for (const std::string& file: files) {
                MeshData* data = new MeshData;
                data->read(file);
                loadedMeshes.push_back(data);

                LogicalMesh* logicalMesh = new LogicalMesh(data);
                physicalVertexBuffer.allocate(&logicalMesh->vertexBuffer);
                physicalIndexBuffer.allocate(&logicalMesh->indexBuffer);
                logicalMeshes.push_back(logicalMesh);
            }

            srand(GetTickCount());
            size_t meshCounter = 0;
            for (int i = -kObjectSizeSquared; i < kObjectSizeSquared; ++i) {
                for (int j = -kObjectSizeSquared; j < kObjectSizeSquared; ++j) {

                    float rx = static_cast<float>(rand() % 10);
                    float ry = static_cast<float>(rand() % 10);

                    float fx = static_cast<float>(i) * 3.0F + rx;
                    float fy = static_cast<float>(j) * 3.0F + ry;

                    GrassObject obj(logicalMeshes[meshCounter]);
                    obj.position = glm::vec3(fx, 0.0F, fy);
                    grassObjects.push_back(obj);

                    meshCounter++;
                    if (meshCounter == logicalMeshes.size())
                        meshCounter = 0;
                }
            }
        }
    }

    void render(sgfx::DrawQueueHandle drawQueue, sgfx::DrawQueueHandle occlusionQueue, sgfx::ComputeQueueHandle computeQueue, const glm::mat4& mvp)
    {
        physicalVertexBuffer.rebuildPages();
        physicalIndexBuffer.rebuildPages();

        // rebuild buffers if needed
        if (grassObjects.size() != numInstances) {
            numInstances = static_cast<uint32_t>(grassObjects.size());

            initialInstanceBuffer = sgfx::createBuffer(
                sgfx::BufferFlags::CPUWrite | sgfx::BufferFlags::StructuredBuffer,
                nullptr,
                numInstances * GrassObject::kDVPBufferSize,
                GrassObject::kDVPBufferSize
            );

            finalInstanceBuffer = sgfx::createBuffer(
                sgfx::BufferFlags::GPUWrite | sgfx::BufferFlags::StructuredBuffer | sgfx::BufferFlags::GPUCounter,
                nullptr,
                numInstances * GrassObject::kDVPBufferSize,
                GrassObject::kDVPBufferSize
            );

            occlusionDataBuffer = sgfx::createBuffer(
                sgfx::BufferFlags::GPUWrite | sgfx::BufferFlags::StructuredBuffer,
                nullptr,
                numInstances * sizeof(uint32_t),
                sizeof(uint32_t)
            );

            indirectRenderBuffer = sgfx::createBuffer(
                sgfx::BufferFlags::GPUWrite | sgfx::BufferFlags::GPUCounter | sgfx::BufferFlags::IndirectArgs,
                nullptr,
                4 * sizeof(uint32_t),
                4 * sizeof(uint32_t)
            );

            indirectRenderBufferCPU = sgfx::createBuffer(
                sgfx::BufferFlags::CPURead,
                nullptr,
                4 * sizeof(uint32_t),
                4 * sizeof(uint32_t)
            );

            cullCSConstantBuffer = sgfx::createConstantBuffer(&cullCSConstantData, sizeof(CullCSConstantBuffer));
            renderConstantBuffer = sgfx::createConstantBuffer(&renderConstantData, sizeof(renderConstantData));

            // update data
            uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sgfx::mapBuffer(initialInstanceBuffer, sgfx::MapType::Write));
            if (dataPtr != nullptr) {

                for (size_t i = 0; i < numInstances; ++i) {
                    size_t offset = i * GrassObject::kDVPBufferSize;
                    const GrassObject& obj = grassObjects[i];

                    glm::mat4 matrix = glm::translate(obj.position);
                    matrix = glm::transpose(matrix);

                    //glm::vec4 boundingBoxMin = obj.boundingBox.min * matrix;
                    //glm::vec4 boundingBoxMax = obj.boundingBox.max * matrix;

                    GrassObject::DVPBuffer dvpData;
                    // fill internal data structure
                    dvpData.drawCallData[0] = obj.vertexBuffer->physicalAddress;
                    dvpData.drawCallData[1] = obj.indexBuffer->physicalAddress;

                    dvpData.drawCallData[2] = 1; // draw indexed
                    dvpData.drawCallData[3] = obj.count;

                    // copy matrix
                    std::memcpy(dvpData.modelview,      glm::value_ptr(matrix),         sizeof(dvpData.modelview));

                    std::memcpy(dataPtr + offset, &dvpData, GrassObject::kDVPBufferSize);

                    maxDrawCallCount = std::max(maxDrawCallCount, obj.count);
                }

                sgfx::unmapBuffer(initialInstanceBuffer);
            }

            // update const buffer
            cullCSConstantData.numItems         = static_cast<float>(numInstances);
            cullCSConstantData.maxDrawCallCount = maxDrawCallCount;
            sgfx::updateConstantBuffer(cullCSConstantBuffer, &cullCSConstantData);
        }

        // update const buffer
        {
            std::memcpy(renderConstantData.viewProjection, glm::value_ptr(glm::transpose(mvp)), sizeof(renderConstantData.viewProjection));
            sgfx::updateConstantBuffer(renderConstantBuffer, &renderConstantData);
        }

        // culling
        {
            sgfx::beginPerfEvent(L"OcclusionCull");

            sgfx::setConstantBuffer(computeQueue, 0, cullCSConstantBuffer);
            sgfx::setResource(computeQueue, 0, initialInstanceBuffer);
            sgfx::setResource(computeQueue, 1, occlusionDataBuffer);
            sgfx::setResourceRW(computeQueue, 0, finalInstanceBuffer);
            sgfx::setResourceRW(computeQueue, 1, indirectRenderBuffer);

            //uint32_t groupCount = static_cast<uint32_t>(std::ceilf(numInstancesF / 128.0F));
            sgfx::submit(computeQueue, 1, 1, 1);

            sgfx::endPerfEvent();
        }

        // rendering
        sgfx::setSamplerState(drawQueue, 0, samplerState);
        {
            sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::setConstantBuffer(drawQueue, 0, renderConstantBuffer);
            sgfx::setResource(drawQueue, 0, physicalVertexBuffer.physicalBuffer);
            sgfx::setResource(drawQueue, 1, physicalIndexBuffer.physicalBuffer);
            sgfx::setResource(drawQueue, 2, finalInstanceBuffer);
            sgfx::setResource(drawQueue, 3, grassTexture);
            sgfx::drawInstancedIndirect(drawQueue, indirectRenderBuffer, 0);

            // queue submit is done later
            //sgfx::submit(drawQueue);
        }

        // render occluders
        {
            sgfx::setPrimitiveTopology(occlusionQueue, sgfx::PrimitiveTopology::TriangleList);
            sgfx::setConstantBuffer(occlusionQueue, 0, renderConstantBuffer);
            sgfx::setResource(occlusionQueue, 0, physicalVertexBuffer.physicalBuffer);
            sgfx::setResource(occlusionQueue, 1, physicalIndexBuffer.physicalBuffer);
            sgfx::setResource(occlusionQueue, 2, initialInstanceBuffer);
            sgfx::drawInstanced(occlusionQueue, numInstances, maxDrawCallCount, 0, 0);

            // queue submit is done later
            //sgfx::submit(occlusionQueue);
        }
    }

    void displayOcclusionCullingStats(Application* app)
    {
        sgfx::copyResource(indirectRenderBuffer, indirectRenderBufferCPU);

        void* dataPtr = sgfx::mapBuffer(indirectRenderBufferCPU, sgfx::MapType::Read);
        if (dataPtr != nullptr) {
            uint32_t* data = reinterpret_cast<uint32_t*>(dataPtr);

            static uint32_t worstVisible = 0;
            static uint32_t bestVisible  = std::numeric_limits<uint32_t>::max();

            worstVisible = std::max(worstVisible, data[1]);
            bestVisible  = std::min(bestVisible, data[1]);

            std::ostringstream ss;
            ss
                << "Visible objects: " << data[1] << " / " << numInstances
                << " culled: " << numInstances - data[1]
                << " worst: " << worstVisible
                << " best: " << bestVisible
                << "\n";

            //OutputDebugString(ss.str().c_str());
            app->setWindowTitle(ss.str().c_str());

            sgfx::unmapBuffer(indirectRenderBufferCPU);
        }
    }
};

class GrassApplication : public Application
{
public:

    // grass render
    util::VertexShaderHandle    vsDrawHandle;
    util::PixelShaderHandle     psDrawHandle;
    util::SurfaceShaderHandle   ssDrawHandle;

    util::PipelineStateHandle   pipelineState;
    util::DrawQueueHandle       drawQueue;

    util::TextureHandle         colorBuffer;
    util::TextureHandle         depthBuffer;
    util::TextureHandle         occlusionDepthBuffer;
    util::RenderTargetHandle    renderTarget;

    // bounding box render
    util::VertexShaderHandle    vsOcclusionHandle;
    util::PixelShaderHandle     psOcclusionHandle;
    util::SurfaceShaderHandle   ssOcclusionHandle;

    util::PipelineStateHandle   occlusionPipelineState;

    util::DrawQueueHandle       occlusionQueue;
    util::RenderTargetHandle    occlusionRT;

    // CS downscale
    util::ConstantBufferHandle  downscaleConstBuffer;
    util::ComputeShaderHandle   downscaleShader;
    util::ComputeQueueHandle    downscaleQueue;

    // CS culling
    util::ComputeShaderHandle   instanceCullShader;
    util::ComputeQueueHandle    instanceCullQueue;

    DVPGrassManager*            grassManager;

    glm::mat4                   MVP;
    glm::mat4                   prevMVP;

    struct DownscaleConstData
    {
        float rtSize[4];
        float invViewProjection[16];
        float viewProjection[16];
    } downscaleConstData;

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

        // mesh data
        grassManager = new DVPGrassManager;
        grassManager->loadDataSet("data/meshes/cattail/");
        grassManager->loadTexture("data/textures/CattailBlades.dds");

        // create render target
        colorBuffer        = sgfx::getBackBuffer();
        depthBuffer        = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::D32F, 1, sgfx::TextureFlags::DepthStencil
        );
        occlusionDepthBuffer = sgfx::createTexture2D(
            width, height, sgfx::DataFormat::R32F, 1, sgfx::TextureFlags::GPUWrite
        );

        sgfx::RenderTargetDescriptor renderTargetDesc;
        renderTargetDesc.numColorTextures    = 1;
        renderTargetDesc.colorTextures[0]    = colorBuffer;
        renderTargetDesc.depthStencilTexture = depthBuffer;

        renderTarget = sgfx::createRenderTarget(renderTargetDesc);

        sgfx::RenderTargetDescriptor occlusionRTDesc;
        occlusionRTDesc.numColorTextures        = 0;
        occlusionRTDesc.depthStencilTexture     = depthBuffer;

        occlusionRT = sgfx::createRenderTarget(occlusionRTDesc);

        // create shaders
        vsDrawHandle = loadVS("shaders/dvp.hlsl");
        psDrawHandle = loadPS("shaders/dvp.hlsl");

        if (vsDrawHandle.valid() && psDrawHandle.valid()) {
            ssDrawHandle = sgfx::linkSurfaceShader(
                vsDrawHandle,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                psDrawHandle
            );
        }

        Application::ShaderMacroVector occlusionMacros;
        occlusionMacros.push_back({"OCCLUSION_RENDER", "1"});

        vsOcclusionHandle = loadVS("shaders/dvp.hlsl", occlusionMacros);
        psOcclusionHandle = loadPS("shaders/dvp.hlsl", occlusionMacros);

        if (vsOcclusionHandle.valid() && psOcclusionHandle.valid()) {
            ssOcclusionHandle = sgfx::linkSurfaceShader(
                vsOcclusionHandle,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                psOcclusionHandle
            );
        }

        if (ssDrawHandle.valid() && ssOcclusionHandle.valid()) {
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::None;
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

            desc.shader       = ssDrawHandle;
            desc.vertexFormat = sgfx::VertexFormatHandle::invalidHandle();

            pipelineState = sgfx::createPipelineState(desc);
            if (pipelineState.valid()) {
                drawQueue = sgfx::createDrawQueue(pipelineState);
            } else {
                OutputDebugString("Failed to create pipeline state!");
            }

            // occlusion state
            sgfx::PipelineStateDescriptor occlusionDesc = desc;
            occlusionDesc.shader                        = ssOcclusionHandle;
            occlusionDesc.depthStencilState.depthFunc   = sgfx::DepthFunc::LessEqual;
            occlusionDesc.depthStencilState.writeMask   = sgfx::DepthWriteMask::Zero;

            occlusionPipelineState = sgfx::createPipelineState(occlusionDesc);
            if (occlusionPipelineState.valid()) {
                occlusionQueue = sgfx::createDrawQueue(occlusionPipelineState);
            } else {
                OutputDebugString("Failed to create occlusion pipeline state!");
            }
        }

        // compute shaders
        instanceCullShader  = loadCS("shaders/dvp_cull.hlsl");
        instanceCullQueue   = sgfx::createComputeQueue(instanceCullShader);

        downscaleShader     = loadCS("shaders/dvp_downscale.hlsl");
        downscaleQueue      = sgfx::createComputeQueue(downscaleShader);

        downscaleConstBuffer = sgfx::createConstantBuffer(&downscaleConstData, sizeof(DownscaleConstData));
    }

    virtual void releaseSampleData() override
    {
        OutputDebugString("Cleanup\n");

        delete grassManager;

        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        // Update our time
        static float t = 0.0f;
        static glm::vec3 cameraPosition = glm::vec3(0.0F, 2.5F, -200);

        static uint32_t frameCounter = 0;
        frameCounter ++;

        static ULONGLONG dwTimeStart = 0;
        ULONGLONG dwTimeCur = GetTickCount64();
        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;
        t = (dwTimeCur - dwTimeStart) / 1000.0f;
        dwTimeStart = dwTimeCur;

        static bool dtReport = false;
        if (GetAsyncKeyState(VK_F1)) dtReport = !dtReport;

        if (dtReport) {
            char buf[64];
            sprintf_s(buf, "DT: %f\n", t);
            OutputDebugString(buf);
        }

        static float cameraAngle = 0.0F;

        float cameraSpeed = 2.0F;
        if (GetAsyncKeyState(VK_SHIFT)) cameraSpeed = 20.0F;

        if (GetAsyncKeyState(VK_CONTROL)) {
            if (GetAsyncKeyState(VK_LEFT))      cameraAngle += t * 5.0F;
            if (GetAsyncKeyState(VK_RIGHT))     cameraAngle -= t * 5.0F;
        } else {
            if (GetAsyncKeyState(VK_UP))        cameraPosition.z += t * cameraSpeed;
            if (GetAsyncKeyState(VK_DOWN))      cameraPosition.z -= t * cameraSpeed;
            if (GetAsyncKeyState(VK_LEFT))      cameraPosition.x += t * cameraSpeed;
            if (GetAsyncKeyState(VK_RIGHT))     cameraPosition.x -= t * cameraSpeed;
            if (GetAsyncKeyState(VK_HOME))      cameraPosition.y += t * cameraSpeed;
            if (GetAsyncKeyState(VK_END))       cameraPosition.y -= t * cameraSpeed;
        }

        if ((frameCounter % 10) == 0)
            grassManager->displayOcclusionCullingStats(this);

        glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.1f, 50000.0f);
        glm::mat4 view       = glm::lookAt(cameraPosition, cameraPosition + glm::vec3(sin(cameraAngle), 0.0F, cos(cameraAngle)), glm::vec3(0.0F, 1.0F, 0.0F));

        prevMVP = MVP;
        MVP     = projection * view;

        // actually draw some stuff
        {
            grassManager->render(drawQueue, occlusionQueue, instanceCullQueue, MVP);
        }

        // update const buffers
        {
            downscaleConstData.rtSize[0] = static_cast<float>(width);
            downscaleConstData.rtSize[1] = static_cast<float>(height);
            downscaleConstData.rtSize[2] = 0.0F;
            downscaleConstData.rtSize[3] = 0.0F;
            
            std::memcpy(downscaleConstData.viewProjection,      glm::value_ptr(glm::transpose(MVP)),                    sizeof(downscaleConstData.viewProjection));
            std::memcpy(downscaleConstData.invViewProjection,   glm::value_ptr(glm::transpose(glm::inverse(prevMVP))),  sizeof(downscaleConstData.invViewProjection));

            sgfx::updateConstantBuffer(downscaleConstBuffer, &downscaleConstData);
        }

        // submit queues
        {
            // clear
            sgfx::clearRenderTarget(renderTarget, 0xFFFFFFF);
            sgfx::clearDepthStencil(renderTarget, 1.0F, 0);

            // draw queue
            {
                sgfx::beginPerfEvent(L"Render");

                sgfx::setRenderTarget(renderTarget);
                sgfx::setViewport(width, height, 0.0F, 1.0F);

                sgfx::submit(drawQueue);

                sgfx::endPerfEvent();
            }

            sgfx::setRenderTarget(sgfx::RenderTargetHandle::invalidHandle());
            sgfx::flush();

            // TODO: according to the CryTek paper: http://www.slideshare.net/TiagoAlexSousa/secrets-of-cryengine-3-graphics-technology
            // depth reprojection should greatly improve culling efficiency, however it seems like this demo is simply not the case.
            // therefore reprojection is temporarily disabled.
            //
            // Another option is that there's no CPU readback, therefore there's only ONE frame latency, instead of 3-5.
            //
            // downscale and reproject
            if (0) {
                sgfx::beginPerfEvent(L"DownscaleAndReproject");

                sgfx::clearTextureRW(occlusionDepthBuffer, 1.0F);

                sgfx::setConstantBuffer(downscaleQueue, 0, downscaleConstBuffer);
                sgfx::setResource(downscaleQueue, 0, depthBuffer);
                sgfx::setResourceRW(downscaleQueue, 0, occlusionDepthBuffer);

                sgfx::submit(
                    downscaleQueue,
                    static_cast<uint32_t>(std::ceilf(width / 16.0F)),
                    static_cast<uint32_t>(std::ceilf(height / 16.0F)),
                    1
                );

                sgfx::copyResource(occlusionDepthBuffer, depthBuffer);

                sgfx::endPerfEvent();
            }

            // occlusion queue
            {
                sgfx::beginPerfEvent(L"OcclusionRender");

                sgfx::clearBufferRW(grassManager->occlusionDataBuffer, 0U);
                sgfx::setResourceRW(occlusionRT, 0, grassManager->occlusionDataBuffer);
                sgfx::setRenderTarget(occlusionRT);
                sgfx::setViewport(width, height, 0.0F, 1.0F);

                sgfx::submit(occlusionQueue);

                sgfx::endPerfEvent();
            }

            sgfx::setRenderTarget(sgfx::RenderTargetHandle::invalidHandle());
        }

        sgfx::present(1);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new GrassApplication;
}

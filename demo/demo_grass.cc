
#include "common/app.hh"
#include "common/meshloader.hh"

#include <vector>
#include <memory>
#include <string>
#include <stack>
#include <algorithm>

#define GLM_FORCE_PURE
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

struct LogicalMeshBuffer final
{
    uint8_t* data             = nullptr;
    size_t   dataSize         = 0;
    size_t   dataFormatStride = 0;
    size_t   physicalAddress  = 0;
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
        vertexBuffer.dataSize         = data->getVertices().size() * sizeof(MeshData::Vertex);

        indexBuffer.data              = reinterpret_cast<uint8_t*>(data->getIndices().data());
        indexBuffer.dataFormatStride  = sizeof(MeshData::Index);
        indexBuffer.dataSize          = data->getIndices().size() * sizeof(MeshData::Index);

        count = data->getIndices().size();
    }
};

struct PhysicalMeshBuffer final
{
    sgfx::BufferHandle physicalBuffer;
    size_t             physicalDataSize;
    bool               isDirty = false;

    typedef std::vector<LogicalMeshBuffer*> PageArray;
    PageArray allPages;

    PhysicalMeshBuffer() = default;
    inline ~PhysicalMeshBuffer()
    {
        sgfx::releaseBuffer(physicalBuffer);
    }

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
            size_t pageOffset = 0;
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
    struct ConstantBuffer
    {
        uint32_t dvpInternalData[4];
        float    mvp[16];
    };

    enum
    {
        kConstantBufferSize = sizeof(ConstantBuffer)
    };

    glm::vec3 position;

    LogicalMeshBuffer* vertexBuffer;
    LogicalMeshBuffer* indexBuffer;

    uint32_t count;

    GrassObject() {}

    GrassObject(LogicalMesh* data)
    {
        vertexBuffer = &data->vertexBuffer;
        indexBuffer  = &data->indexBuffer;
        count        = data->count;
    }

    ~GrassObject()
    {}
};

struct DVPGrassManager final
{
    std::vector<MeshData*>    loadedMeshes;
    std::vector<LogicalMesh*> logicalMeshes;
    std::vector<GrassObject>  grassObjects;
    PhysicalMeshBuffer        physicalVertexBuffer;
    PhysicalMeshBuffer        physicalIndexBuffer;

    sgfx::BufferHandle sharedConstantBuffer;
    size_t             numInstances = 0;

    enum
    {
        kObjectSizeSquared = 64
    };

    ~DVPGrassManager()
    {
        for (MeshData* data: loadedMeshes)
            delete data;
        for (LogicalMesh* mesh: logicalMeshes) {
            delete mesh; // TODO: release
        }
        sgfx::releaseBuffer(sharedConstantBuffer);
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

            size_t meshCounter = 0;
            for (int i = -kObjectSizeSquared; i < kObjectSizeSquared; ++i) {
                for (int j = -kObjectSizeSquared; j < kObjectSizeSquared; ++j) {

                    float fx = static_cast<float>(i) * 3.0F;
                    float fy = static_cast<float>(j) * 3.0F;

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

    void render(sgfx::DrawQueueHandle drawQueue, const glm::mat4& mvp)
    {
        physicalVertexBuffer.rebuildPages();
        physicalIndexBuffer.rebuildPages();

        if (grassObjects.size() != numInstances) {
            numInstances = grassObjects.size();

            sgfx::releaseBuffer(sharedConstantBuffer);
            sharedConstantBuffer = sgfx::createBuffer(
                sgfx::BufferFlags::CPUWrite | sgfx::BufferFlags::StructuredBuffer,
                nullptr,
                numInstances * GrassObject::kConstantBufferSize,
                GrassObject::kConstantBufferSize
            );
        }

        // update constants
        size_t maxDrawCallCount = 0;
        uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sgfx::mapBuffer(sharedConstantBuffer, sgfx::MapType::Write));
        if (dataPtr != nullptr) {

            for (size_t i = 0; i < numInstances; ++i) {
                size_t offset = i * GrassObject::kConstantBufferSize;
                const GrassObject& obj = grassObjects[i];

                glm::mat4 matrix = mvp * glm::translate(obj.position);

                GrassObject::ConstantBuffer constants;
                // fill internal data structure
                constants.dvpInternalData[0] = obj.vertexBuffer->physicalAddress;
                constants.dvpInternalData[1] = obj.indexBuffer->physicalAddress;

                constants.dvpInternalData[2] = 1; // draw indexed
                constants.dvpInternalData[3] = obj.count;
                // copy matrix
                std::memcpy(constants.mvp, glm::value_ptr(matrix), sizeof(constants.mvp));

                std::memcpy(dataPtr + offset, &constants, GrassObject::kConstantBufferSize);

                maxDrawCallCount = std::max(maxDrawCallCount, obj.count);
            }

            sgfx::unmapBuffer(sharedConstantBuffer);
        }

        sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
        sgfx::setResource(drawQueue, 0, physicalVertexBuffer.physicalBuffer);
        sgfx::setResource(drawQueue, 1, physicalIndexBuffer.physicalBuffer);
        sgfx::setResource(drawQueue, 2, sharedConstantBuffer);
        sgfx::drawInstanced(drawQueue, numInstances, maxDrawCallCount, 0);
    }
};

class GrassApplication : public Application
{
public:

    sgfx::VertexShaderHandle  vsHandle;
    sgfx::PixelShaderHandle   psHandle;
    sgfx::SurfaceShaderHandle ssHandle;

    sgfx::PipelineStateHandle pipelineState;
    sgfx::DrawQueueHandle     drawQueue;

    DVPGrassManager*          grassManager;

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

        vsHandle = loadVS("shaders/dvp.hlsl");
        psHandle = loadPS("shaders/dvp.hlsl");

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
            desc.rasterizerState.cullMode                           = sgfx::CullMode::None;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CW;

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

            desc.shader       = ssHandle;
            //desc.vertexFormat = vertexFormat;

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
        sgfx::releaseVertexShader(vsHandle);
        sgfx::releasePixelShader(psHandle);
        sgfx::releaseSurfaceShader(ssHandle);
        sgfx::releasePipelineState(pipelineState);
        sgfx::releaseDrawQueue(drawQueue);
        delete grassManager;

        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        // Update our time
        static float t = 0.0f;
        static glm::vec3 cameraPosition = glm::vec3(0.0F, 30.0F, -500.0F);

        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
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
        cameraPosition.z += t * 5.0F;

        glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.0F, width / (FLOAT)height, 0.01f, 50000.0f);
        glm::mat4 view       = glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 1.0F, 0.0F));

        glm::mat4 mvp = projection * view;

        // actually draw some stuff
        grassManager->render(drawQueue, mvp);

        sgfx::submit(drawQueue);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new GrassApplication;
}

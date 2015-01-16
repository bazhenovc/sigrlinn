
#include "sigrlinn.hh"
#include "private/drawqueue.hh"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <memory>
#include <algorithm>

#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace sgfx
{

static D3D11_PRIMITIVE_TOPOLOGY MapPrimitiveTopology[PrimitiveTopology::Count] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
};
static_assert((sizeof(MapPrimitiveTopology) / sizeof(D3D11_PRIMITIVE_TOPOLOGY)) == static_cast<size_t>(PrimitiveTopology::Count), "Mapping is broken!");

static DXGI_FORMAT MapDataFormat[DataFormat::Count] = {
    DXGI_FORMAT_BC1_TYPELESS,    // DXT1
    DXGI_FORMAT_BC2_TYPELESS,    // DXT3
    DXGI_FORMAT_BC3_TYPELESS,    // DXT5
    DXGI_FORMAT_BC4_TYPELESS,    // LATC1/ATI1
    DXGI_FORMAT_BC5_TYPELESS,    // LATC2/ATI2
    DXGI_FORMAT_BC6H_TYPELESS,   // BC6H
    DXGI_FORMAT_BC7_TYPELESS,    // BC7
    DXGI_FORMAT_UNKNOWN,         // ETC1 RGB8
    DXGI_FORMAT_UNKNOWN,         // ETC2 RGB8
    DXGI_FORMAT_UNKNOWN,         // ETC2 RGBA8
    DXGI_FORMAT_UNKNOWN,         // ETC2 RGB8A1
    DXGI_FORMAT_UNKNOWN,         // PVRTC1 RGB 2BPP
    DXGI_FORMAT_UNKNOWN,         // PVRTC1 RGB 4BPP
    DXGI_FORMAT_UNKNOWN,         // PVRTC1 RGBA 2BPP
    DXGI_FORMAT_UNKNOWN,         // PVRTC1 RGBA 4BPP
    DXGI_FORMAT_UNKNOWN,         // PVRTC2 RGBA 2BPP
    DXGI_FORMAT_UNKNOWN,         // PVRTC2 RGBA 4BPP

    DXGI_FORMAT_UNKNOWN,         // compressed formats above

    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R8G8_TYPELESS,
    DXGI_FORMAT_R16G16_TYPELESS,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R32G32_TYPELESS,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_TYPELESS,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R8G8B8A8_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R32G32B32A32_TYPELESS,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R11G11B10_FLOAT,

    DXGI_FORMAT_UNKNOWN, // depth formats below

    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_D32_FLOAT
};
static_assert((sizeof(MapDataFormat) / sizeof(DXGI_FORMAT)) == static_cast<size_t>(DataFormat::Count), "Mapping is broken!");

static D3D11_FILL_MODE MapFillMode[FillMode::Count] = {
    D3D11_FILL_SOLID,
    D3D11_FILL_WIREFRAME
};
static_assert((sizeof(MapFillMode) / sizeof(D3D11_FILL_MODE)) == static_cast<size_t>(FillMode::Count), "Mapping is broken!");

static D3D11_CULL_MODE MapCullMode[CullMode::Count] = {
    D3D11_CULL_BACK,
    D3D11_CULL_FRONT
};
static_assert((sizeof(MapCullMode) / sizeof(D3D11_CULL_MODE)) == static_cast<size_t>(CullMode::Count), "Mapping is broken!");

static BOOL MapCounterDirection[CounterDirection::Count] = {
    TRUE,
    FALSE
};
static_assert((sizeof(MapCounterDirection) / sizeof(BOOL)) == static_cast<size_t>(CounterDirection::Count), "Mapping is broken!");

static D3D11_BLEND MapBlendFactor[BlendFactor::Count] = {
    D3D11_BLEND_ZERO,
    D3D11_BLEND_ONE,
    D3D11_BLEND_SRC_ALPHA,
    D3D11_BLEND_DEST_ALPHA,
    D3D11_BLEND_INV_SRC_ALPHA,
    D3D11_BLEND_INV_DEST_ALPHA,
    D3D11_BLEND_SRC_COLOR,
    D3D11_BLEND_DEST_COLOR,
    D3D11_BLEND_INV_SRC_COLOR,
    D3D11_BLEND_INV_DEST_COLOR
};
static_assert((sizeof(MapBlendFactor) / sizeof(D3D11_BLEND)) == static_cast<size_t>(BlendFactor::Count), "Mapping is broken!");

static D3D11_BLEND_OP MapBlendOp[BlendOp::Count] = {
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT,
    D3D11_BLEND_OP_MIN,
    D3D11_BLEND_OP_MAX
};
static_assert((sizeof(MapBlendOp) / sizeof(D3D11_BLEND_OP)) == static_cast<size_t>(BlendOp::Count), "Mapping is broken!");

static D3D11_DEPTH_WRITE_MASK MapDepthWriteMask[DepthWriteMask::Count] = {
    D3D11_DEPTH_WRITE_MASK_ZERO,
    D3D11_DEPTH_WRITE_MASK_ALL
};
static_assert((sizeof(MapDepthWriteMask) / sizeof(D3D11_DEPTH_WRITE_MASK)) == static_cast<size_t>(DepthWriteMask::Count), "Mapping is broken!");

static D3D11_COMPARISON_FUNC MapComparisonFunc[ComparisonFunc::Count] = {
    D3D11_COMPARISON_ALWAYS,
    D3D11_COMPARISON_NEVER,
    D3D11_COMPARISON_LESS,
    D3D11_COMPARISON_LESS_EQUAL,
    D3D11_COMPARISON_GREATER,
    D3D11_COMPARISON_GREATER_EQUAL,
    D3D11_COMPARISON_EQUAL,
    D3D11_COMPARISON_NOT_EQUAL
};
static_assert((sizeof(MapComparisonFunc) / sizeof(D3D11_COMPARISON_FUNC)) == static_cast<size_t>(ComparisonFunc::Count), "Mapping is broken!");

static D3D11_STENCIL_OP MapStencilOp[StencilOp::Count] = {
    D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP_ZERO,
    D3D11_STENCIL_OP_REPLACE,
    D3D11_STENCIL_OP_INCR,
    D3D11_STENCIL_OP_DECR
};
static_assert((sizeof(MapStencilOp) / sizeof(D3D11_STENCIL_OP)) == static_cast<size_t>(StencilOp::Count), "Mapping is broken!");

//=============================================================================

ID3D11Device*         g_pd3dDevice         = nullptr;
ID3D11DeviceContext*  g_pImmediateContext  = nullptr;
IDXGISwapChain*       g_pSwapChain         = nullptr;

//=============================================================================

struct DXLogicalMeshBuffer final
{
    uint8_t* data             = nullptr;
    size_t   dataSize         = 0;
    size_t   dataFormatStride = 0;
    size_t   physicalAddress  = 0;

    BufferType type;
};

struct DXPhysicalMeshBuffer final
{
    ID3D11Buffer*             physicalBuffer     = nullptr;
    ID3D11ShaderResourceView* physicalBufferView = nullptr;
    size_t                    physicalDataSize   = 0;
    bool                      isDirty            = false;

    typedef internal::DynamicArray<DXLogicalMeshBuffer*, 128, 128> PageArray;
    PageArray allPages;

    DXPhysicalMeshBuffer() = default;
    inline ~DXPhysicalMeshBuffer()
    {
        if (physicalBuffer != nullptr)     physicalBuffer->Release();
        if (physicalBufferView != nullptr) physicalBufferView->Release();
    }

    // both allocate() and release() do rebuildPages()
    inline void allocate(DXLogicalMeshBuffer* logicalBuffer)
    {
        allPages.Add(logicalBuffer);
        isDirty = true;
    }

    inline void release(DXLogicalMeshBuffer* logicalBuffer)
    {
        allPages.Remove(logicalBuffer);
        isDirty = true;
    }

    inline void rebuildPages() // very expensive operation
    {
        if (!isDirty) return;

        size_t vfStride = allPages[0]->dataFormatStride; // TODO: right now will not work with different strides

        physicalDataSize = 0;
        for (DXLogicalMeshBuffer* logicalBuffer: allPages) // calculate total size
            physicalDataSize += logicalBuffer->dataSize;   // TODO: find a better place for this

        //if (physicalDataSize >= 128 * 1024)
        //    ; // TODO: error handling

        size_t numElements = physicalDataSize / vfStride;

        if (physicalBuffer != nullptr)     physicalBuffer->Release();
        if (physicalBufferView != nullptr) physicalBufferView->Release();

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.ByteWidth           = physicalDataSize;
        bufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
        bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = vfStride;
        bufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &physicalBuffer))) {
            // TODO: error handling
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));

        viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
        viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
        viewDesc.Buffer.ElementWidth = numElements;

        if (FAILED(g_pd3dDevice->CreateShaderResourceView(physicalBuffer, &viewDesc, &physicalBufferView))) {
            // TODO: error handling
            return;
        }

        // fill the physical buffer
        D3D11_MAPPED_SUBRESOURCE mappedData;
        std::memset(&mappedData, 0, sizeof(mappedData));

        if (FAILED(g_pImmediateContext->Map(physicalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData))) {
            // TODO: error handling
            return;
        }

        uint8_t* dataPtr = reinterpret_cast<uint8_t*>(mappedData.pData);
        size_t pageOffset = 0;
        for (size_t i = 0; i < allPages.GetSize(); ++i) {
            DXLogicalMeshBuffer* logicalBuffer = allPages[i];
            std::memcpy(dataPtr + pageOffset, logicalBuffer->data, logicalBuffer->dataSize);
            logicalBuffer->physicalAddress = pageOffset / logicalBuffer->dataFormatStride;
            pageOffset += logicalBuffer->dataSize;
        }

        g_pImmediateContext->Unmap(physicalBuffer, 0);

        isDirty = false;
    }
};

struct DXSharedBuffer final
{
    ID3D11Buffer*             dataBuffer     = nullptr;
    ID3D11ShaderResourceView* dataView       = nullptr;
    size_t                    dataBufferSize = 0;

    DXSharedBuffer() {}
    ~DXSharedBuffer()
    {
        if (dataBuffer != nullptr) dataBuffer->Release();
        if (dataView   != nullptr) dataView->Release();
    }

    inline void setImmutable(void* mem, size_t size, size_t stride)
    {
        dataBufferSize = size;

        if (dataBuffer != nullptr) dataBuffer->Release();
        if (dataView   != nullptr) dataView->Release();

        D3D11_BUFFER_DESC bufferDesc;
        std::memset(&bufferDesc, 0, sizeof(bufferDesc));

        bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.ByteWidth           = dataBufferSize;
        bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
        bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = stride;
        bufferDesc.CPUAccessFlags      = 0;

        D3D11_SUBRESOURCE_DATA bufferData;
        std::memset(&bufferData, 0, sizeof(bufferData));

        bufferData.pSysMem = mem;

        if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, &bufferData, &dataBuffer))) {
            // TODO: error handling
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));

        viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
        viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
        viewDesc.Buffer.ElementWidth = size / stride;

        if (FAILED(g_pd3dDevice->CreateShaderResourceView(dataBuffer, &viewDesc, &dataView))) {
            // TODO: error handling
            return;
        }
    }

    inline void setConstant(size_t newSize, size_t stride, size_t numInstances)
    {
        if (newSize > dataBufferSize) {
            dataBufferSize = newSize;

            if (dataBuffer != nullptr) dataBuffer->Release();
            if (dataView   != nullptr) dataView->Release();

            D3D11_BUFFER_DESC bufferDesc;
            std::memset(&bufferDesc, 0, sizeof(bufferDesc));

            bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
            bufferDesc.ByteWidth           = dataBufferSize;
            bufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
            bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            bufferDesc.StructureByteStride = stride;
            bufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;

            if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &dataBuffer))) {
                // TODO: error handling
                return;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
            std::memset(&viewDesc, 0, sizeof(viewDesc));

            viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
            viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
            viewDesc.Buffer.ElementWidth = numInstances;

            if (FAILED(g_pd3dDevice->CreateShaderResourceView(dataBuffer, &viewDesc, &dataView))) {
                // TODO: error handling
                return;
            }
        }
    }
};

DXPhysicalMeshBuffer* g_physicalVertexBuffer = nullptr;
DXPhysicalMeshBuffer* g_physicalIndexBuffer  = nullptr;

//=============================================================================

struct VertexFormatImpl final
{
    ID3D11InputLayout* inputLayout;
    UINT               stride;
};

struct SurfaceShaderImpl final
{
    ID3D11VertexShader*   vs;
    ID3D11HullShader*     hs;
    ID3D11DomainShader*   ds;
    ID3D11GeometryShader* gs;
    ID3D11PixelShader*    ps;
};

struct PipelineStateImpl final
{
    ID3D11RasterizerState*   rasterizerState;
    ID3D11BlendState*        blendState;
    ID3D11DepthStencilState* depthStencilState;

    SurfaceShaderImpl*       shader;
    VertexFormatImpl*        vertexFormat;

    // additional stuff passed as parameters
    UINT                     stencilRef;

    // constant buffer
    DXSharedBuffer           constantBuffer;
};

static UINT dxFormatStride(DataFormat format)
{
    switch (format) {
    case DataFormat::R8:      { return 1;  } break;
    case DataFormat::R16:     { return 2;  } break;
    case DataFormat::R16F:    { return 2;  } break;
    case DataFormat::R32I:    { return 4;  } break;
    case DataFormat::R32U:    { return 4;  } break;
    case DataFormat::R32F:    { return 4;  } break;
    case DataFormat::RG8:     { return 2;  } break;
    case DataFormat::RG16:    { return 4;  } break;
    case DataFormat::RG16F:   { return 4;  } break;
    case DataFormat::RG32:    { return 8;  } break;
    case DataFormat::RG32F:   { return 8;  } break;
    case DataFormat::RGB32:   { return 12; } break;
    case DataFormat::RGB32F:  { return 12; } break;
    case DataFormat::RGBA8:   { return 4;  } break;
    case DataFormat::RGBA16:  { return 8;  } break;
    case DataFormat::RGBA16F: { return 8;  } break;
    case DataFormat::RGBA32:  { return 16; } break;
    case DataFormat::RGBA32F: { return 16; } break;

    default: { return 0; } break; // unsupported
    }
};

static void dxSetPipelineState(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        PipelineStateImpl* impl = static_cast<PipelineStateImpl*>(handle.value);

        g_pImmediateContext->RSSetState(impl->rasterizerState);
        g_pImmediateContext->OMSetBlendState(impl->blendState, nullptr, 0xffffffff);
        g_pImmediateContext->OMSetDepthStencilState(impl->depthStencilState, impl->stencilRef);

        //g_pImmediateContext->IASetInputLayout(impl->vertexFormat->inputLayout);
        g_pImmediateContext->IASetInputLayout(nullptr);

        g_pImmediateContext->VSSetShader(impl->shader->vs, nullptr, 0);
        g_pImmediateContext->HSSetShader(impl->shader->hs, nullptr, 0);
        g_pImmediateContext->DSSetShader(impl->shader->ds, nullptr, 0);
        g_pImmediateContext->GSSetShader(impl->shader->gs, nullptr, 0);
        g_pImmediateContext->PSSetShader(impl->shader->ps, nullptr, 0);
    }
}

static void dxProcessDrawQueue(internal::DrawQueue* queue)
{
    PipelineStateImpl* psimpl = static_cast<PipelineStateImpl*>(queue->getState().value);

    dxSetPipelineState(queue->getState());
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_physicalVertexBuffer->rebuildPages(); // TODO: move somewhere else
    g_physicalIndexBuffer->rebuildPages(); // TODO: move somewhere else

//#define DC_CHECK_PROCESSED
#ifdef DC_CHECK_PROCESSED
    size_t _checkProcessedCalls = 0;
#endif

    size_t numInstances = queue->getDrawCalls().GetSize();

    // process draw calls
    psimpl->constantBuffer.setConstant(
        internal::DrawCall::ConstantBufferSize * numInstances,
        internal::DrawCall::ConstantBufferSize,
        numInstances
    );

    // update constants
    {
        D3D11_MAPPED_SUBRESOURCE mappedData;
        if (FAILED(g_pImmediateContext->Map(psimpl->constantBuffer.dataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData))) {
            // TODO: error handling
            return;
        }
        uint8_t* dataPtr = reinterpret_cast<uint8_t*>(mappedData.pData);
        for (size_t i = 0; i < numInstances; ++i) {
            size_t offset = i * internal::DrawCall::ConstantBufferSize;
            const internal::DrawCall& call = queue->getDrawCalls()[i];

            std::memcpy(dataPtr + offset, call.constantBufferData, internal::DrawCall::ConstantBufferSize);

            // fill internal data structure
            struct InternalData
            {
                uint32_t vb;
                uint32_t ib;
                uint32_t drawCallType;
                uint32_t reserved;
            };

            InternalData* idata = reinterpret_cast<InternalData*>(dataPtr + offset);

            DXLogicalMeshBuffer* vertexBuffer = static_cast<DXLogicalMeshBuffer*>(call.vertexBuffer.value);
            if (vertexBuffer != nullptr)
                idata->vb = vertexBuffer->physicalAddress;

            DXLogicalMeshBuffer* indexBuffer = static_cast<DXLogicalMeshBuffer*>(call.indexBuffer.value);
            if (indexBuffer != nullptr)
                idata->ib = indexBuffer->physicalAddress;

            idata->drawCallType = call.type;
        }
        g_pImmediateContext->Unmap(psimpl->constantBuffer.dataBuffer, 0);
    }

    const internal::DrawCall& call = queue->getDrawCalls()[0]; // TODO: get rid of this somehow

    ID3D11ShaderResourceView* vbibViews[2] = {
        g_physicalVertexBuffer->physicalBufferView,
        g_physicalIndexBuffer->physicalBufferView
    };

    g_pImmediateContext->VSSetShaderResources(0, 2, vbibViews);

    g_pImmediateContext->VSSetShaderResources(0 + 2, 1, &psimpl->constantBuffer.dataView);
    g_pImmediateContext->HSSetShaderResources(0 + 2, 1, &psimpl->constantBuffer.dataView);
    g_pImmediateContext->DSSetShaderResources(0 + 2, 1, &psimpl->constantBuffer.dataView);
    g_pImmediateContext->GSSetShaderResources(0 + 2, 1, &psimpl->constantBuffer.dataView);
    g_pImmediateContext->PSSetShaderResources(0 + 2, 1, &psimpl->constantBuffer.dataView);

    g_pImmediateContext->DrawInstanced(call.count, numInstances, 0, 0);

#ifdef DC_CHECK_PROCESSED
    if (_checkProcessedCalls != queue->getDrawCalls().GetSize())
        OutputDebugString("Not all draw calls were processed!\n");
#endif
}

//=============================================================================

bool initD3D11(void* d3dDevice, void* d3dContext, void* d3dSwapChain)
{
    g_pd3dDevice        = static_cast<ID3D11Device*>(d3dDevice);
    g_pImmediateContext = static_cast<ID3D11DeviceContext*>(d3dContext);
    g_pSwapChain        = static_cast<IDXGISwapChain*>(d3dSwapChain);

    g_physicalVertexBuffer = new DXPhysicalMeshBuffer;
    g_physicalIndexBuffer  = new DXPhysicalMeshBuffer;

    return true;
}

void shutdown()
{
    delete g_physicalVertexBuffer;
    delete g_physicalIndexBuffer;
}

// shader compiling

bool compileShader(
    const char*          sourceCode,
    size_t               sourceCodeSize,
    ShaderCompileVersion version,
    ShaderCompileTarget  target,
    ShaderCompileMacro*  macros,
    size_t               macrosSize,
    uint64_t             flags,
    ErrorReportFunc      errorReport,

    void*&  outData,
    size_t& outDataSize
)
{
    D3D_SHADER_MACRO* d3dmacros = nullptr;

    if (macros != nullptr) {
        size_t totalSize = macrosSize + sizeof(D3D_SHADER_MACRO);
        d3dmacros = reinterpret_cast<D3D_SHADER_MACRO*>(alloca(totalSize));
        std::memset(d3dmacros, 0, totalSize);

        for (size_t i = 0; i < macrosSize; ++i) {
            d3dmacros[i].Name       = macros[i].name;
            d3dmacros[i].Definition = macros[i].value;
        }
    }

    ID3DBlob* outBlob   = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT d3dFlags           = 0;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Debug))     d3dFlags |= D3DCOMPILE_DEBUG;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Strict))    d3dFlags |= D3DCOMPILE_ENABLE_STRICTNESS;
    if (flags & static_cast<UINT>(ShaderCompileFlags::IEEStrict)) d3dFlags |= D3DCOMPILE_IEEE_STRICTNESS;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Optimize0)) d3dFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Optimize1)) d3dFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Optimize2)) d3dFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
    if (flags & static_cast<UINT>(ShaderCompileFlags::Optimize3)) d3dFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

    LPCSTR d3dTarget        = "";
    LPCSTR d3dEntryPoint    = "";

    switch (version) {
    case ShaderCompileVersion::v4_0: {
        switch(target) {
        case ShaderCompileTarget::VS: { d3dEntryPoint = "vs_main"; d3dTarget = "vs_4_0"; } break;
        case ShaderCompileTarget::GS: { d3dEntryPoint = "gs_main"; d3dTarget = "gs_4_0"; } break;
        case ShaderCompileTarget::PS: { d3dEntryPoint = "ps_main"; d3dTarget = "ps_4_0"; } break;
        case ShaderCompileTarget::CS: { d3dEntryPoint = "cs_main"; d3dTarget = "cs_4_0"; } break;
        default: {} break; // TODO: raise error here!
        }
    } break;

    case ShaderCompileVersion::v5_0: {
        switch (target) {
        case ShaderCompileTarget::VS: { d3dEntryPoint = "vs_main"; d3dTarget = "vs_5_0"; } break;
        case ShaderCompileTarget::HS: { d3dEntryPoint = "hs_main"; d3dTarget = "hs_5_0"; } break;
        case ShaderCompileTarget::DS: { d3dEntryPoint = "ds_main"; d3dTarget = "ds_5_0"; } break;
        case ShaderCompileTarget::GS: { d3dEntryPoint = "gs_main"; d3dTarget = "gs_5_0"; } break;
        case ShaderCompileTarget::PS: { d3dEntryPoint = "ps_main"; d3dTarget = "ps_5_0"; } break;
        case ShaderCompileTarget::CS: { d3dEntryPoint = "cs_main"; d3dTarget = "cs_5_0"; } break;
        }
    } break;
    }

    HRESULT hr = D3DCompile(
        sourceCode,
        sourceCodeSize,
        nullptr,
        d3dmacros,
        nullptr,
        d3dEntryPoint,
        d3dTarget,
        d3dFlags,
        0,
        &outBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorReport != nullptr) {
            char* error = new char[errorBlob->GetBufferSize() + 1];
            std::memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            error[errorBlob->GetBufferSize()] = '\0';
            errorReport(error);
            delete [] error;
        }
        errorBlob->Release();
        return false;
    }

    outDataSize = outBlob->GetBufferSize();
    outData = new uint8_t[outDataSize];
    std::memcpy(outData, outBlob->GetBufferPointer(), outDataSize);
    outBlob->Release();

    return true;
}

// shaders
VertexShaderHandle createVertexShader(const void* data, size_t dataSize)
{
    ID3D11VertexShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreateVertexShader(data, dataSize, nullptr, &shader))) {
        return VertexShaderHandle::invalidHandle();
    }
    return VertexShaderHandle(shader);
}

void releaseVertexShader(VertexShaderHandle handle)
{
    if (handle != VertexShaderHandle::invalidHandle()) {
        ID3D11VertexShader* shader = static_cast<ID3D11VertexShader*>(handle.value);
        shader->Release();
    }
}

HullShaderHandle createHullShader(const void* data, size_t dataSize)
{
    ID3D11HullShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreateHullShader(data, dataSize, nullptr, &shader))) {
        return HullShaderHandle::invalidHandle();
    }
    return HullShaderHandle(shader);
}

void releaseHullShader(HullShaderHandle handle)
{
    if (handle != HullShaderHandle::invalidHandle()) {
        ID3D11HullShader* shader = static_cast<ID3D11HullShader*>(handle.value);
        shader->Release();
    }
}

DomainShaderHandle createDomainShader(const void* data, size_t dataSize)
{
    ID3D11DomainShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreateDomainShader(data, dataSize, nullptr, &shader))) {
        return DomainShaderHandle::invalidHandle();
    }
    return DomainShaderHandle(shader);
}

void releaseDomainShader(DomainShaderHandle handle)
{
    if (handle != DomainShaderHandle::invalidHandle()) {
        ID3D11DomainShader* shader = static_cast<ID3D11DomainShader*>(handle.value);
        shader->Release();
    }
}

GeometryShaderHandle createGeometryShader(const void* data, size_t dataSize)
{
    ID3D11GeometryShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreateGeometryShader(data, dataSize, nullptr, &shader))) {
        return GeometryShaderHandle::invalidHandle();
    }
    return GeometryShaderHandle(shader);
}

void releaseGeometryShader(GeometryShaderHandle handle)
{
    if (handle != GeometryShaderHandle::invalidHandle()) {
        ID3D11GeometryShader* shader = static_cast<ID3D11GeometryShader*>(handle.value);
        shader->Release();
    }
}

PixelShaderHandle createPixelShader(const void* data, size_t dataSize)
{
    ID3D11PixelShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreatePixelShader(data, dataSize, nullptr, &shader))) {
        return PixelShaderHandle::invalidHandle();
    }
    return PixelShaderHandle(shader);
}

void releasePixelShader(PixelShaderHandle handle)
{
    if (handle != PixelShaderHandle::invalidHandle()) {
        ID3D11PixelShader* shader = static_cast<ID3D11PixelShader*>(handle.value);
        shader->Release();
    }
}

SurfaceShaderHandle linkSurfaceShader(VertexShaderHandle vs, HullShaderHandle hs, DomainShaderHandle ds, GeometryShaderHandle gs, PixelShaderHandle ps)
{
    SurfaceShaderImpl* impl = new SurfaceShaderImpl;
    impl->vs = static_cast<ID3D11VertexShader*>(vs.value);
    impl->hs = static_cast<ID3D11HullShader*>(hs.value);
    impl->ds = static_cast<ID3D11DomainShader*>(ds.value);
    impl->gs = static_cast<ID3D11GeometryShader*>(gs.value);
    impl->ps = static_cast<ID3D11PixelShader*>(ps.value);
    return SurfaceShaderHandle(impl);
}

void releaseSurfaceShader(SurfaceShaderHandle handle)
{
    if (handle != SurfaceShaderHandle::invalidHandle()) {
        SurfaceShaderImpl* impl = static_cast<SurfaceShaderImpl*>(handle.value);
        delete impl;
    }
}

ComputeShaderHandle createComputeShader(const void* data, size_t dataSize)
{
    ID3D11ComputeShader* shader = nullptr;
    if (FAILED(g_pd3dDevice->CreateComputeShader(data, dataSize, nullptr, &shader))) {
        return ComputeShaderHandle::invalidHandle();
    }
    return ComputeShaderHandle(shader);
}

void releaseComputeShader(ComputeShaderHandle handle)
{
    if (handle != ComputeShaderHandle::invalidHandle()) {
        ID3D11ComputeShader* shader = static_cast<ID3D11ComputeShader*>(handle.value);
        shader->Release();
    }
}

void dispatchComputeShader(ComputeShaderHandle handle, uint32_t x, uint32_t y, uint32_t z)
{
    if (handle != ComputeShaderHandle::invalidHandle()) {
        ID3D11ComputeShader* shader = static_cast<ID3D11ComputeShader*>(handle.value);
        g_pImmediateContext->CSSetShader(shader, nullptr, 0);
        g_pImmediateContext->Dispatch(x, y, z);
    }
}

PipelineStateHandle createPipelineState(const PipelineStateDescriptor& desc)
{
    const RasterizerState& rsState = desc.rasterizerState;

    D3D11_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = MapFillMode[static_cast<uint64_t>(rsState.fillMode)];
    rasterizerDesc.CullMode              = MapCullMode[static_cast<uint64_t>(rsState.cullMode)];
    rasterizerDesc.FrontCounterClockwise = MapCounterDirection[static_cast<uint64_t>(rsState.counterDirection)];
    rasterizerDesc.DepthBias             = 0;
    rasterizerDesc.DepthBiasClamp        = 0.0F;
    rasterizerDesc.SlopeScaledDepthBias  = 0.0F;
    rasterizerDesc.DepthClipEnable       = TRUE;
    rasterizerDesc.ScissorEnable         = FALSE;
    rasterizerDesc.MultisampleEnable     = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;

    ID3D11RasterizerState* rasterizerState = nullptr;
    if (FAILED(g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState))) {
        return PipelineStateHandle::invalidHandle();
    }

    const BlendState& bsState = desc.blendState;

    D3D11_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable  = bsState.alphaToCoverageEnabled;
    blendDesc.IndependentBlendEnable = bsState.separateBlendEnabled;
    std::memset(&blendDesc.RenderTarget, 0, 8 * sizeof(D3D11_RENDER_TARGET_BLEND_DESC));

    //if (bsState.blendDesc.blendEnabled) {
        blendDesc.RenderTarget[0].BlendEnable           = bsState.blendDesc.blendEnabled;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].SrcBlend              = MapBlendFactor[static_cast<uint64_t>(bsState.blendDesc.srcBlend)];
        blendDesc.RenderTarget[0].DestBlend             = MapBlendFactor[static_cast<uint64_t>(bsState.blendDesc.dstBlend)];
        blendDesc.RenderTarget[0].BlendOp               = MapBlendOp[static_cast<uint64_t>(bsState.blendDesc.blendOp)];
        blendDesc.RenderTarget[0].SrcBlendAlpha         = MapBlendFactor[static_cast<uint64_t>(bsState.blendDesc.srcBlendAlpha)];
        blendDesc.RenderTarget[0].DestBlendAlpha        = MapBlendFactor[static_cast<uint64_t>(bsState.blendDesc.dstBlendAlpha)];
        blendDesc.RenderTarget[0].BlendOpAlpha          = MapBlendOp[static_cast<uint64_t>(bsState.blendDesc.blendOpAlpha)];
    //}

    if (bsState.separateBlendEnabled) {
        for (size_t i = 0; i < 8; ++i) {
            blendDesc.RenderTarget[i].BlendEnable           = bsState.renderTargetBlendDesc[i].blendEnabled;
            blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            blendDesc.RenderTarget[i].SrcBlend              = MapBlendFactor[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].srcBlend)];
            blendDesc.RenderTarget[i].DestBlend             = MapBlendFactor[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].dstBlend)];
            blendDesc.RenderTarget[i].BlendOp               = MapBlendOp[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].blendOp)];
            blendDesc.RenderTarget[i].SrcBlendAlpha         = MapBlendFactor[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].srcBlendAlpha)];
            blendDesc.RenderTarget[i].DestBlendAlpha        = MapBlendFactor[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].dstBlendAlpha)];
            blendDesc.RenderTarget[i].BlendOpAlpha          = MapBlendOp[static_cast<uint64_t>(bsState.renderTargetBlendDesc[i].blendOpAlpha)];
        }
    }

    ID3D11BlendState* blendState = nullptr;
    if (FAILED(g_pd3dDevice->CreateBlendState(&blendDesc, &blendState))) {
        rasterizerState->Release();
        return PipelineStateHandle::invalidHandle();
    }

    const DepthStencilState& dsState = desc.depthStencilState;

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    depthStencilDesc.DepthEnable                  = dsState.depthEnabled;
    depthStencilDesc.DepthWriteMask               = MapDepthWriteMask[static_cast<uint64_t>(dsState.writeMask)];
    depthStencilDesc.DepthFunc                    = MapComparisonFunc[static_cast<uint64_t>(dsState.depthFunc)];
    depthStencilDesc.StencilEnable                = dsState.stencilEnabled;
    depthStencilDesc.StencilReadMask              = dsState.stencilReadMask;
    depthStencilDesc.StencilWriteMask             = dsState.stencilWriteMask;
    depthStencilDesc.FrontFace.StencilFunc        = MapComparisonFunc[static_cast<uint64_t>(dsState.frontFaceStencilDesc.stencilFunc)];
    depthStencilDesc.FrontFace.StencilFailOp      = MapStencilOp[static_cast<uint64_t>(dsState.frontFaceStencilDesc.failOp)];
    depthStencilDesc.FrontFace.StencilDepthFailOp = MapStencilOp[static_cast<uint64_t>(dsState.frontFaceStencilDesc.depthFailOp)];
    depthStencilDesc.FrontFace.StencilPassOp      = MapStencilOp[static_cast<uint64_t>(dsState.frontFaceStencilDesc.passOp)];
    depthStencilDesc.BackFace.StencilFunc         = MapComparisonFunc[static_cast<uint64_t>(dsState.backFaceStencilDesc.stencilFunc)];
    depthStencilDesc.BackFace.StencilFailOp       = MapStencilOp[static_cast<uint64_t>(dsState.backFaceStencilDesc.failOp)];
    depthStencilDesc.BackFace.StencilDepthFailOp  = MapStencilOp[static_cast<uint64_t>(dsState.backFaceStencilDesc.depthFailOp)];
    depthStencilDesc.BackFace.StencilPassOp       = MapStencilOp[static_cast<uint64_t>(dsState.backFaceStencilDesc.passOp)];

    ID3D11DepthStencilState* depthStencilState = nullptr;
    if (FAILED(g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &depthStencilState))) {
        rasterizerState->Release();
        blendState->Release();
        return PipelineStateHandle::invalidHandle();
    }

    PipelineStateImpl* impl = new PipelineStateImpl;
    impl->rasterizerState   = rasterizerState;
    impl->blendState        = blendState;
    impl->depthStencilState = depthStencilState;

    impl->shader       = static_cast<SurfaceShaderImpl*>(desc.shader.value);

    impl->stencilRef = dsState.stencilRef;

    return PipelineStateHandle(impl);
}

void releasePipelineState(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        PipelineStateImpl* impl = static_cast<PipelineStateImpl*>(handle.value);
        impl->rasterizerState->Release();
        impl->blendState->Release();
        impl->depthStencilState->Release();
        delete impl;
    }
}

BufferHandle createBuffer(BufferType type, void* mem, size_t size, size_t stride)
{
    DXLogicalMeshBuffer* logicalBuffer = new DXLogicalMeshBuffer;
    logicalBuffer->data                = new uint8_t[size];
    logicalBuffer->dataSize            = size;
    logicalBuffer->dataFormatStride    = stride;
    logicalBuffer->type                = type;

    std::memcpy(logicalBuffer->data, mem, size);

    switch (type) {
    case BufferType::Vertex: { g_physicalVertexBuffer->allocate(logicalBuffer); } break;
    case BufferType::Index:  { g_physicalIndexBuffer->allocate(logicalBuffer);  } break;
    };

    return BufferHandle(logicalBuffer);
}

void releaseBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXLogicalMeshBuffer* logicalBuffer = static_cast<DXLogicalMeshBuffer*>(handle.value);

        switch (logicalBuffer->type) {
        case BufferType::Vertex: { g_physicalVertexBuffer->release(logicalBuffer); } break;
        case BufferType::Index:  { g_physicalIndexBuffer->release(logicalBuffer);  } break;
        };

        delete [] logicalBuffer->data;
        delete logicalBuffer;
    }
}

TransientBufferHandle createTransientBuffer(TransientBufferType type, void* mem, size_t size)
{
    UINT bindFlags = 0;
    switch (type) {
    case TransientBufferType::Vertex:   { bindFlags = D3D11_BIND_VERTEX_BUFFER;    } break;
    case TransientBufferType::Index:    { bindFlags = D3D11_BIND_INDEX_BUFFER;     } break;
    case TransientBufferType::Constant: { bindFlags = D3D11_BIND_CONSTANT_BUFFER;  } break;
    case TransientBufferType::Storage:  { bindFlags = D3D11_BIND_UNORDERED_ACCESS; } break;
    }

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = size;
    desc.Usage               = D3D11_USAGE_DEFAULT;
    desc.BindFlags           = bindFlags;
    desc.CPUAccessFlags      = 0;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem          = mem;
    data.SysMemPitch      = 0;
    data.SysMemSlicePitch = 0;

    ID3D11Buffer* buffer = nullptr;
    if (FAILED(g_pd3dDevice->CreateBuffer(&desc, mem == nullptr ? nullptr : &data, &buffer))) {
        return BufferHandle::invalidHandle();
    }

    return BufferHandle(buffer);
}

void updateTransientBuffer(TransientBufferHandle handle, void* mem, size_t size, size_t offset)
{
    if (handle != TransientBufferHandle::invalidHandle()) {
        ID3D11Buffer* buffer = static_cast<ID3D11Buffer*>(handle.value);

        D3D11_BOX box; // a box? wtf?
        box.left   = offset;
        box.right  = offset + size;
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        g_pImmediateContext->UpdateSubresource(
            buffer, 0,
            offset == 0 ? nullptr : &box,
            mem,
            0, 0
        );
    }
}

DrawQueueHandle createDrawQueue(PipelineStateHandle state)
{
    internal::DrawQueue* queue = new internal::DrawQueue(state);
    return DrawQueueHandle(queue);
}

void releaseDrawQueue(DrawQueueHandle handle)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        delete queue;
    }
}

void setPrimitiveTopology(DrawQueueHandle handle, PrimitiveTopology topology)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setPrimitiveTopology(topology);
    }
}

void setVertexBuffer(DrawQueueHandle handle, BufferHandle vb)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setVertexBuffer(vb);
    }
}

void setIndexBuffer(DrawQueueHandle handle, BufferHandle ib)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setIndexBuffer(ib);
    }
}

void setConstants(DrawQueueHandle handle, uint32_t idx, void* constantsData, size_t constantsSize)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setConstants(idx, constantsData, constantsSize);
    }
}

void draw(DrawQueueHandle handle, uint32_t count, uint32_t startVertex)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->draw(count, startVertex);
    }
}

void drawIndexed(DrawQueueHandle handle, uint32_t count, uint32_t startIndex, uint32_t startVertex)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->drawIndexed(count, startIndex, startVertex);
    }
}

void submit(DrawQueueHandle handle)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        dxProcessDrawQueue(queue);
        queue->clear();
    }
}

}

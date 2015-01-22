
#include "sigrlinn.hh"
#include "private/drawqueue.hh"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <memory>

#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace sgfx
{

static D3D11_MAP MapMapType[MapType::Count] = {
    D3D11_MAP_READ,
    D3D11_MAP_WRITE_DISCARD
};
static_assert((sizeof(MapMapType) / sizeof(D3D11_MAP)) == static_cast<uint32_t>(MapType::Count), "Mapping is broken!");

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
    D3D11_CULL_FRONT,
    D3D11_CULL_NONE
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

static D3D11_INPUT_CLASSIFICATION MapVertexElementType[VertexElementType::Count] = {
    D3D11_INPUT_PER_VERTEX_DATA,
    D3D11_INPUT_PER_INSTANCE_DATA
};
static_assert((sizeof(MapVertexElementType) / sizeof(D3D11_INPUT_CLASSIFICATION)) == static_cast<size_t>(VertexElementType::Count), "Mapping is broken!");


//=============================================================================

ID3D11Device*         g_pd3dDevice         = nullptr;
ID3D11DeviceContext*  g_pImmediateContext  = nullptr;
IDXGISwapChain*       g_pSwapChain         = nullptr;

//=============================================================================
struct DXSharedBuffer final
{
    ID3D11Buffer*              dataBuffer     = nullptr;
    ID3D11ShaderResourceView*  dataView       = nullptr;
    ID3D11UnorderedAccessView* dataUAV        = 0;
    size_t                     dataBufferSize = 0;

    DXSharedBuffer() {}
    ~DXSharedBuffer()
    {
        if (dataBuffer != nullptr) dataBuffer->Release();
        if (dataView   != nullptr) dataView->Release();
        if (dataUAV    != nullptr) dataUAV->Release();
    }

    inline void createFromDesc(const D3D11_BUFFER_DESC& bufferDesc, const D3D11_SUBRESOURCE_DATA& bufferData)
    {
        if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, (bufferData.pSysMem == nullptr) ? nullptr : &bufferData, &dataBuffer))) {
            // TODO: error handling
            return;
        }
    }

    inline void createView(size_t numElements)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));
        viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
        viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
        viewDesc.Buffer.ElementWidth = numElements;

        if (FAILED(g_pd3dDevice->CreateShaderResourceView(dataBuffer, &viewDesc, &dataView))) {
            // TODO: error handling
            return;
        }
    }

    inline void createUAV(size_t numElements)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        std::memset(&uavDesc, 0, sizeof(uavDesc));
        uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Format             = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = numElements;

        if (FAILED(g_pd3dDevice->CreateUnorderedAccessView(dataBuffer, &uavDesc, &dataUAV))) {
            // TODO: error handling
            return;
        }
    }
};

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
    ID3D11RasterizerState*   rasterizerState   = nullptr;
    ID3D11BlendState*        blendState        = nullptr;
    ID3D11DepthStencilState* depthStencilState = nullptr;

    SurfaceShaderImpl*       shader            = nullptr;
    VertexFormatImpl*        vertexFormat      = nullptr;

    // additional stuff passed as parameters
    UINT                     stencilRef;
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

        if (impl->vertexFormat != nullptr)
            g_pImmediateContext->IASetInputLayout(impl->vertexFormat->inputLayout);
        else
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

    for (const internal::DrawCall& call: queue->getDrawCalls()) {
        DXSharedBuffer* vertexBuffer = static_cast<DXSharedBuffer*>(call.vertexBuffer.value);
        DXSharedBuffer* indexBuffer  = static_cast<DXSharedBuffer*>(call.indexBuffer.value);
        UINT            offset       = 0;

        g_pImmediateContext->IASetPrimitiveTopology(MapPrimitiveTopology[static_cast<uint64_t>(call.primitiveTopology)]);
        if (psimpl->vertexFormat != nullptr) {
            g_pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer->dataBuffer, &psimpl->vertexFormat->stride, &offset);
            g_pImmediateContext->IASetIndexBuffer(indexBuffer->dataBuffer, DXGI_FORMAT_R32_UINT, 0);
        }

        // constant buffers are ID3D11Buffers effectively
        ID3D11Buffer* constantBuffers[internal::DrawCall::kMaxConstantBuffers] = { nullptr };
        for (size_t i = 0; i < internal::DrawCall::kMaxConstantBuffers; ++i)
            constantBuffers[i] = static_cast<ID3D11Buffer*>(call.constantBuffers[i].value);

        g_pImmediateContext->VSSetConstantBuffers(0, internal::DrawCall::kMaxConstantBuffers, constantBuffers);
        g_pImmediateContext->HSSetConstantBuffers(0, internal::DrawCall::kMaxConstantBuffers, constantBuffers);
        g_pImmediateContext->DSSetConstantBuffers(0, internal::DrawCall::kMaxConstantBuffers, constantBuffers);
        g_pImmediateContext->GSSetConstantBuffers(0, internal::DrawCall::kMaxConstantBuffers, constantBuffers);
        g_pImmediateContext->PSSetConstantBuffers(0, internal::DrawCall::kMaxConstantBuffers, constantBuffers);

        // shader resources
        ID3D11ShaderResourceView* shaderResources[internal::DrawCall::kMaxShaderResources] = { nullptr };
        for (size_t i = 0; i < internal::DrawCall::kMaxShaderResources; ++i) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(call.shaderResources[i].value);
            if (buffer != nullptr)
                shaderResources[i] = buffer->dataView;
        }

        g_pImmediateContext->VSSetShaderResources(0, internal::DrawCall::kMaxShaderResources, shaderResources);
        g_pImmediateContext->HSSetShaderResources(0, internal::DrawCall::kMaxShaderResources, shaderResources);
        g_pImmediateContext->DSSetShaderResources(0, internal::DrawCall::kMaxShaderResources, shaderResources);
        g_pImmediateContext->GSSetShaderResources(0, internal::DrawCall::kMaxShaderResources, shaderResources);
        g_pImmediateContext->PSSetShaderResources(0, internal::DrawCall::kMaxShaderResources, shaderResources);

        switch (call.type) {
        case internal::DrawCall::Draw:                 { g_pImmediateContext->Draw(call.count, call.startVertex); } break;
        case internal::DrawCall::DrawIndexed:          { g_pImmediateContext->DrawIndexed(call.count, call.startIndex, call.startVertex); } break;
        case internal::DrawCall::DrawInstanced:        { g_pImmediateContext->DrawInstanced(call.count, call.instanceCount, call.startVertex, 0); } break;
        case internal::DrawCall::DrawIndexedInstanced: { g_pImmediateContext->DrawIndexedInstanced(call.count, call.instanceCount, call.startIndex, call.startVertex, 0); } break;
        }
    }
}

//=============================================================================

static void reportError(const char* msg)
{
    OutputDebugString(msg);
}

bool initD3D11(void* d3dDevice, void* d3dContext, void* d3dSwapChain)
{
    g_pd3dDevice        = static_cast<ID3D11Device*>(d3dDevice);
    g_pImmediateContext = static_cast<ID3D11DeviceContext*>(d3dContext);
    g_pSwapChain        = static_cast<IDXGISwapChain*>(d3dSwapChain);

    return true;
}

void shutdown()
{}

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

VertexFormatHandle createVertexFormat(
    VertexElementDescriptor* elements,
    size_t size,
    void* shaderBytecode, size_t shaderBytecodeSize,
    ErrorReportFunc errorReport
)
{
    if (elements == nullptr || size == 0)
        return VertexFormatHandle::invalidHandle();

    size_t totalSize = size * sizeof(D3D11_INPUT_ELEMENT_DESC);
    D3D11_INPUT_ELEMENT_DESC* inputData = reinterpret_cast<D3D11_INPUT_ELEMENT_DESC*>(alloca(totalSize));
    std::memset(inputData, 0, totalSize);

    UINT stride = 0;

    for (size_t i = 0; i < size; ++i) {
        inputData[i].SemanticName      = elements[i].semanticName;
        inputData[i].SemanticIndex     = elements[i].semanticIndex;
        inputData[i].Format            = MapDataFormat[static_cast<uint64_t>(elements[i].format)];
        inputData[i].InputSlot         = elements[i].slot;
        inputData[i].AlignedByteOffset = static_cast<UINT>(elements[i].offset);
        inputData[i].InputSlotClass    = MapVertexElementType[static_cast<uint64_t>(elements[i].type)];
        if (elements[i].type == VertexElementType::PerVertex)
            inputData[i].InstanceDataStepRate = 0;
        else
            inputData[i].InstanceDataStepRate = 1;

        stride += dxFormatStride(elements[i].format);
    }

    // validate first
    if (FAILED(g_pd3dDevice->CreateInputLayout(inputData, size, shaderBytecode, shaderBytecodeSize, nullptr))) {
        if (errorReport != nullptr) errorReport("Warning: VertexFormat validation failed!");
    }

    ID3D11InputLayout* layout = nullptr;
    if (FAILED(g_pd3dDevice->CreateInputLayout(inputData, size, shaderBytecode, shaderBytecodeSize, &layout))) {
        if (errorReport != nullptr) errorReport("Failed to create vertex format!");
        return VertexFormatHandle::invalidHandle();
    }

    VertexFormatImpl* impl = new VertexFormatImpl;
    impl->inputLayout = layout;
    impl->stride      = stride;
    return VertexFormatHandle(impl);
}

void releaseVertexFormat(VertexFormatHandle handle)
{
    if (handle != VertexFormatHandle::invalidHandle()) {
        VertexFormatImpl* impl = static_cast<VertexFormatImpl*>(handle.value);
        impl->inputLayout->Release();
        delete impl;
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
    impl->vertexFormat = static_cast<VertexFormatImpl*>(desc.vertexFormat.value);

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

BufferHandle createBuffer(uint32_t flags, void* mem, size_t size, size_t stride)
{
    DXSharedBuffer* buffer = new DXSharedBuffer;

    D3D11_USAGE bufferUsage    = D3D11_USAGE_IMMUTABLE;
    UINT        bufferCPUFlags = 0;
    UINT        bufferBindFlag = 0;
    UINT        bufferMiscFlag = 0;

    bool isStructured = false;
    bool isUAV        = false;

    if (flags & BufferFlags::VertexBuffer) {
        bufferBindFlag = D3D11_BIND_VERTEX_BUFFER;
    }
    if (flags & BufferFlags::IndexBuffer) {
        bufferBindFlag = D3D11_BIND_INDEX_BUFFER;
    }

    if (flags & BufferFlags::StructuredBuffer) {
        bufferBindFlag  = D3D11_BIND_SHADER_RESOURCE;
        bufferMiscFlag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        isStructured    = true;
    }

    if (flags & BufferFlags::GPUWrite) {
        bufferUsage     = D3D11_USAGE_DEFAULT;
        bufferMiscFlag |= D3D11_BIND_UNORDERED_ACCESS;
        isUAV           = true;
    }
    if (flags & BufferFlags::CPURead) {
        bufferUsage     = D3D11_USAGE_DYNAMIC;
        bufferCPUFlags |= D3D11_CPU_ACCESS_READ;
    }
    if (flags & BufferFlags::CPUWrite) {
        bufferUsage     = D3D11_USAGE_DYNAMIC;
        bufferCPUFlags |= D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_BUFFER_DESC bufferDesc;
    std::memset(&bufferDesc, 0, sizeof(bufferDesc));

    bufferDesc.ByteWidth           = size;
    bufferDesc.Usage               = bufferUsage;
    bufferDesc.BindFlags           = bufferBindFlag;
    bufferDesc.CPUAccessFlags      = bufferCPUFlags;
    bufferDesc.MiscFlags           = bufferMiscFlag;
    bufferDesc.StructureByteStride = stride;

    D3D11_SUBRESOURCE_DATA bufferData;
    std::memset(&bufferData, 0, sizeof(bufferData));
    bufferData.pSysMem = mem;

    buffer->createFromDesc(bufferDesc, bufferData);

    if (isStructured) buffer->createView(size / stride);
    if (isUAV)        buffer->createUAV(size / stride);

    return BufferHandle(buffer);
}

void releaseBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        delete buffer;
    }
}

void* mapBuffer(BufferHandle handle, MapType type)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        D3D11_MAPPED_SUBRESOURCE mappedData;
        std::memset(&mappedData, 0, sizeof(mappedData));

        if (FAILED(g_pImmediateContext->Map(buffer->dataBuffer, 0, MapMapType[static_cast<uint32_t>(type)], 0, &mappedData))) {
            return nullptr;
        }

        return mappedData.pData;
    }
    return nullptr;
}

void unmapBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        g_pImmediateContext->Unmap(buffer->dataBuffer, 0);
    }
}

ConstantBufferHandle createConstantBuffer(void* mem, size_t size)
{
    D3D11_BUFFER_DESC bufferDesc;
    std::memset(&bufferDesc, 0, sizeof(bufferDesc));
    bufferDesc.ByteWidth           = size;
    bufferDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags      = 0;
    bufferDesc.MiscFlags           = 0;
    bufferDesc.StructureByteStride = size;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem          = mem;
    data.SysMemPitch      = 0;
    data.SysMemSlicePitch = 0;

    ID3D11Buffer* buffer = nullptr;
    if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, (mem == nullptr) ? nullptr : &data, &buffer))) {
        // TODO: error handling
        return ConstantBufferHandle::invalidHandle();
    }

    return ConstantBufferHandle(buffer);
}

void updateConstantBuffer(ConstantBufferHandle handle, void* mem)
{
    if (handle != ConstantBufferHandle::invalidHandle()) {
        ID3D11Buffer* buffer = static_cast<ID3D11Buffer*>(handle.value);

        g_pImmediateContext->UpdateSubresource(buffer, 0, nullptr, mem, 0, 0);
    }
}

void releaseConstantBuffer(ConstantBufferHandle handle)
{
    if (handle != ConstantBufferHandle::invalidHandle()) {
        ID3D11Buffer* buffer = static_cast<ID3D11Buffer*>(handle.value);
        buffer->Release();
    }
}

Texture1DHandle createTexture1D(uint32_t width, DataFormat format, size_t numMipmaps)
{
    D3D11_TEXTURE1D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.ArraySize      = 1;
    textureDesc.Format         = MapDataFormat[static_cast<uint32_t>(format)];
    textureDesc.Usage          = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags      = 0;

    ID3D11Texture1D* texture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture1D(&textureDesc, nullptr, &texture))) {
        // TODO: error handling
        return Texture1DHandle::invalidHandle();
    }

    return Texture1DHandle(texture);
}

Texture2DHandle createTexture2D(uint32_t width, uint32_t height, DataFormat format, size_t numMipmaps)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.Height         = height;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.ArraySize      = 1;
    textureDesc.Format         = MapDataFormat[static_cast<uint32_t>(format)];
    textureDesc.Usage          = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags      = 0;

    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;

    ID3D11Texture2D* texture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture))) {
        // TODO: error handling
        return Texture2DHandle::invalidHandle();
    }

    return Texture2DHandle(texture);
}

Texture3DHandle createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DataFormat format, size_t numMipmaps)
{
    D3D11_TEXTURE3D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.Height         = height;
    textureDesc.Depth          = depth;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.Format         = MapDataFormat[static_cast<uint32_t>(format)];
    textureDesc.Usage          = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags      = 0;

    ID3D11Texture3D* texture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture3D(&textureDesc, nullptr, &texture))) {
        // TODO: error handling
        return Texture3DHandle::invalidHandle();
    }

    return Texture3DHandle(texture);
}

void updateTexture(
    TextureHandle handle, void* mem,
    uint32_t mip,
    size_t offsetX, size_t sizeX,
    size_t offsetY, size_t sizeY,
    size_t offsetZ, size_t sizeZ
)
{
    if (handle != TextureHandle::invalidHandle()) {
        ID3D11Resource* resource = static_cast<ID3D11Resource*>(handle.value);

        D3D11_BOX box;
        box.left   = offsetX;
        box.right  = offsetX + sizeX;
        box.top    = offsetY;
        box.bottom = offsetY + sizeY;
        box.front  = offsetZ;
        box.back   = offsetZ + sizeZ;

        g_pImmediateContext->UpdateSubresource(resource, mip, &box, mem, sizeY, sizeZ);
    }
}

void releaseTexture(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        ID3D11Resource* resource = static_cast<ID3D11Resource*>(handle.value);
        resource->Release();
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

void setConstantBuffer(DrawQueueHandle handle, uint32_t idx, ConstantBufferHandle buffer)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setConstantBuffer(idx, buffer);
    }
}

void setResource(DrawQueueHandle handle, uint32_t idx, BufferHandle resource)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setResource(idx, resource);
    }
}

void setResourceRW(DrawQueueHandle handle, uint32_t idx, BufferHandle resource)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setResourceRW(idx, resource);
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

void drawInstanced(DrawQueueHandle handle, uint32_t instanceCount, uint32_t count, uint32_t startVertex)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->drawInstanced(instanceCount, count, startVertex);
    }
}

void drawIndexedInstanced(DrawQueueHandle handle, uint32_t instanceCount, uint32_t count, uint32_t startIndex, uint32_t startVertex)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->drawIndexedInstanced(instanceCount, count, startIndex, startVertex);
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

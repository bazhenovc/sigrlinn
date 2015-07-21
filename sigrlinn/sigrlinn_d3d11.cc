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
#include <d3d11.h>
#include <d3dcompiler.h>
#include <memory>

#ifndef SGFX_D3D11_INTEROP
#define SGFX_D3D11_INTEROP 1
#endif

#include "sigrlinn.hh"
#include "private/drawqueue.hh"
#include "private/computequeue.hh"

#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef SGFX_USE_D3D11_1
#include <d3d11_1.h>
#endif

namespace sgfx
{

static D3D11_MAP MapMapType[MapType::Count] = {
    D3D11_MAP_READ,
    D3D11_MAP_WRITE_DISCARD
};
static_assert((sizeof(MapMapType) / sizeof(D3D11_MAP)) == static_cast<uint32_t>(MapType::Count), "Mapping is broken!");

static D3D11_PRIMITIVE_TOPOLOGY MapPrimitiveTopology[PrimitiveTopology::Count] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
};
static_assert((sizeof(MapPrimitiveTopology) / sizeof(D3D11_PRIMITIVE_TOPOLOGY)) == static_cast<size_t>(PrimitiveTopology::Count), "Mapping is broken!");

static D3D11_FILTER MapTextureFilter[TextureFilter::Count] = {
    D3D11_FILTER_MIN_MAG_MIP_POINT,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
    D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
    D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    D3D11_FILTER_ANISOTROPIC
};
static_assert((sizeof(MapTextureFilter) / sizeof(D3D11_FILTER)) == static_cast<size_t>(TextureFilter::Count), "Mapping is broken!");

static D3D11_TEXTURE_ADDRESS_MODE MapAddressMode[AddressMode::Count] = {
    D3D11_TEXTURE_ADDRESS_WRAP,
    D3D11_TEXTURE_ADDRESS_MIRROR,
    D3D11_TEXTURE_ADDRESS_CLAMP,
    D3D11_TEXTURE_ADDRESS_BORDER
};
static_assert((sizeof(MapAddressMode) / sizeof(D3D11_TEXTURE_ADDRESS_MODE)) == static_cast<size_t>(AddressMode::Count), "Mapping is broken!");

static DXGI_FORMAT MapDataFormat[DataFormat::Count] = {
    DXGI_FORMAT_BC1_UNORM,     // DXT1
    DXGI_FORMAT_BC2_UNORM,     // DXT3
    DXGI_FORMAT_BC3_UNORM,     // DXT5
    DXGI_FORMAT_BC4_UNORM,     // LATC1/ATI1
    DXGI_FORMAT_BC5_UNORM,     // LATC2/ATI2
    DXGI_FORMAT_BC6H_TYPELESS, // BC6H
    DXGI_FORMAT_BC7_UNORM,     // BC7
    DXGI_FORMAT_UNKNOWN,       // ETC1 RGB8
    DXGI_FORMAT_UNKNOWN,       // ETC2 RGB8
    DXGI_FORMAT_UNKNOWN,       // ETC2 RGBA8
    DXGI_FORMAT_UNKNOWN,       // ETC2 RGB8A1
    DXGI_FORMAT_UNKNOWN,       // PVRTC1 RGB 2BPP
    DXGI_FORMAT_UNKNOWN,       // PVRTC1 RGB 4BPP
    DXGI_FORMAT_UNKNOWN,       // PVRTC1 RGBA 2BPP
    DXGI_FORMAT_UNKNOWN,       // PVRTC1 RGBA 4BPP
    DXGI_FORMAT_UNKNOWN,       // PVRTC2 RGBA 2BPP
    DXGI_FORMAT_UNKNOWN,       // PVRTC2 RGBA 4BPP

    DXGI_FORMAT_UNKNOWN,         // compressed formats above

    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32A32_UINT,
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

//=============================================================================
static inline void* sgfx_malloc(size_t size)
{
    return malloc(size);
}

static inline void sgfx_free(void* ptr)
{
    return free(ptr);
}

//=============================================================================

ID3D11Device*         g_pd3dDevice         = nullptr;
ID3D11DeviceContext*  g_pImmediateContext  = nullptr;
IDXGISwapChain*       g_pSwapChain         = nullptr;

AllocFunc             g_allocFunc = sgfx_malloc;
FreeFunc              g_freeFunc  = sgfx_free;

#ifdef SGFX_USE_D3D11_1
ID3DUserDefinedAnnotation* g_debugAnnotation = nullptr;
#endif

//=============================================================================
struct DXSharedBuffer final
{
    ID3D11Resource*            dataBuffer       = nullptr;
    ID3D11ShaderResourceView*  dataView         = nullptr;
    ID3D11UnorderedAccessView* dataUAV          = 0;
    size_t                     dataBufferSize   = 0;

    inline DXSharedBuffer() {}
    inline ~DXSharedBuffer()
    {
        if (dataBuffer != nullptr) dataBuffer->Release();
        if (dataView   != nullptr) dataView->Release();
        if (dataUAV    != nullptr) dataUAV->Release();
    }

    inline void createView(size_t numElements)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));
        viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
        viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
        viewDesc.Buffer.ElementWidth = static_cast<UINT>(numElements);

        if (FAILED(g_pd3dDevice->CreateShaderResourceView(dataBuffer, &viewDesc, &dataView))) {
            // TODO: error handling
            return;
        }
    }

    inline void createUAV(size_t numElements, bool isCounter)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        std::memset(&uavDesc, 0, sizeof(uavDesc));
        uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Format             = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = static_cast<UINT>(numElements);

        if (isCounter)
            uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;

        if (FAILED(g_pd3dDevice->CreateUnorderedAccessView(dataBuffer, &uavDesc, &dataUAV))) {
            // TODO: error handling
            return;
        }
    }
};

//=============================================================================
struct DXStateCache final
{
    enum
    {
        SC_Draw = 0,
        SC_Compute = 1
    };

    uint32_t type;

    ID3D11SamplerState*         samplerStates[internal::DrawQueue::kMaxSamplerStates];
    ID3D11Buffer*               constantBuffers[internal::DrawCall::kMaxConstantBuffers];
    ID3D11ShaderResourceView*   shaderResourceViews[internal::DrawCall::kMaxShaderResources];
    ID3D11UnorderedAccessView*  shaderUAVs[internal::ComputeQueue::kMaxShaderResourcesRW];
    uint32_t                    shaderUAVCounters[internal::ComputeQueue::kMaxShaderResourcesRW];

    bool vs = false;
    bool hs = false;
    bool ds = false;
    bool gs = false;
    bool ps = false;

    inline DXStateCache(uint32_t newType)
    {
        clear();

        type = newType;
    }

    inline void clear()
    {
        std::memset(samplerStates, 0, sizeof(samplerStates));
        std::memset(constantBuffers, 0, sizeof(constantBuffers));
        std::memset(shaderResourceViews, 0, sizeof(shaderResourceViews));
        std::memset(shaderUAVs, 0, sizeof(shaderUAVs));
        std::memset(shaderUAVCounters, 0, sizeof(shaderUAVCounters));
    }

    inline void setSamplerStates(const SamplerStateHandle* handles)
    {
        for (size_t i = 0; i < internal::DrawQueue::kMaxSamplerStates; ++i) {
            ID3D11SamplerState* state = static_cast<ID3D11SamplerState*>(handles[i].value);
            if (state != samplerStates[i]) {
                samplerStates[i] = state;

                if (type == SC_Draw) {
                    if (vs) g_pImmediateContext->VSSetSamplers(i, 1, &state);
                    if (hs) g_pImmediateContext->HSSetSamplers(i, 1, &state);
                    if (ds) g_pImmediateContext->DSSetSamplers(i, 1, &state);
                    if (gs) g_pImmediateContext->GSSetSamplers(i, 1, &state);
                    if (ps) g_pImmediateContext->PSSetSamplers(i, 1, &state);
                }

                if (type == SC_Compute)
                    g_pImmediateContext->CSSetSamplers(i, 1, &state);
            }
        }
    }

    inline void setConstantBuffers(const ConstantBufferHandle* handles)
    {
        for (size_t i = 0; i < internal::DrawCall::kMaxConstantBuffers; ++i) {
            ID3D11Buffer* state = static_cast<ID3D11Buffer*>(handles[i].value);
            if (state != constantBuffers[i]) {
                constantBuffers[i] = state;

                if (type == SC_Draw) {
                    if (vs) g_pImmediateContext->VSSetConstantBuffers(i, 1, &state);
                    if (hs) g_pImmediateContext->HSSetConstantBuffers(i, 1, &state);
                    if (ds) g_pImmediateContext->DSSetConstantBuffers(i, 1, &state);
                    if (gs) g_pImmediateContext->GSSetConstantBuffers(i, 1, &state);
                    if (ps) g_pImmediateContext->PSSetConstantBuffers(i, 1, &state);
                }

                if (type == SC_Compute)
                    g_pImmediateContext->CSSetConstantBuffers(i, 1, &state);
            }
        }
    }

    inline void setShaderResources(const internal::ShaderResource* resources)
    {
        for (size_t i = 0; i < internal::DrawCall::kMaxShaderResources; ++i) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(resources[i].value);

            ID3D11ShaderResourceView* state = nullptr;
            if (buffer != nullptr)
                state = buffer->dataView;

            if (state != shaderResourceViews[i]) {
                shaderResourceViews[i] = state;

                if (type == SC_Draw) {
                    if (vs) g_pImmediateContext->VSSetShaderResources(i, 1, &state);
                    if (hs) g_pImmediateContext->HSSetShaderResources(i, 1, &state);
                    if (ds) g_pImmediateContext->DSSetShaderResources(i, 1, &state);
                    if (gs) g_pImmediateContext->GSSetShaderResources(i, 1, &state);
                    if (ps) g_pImmediateContext->PSSetShaderResources(i, 1, &state);
                }

                if (type == SC_Compute)
                    g_pImmediateContext->CSSetShaderResources(i, 1, &state);
            }
        }
    }

    inline void setShaderResourcesRW(const internal::ShaderResource* resources)
    {
        for (size_t i = 0; i < internal::ComputeQueue::kMaxShaderResourcesRW; ++i) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(resources[i].value);

            ID3D11UnorderedAccessView* state = nullptr;
            if (buffer != nullptr)
                state = buffer->dataUAV;

            if (state != shaderUAVs[i]) {
                shaderUAVs[i] = state;

                if (type == SC_Compute)
                    g_pImmediateContext->CSSetUnorderedAccessViews(i, 1, &state, shaderUAVCounters);
            }
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

    // state cache
    DXStateCache             stateCache = DXStateCache(DXStateCache::SC_Draw);
};

struct RenderTargetImpl final
{
    UINT                        numRenderTargets = 0;
    ID3D11RenderTargetView*     renderTargetViews[RenderTargetSlot::Count];
    ID3D11DepthStencilView*     depthStencilView = nullptr;
    ID3D11UnorderedAccessView*  unorderedAccessViews[RenderTargetSlot::Count];
    uint32_t                    uavCounters[RenderTargetSlot::Count];

    inline RenderTargetImpl()
    {
        std::memset(renderTargetViews, 0, sizeof(renderTargetViews));
        std::memset(unorderedAccessViews, 0, sizeof(unorderedAccessViews));
        std::memset(uavCounters, 0, sizeof(uavCounters));
    }

    inline ~RenderTargetImpl()
    {
        for (size_t i = 0; i < RenderTargetSlot::Count; ++i) {
            if (renderTargetViews[i] != nullptr)
                renderTargetViews[i]->Release();
            //if (unorderedAccessViews[i] != nullptr)
            //    unorderedAccessViews[i]->Release();
        }
        if (depthStencilView != nullptr)
            depthStencilView->Release();
    }
};

static inline UINT dxFormatStride(DataFormat format)
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
    case DataFormat::RG32I:   { return 8;  } break;
    case DataFormat::RG32U:   { return 8;  } break;
    case DataFormat::RG32F:   { return 8;  } break;
    case DataFormat::RGB32I:  { return 12; } break;
    case DataFormat::RGB32U:  { return 12; } break;
    case DataFormat::RGB32F:  { return 12; } break;
    case DataFormat::RGBA8:   { return 4;  } break;
    case DataFormat::RGBA16:  { return 8;  } break;
    case DataFormat::RGBA16F: { return 8;  } break;
    case DataFormat::RGBA32I: { return 16; } break;
    case DataFormat::RGBA32U: { return 16; } break;
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

    psimpl->stateCache.setSamplerStates(queue->samplerStates);

    // process draw calls
    for (const internal::DrawCall& call: queue->getDrawCalls()) {
        DXSharedBuffer* vertexBuffer = static_cast<DXSharedBuffer*>(call.vertexBuffer.value);
        DXSharedBuffer* indexBuffer  = static_cast<DXSharedBuffer*>(call.indexBuffer.value);
        UINT            offset       = 0;

        g_pImmediateContext->IASetPrimitiveTopology(MapPrimitiveTopology[static_cast<size_t>(call.primitiveTopology)]);
        if (psimpl->vertexFormat != nullptr) {
            ID3D11Buffer* vbuffer = nullptr;
            if (vertexBuffer != nullptr)
                vbuffer = static_cast<ID3D11Buffer*>(vertexBuffer->dataBuffer);

            ID3D11Buffer* ibuffer = nullptr;
            if (indexBuffer != nullptr)
                ibuffer = static_cast<ID3D11Buffer*>(indexBuffer->dataBuffer);

            g_pImmediateContext->IASetVertexBuffers(0, 1, &vbuffer, &psimpl->vertexFormat->stride, &offset);
            g_pImmediateContext->IASetIndexBuffer(ibuffer, DXGI_FORMAT_R32_UINT, 0); // TODO: different index format
        }

        // constant buffers
        psimpl->stateCache.setConstantBuffers(call.constantBuffers);

        // shader resources and textures
        psimpl->stateCache.setShaderResources(call.shaderResources);

        switch (call.type) {
        case internal::DrawCall::Draw:                 { g_pImmediateContext->Draw(call.count, call.startVertex); } break;
        case internal::DrawCall::DrawIndexed:          { g_pImmediateContext->DrawIndexed(call.count, call.startIndex, call.startVertex); } break;
        case internal::DrawCall::DrawInstanced:        { g_pImmediateContext->DrawInstanced(call.count, call.instanceCount, call.startVertex, 0); } break;
        case internal::DrawCall::DrawIndexedInstanced: { g_pImmediateContext->DrawIndexedInstanced(call.count, call.instanceCount, call.startIndex, call.startVertex, 0); } break;
        }
    }

    psimpl->stateCache.clear();
}

template <typename T, typename ...Args>
static inline T* sgfx_new(Args&... args)
{
    return new (g_allocFunc(sizeof(T))) T(static_cast<Args&&>(args)...);
}

template <typename T>
static inline void sgfx_delete(T* t)
{
    t->~T();
    g_freeFunc(t);
}

//=============================================================================
bool initD3D11(void* d3dDevice, void* d3dContext, void* d3dSwapChain)
{
    g_pd3dDevice        = static_cast<ID3D11Device*>(d3dDevice);
    g_pImmediateContext = static_cast<ID3D11DeviceContext*>(d3dContext);
    g_pSwapChain        = static_cast<IDXGISwapChain*>(d3dSwapChain);

#ifdef SGFX_USE_D3D11_1
    HRESULT hr = g_pImmediateContext->QueryInterface(&g_debugAnnotation);
    if (FAILED(hr))
        g_debugAnnotation = nullptr; // probably redundant
#endif

    return true;
}

void shutdown()
{
#ifdef SGFX_USE_D3D11_1
    if (g_debugAnnotation)
        g_debugAnnotation->Release();
#endif
}

void setAllocator(AllocFunc nalloc, FreeFunc nfree)
{
    g_allocFunc = nalloc;
    g_freeFunc  = nfree;
}

void* allocate(size_t size)
{
    return g_allocFunc(size);
}

void deallocate(void* ptr)
{
    return g_freeFunc(ptr);
}

uint64_t getGPUCaps()
{
    uint64_t caps = 0;

    D3D_FEATURE_LEVEL featureLevel = g_pd3dDevice->GetFeatureLevel();

    // default features
    caps |= GPUCaps::MultipleRenderTargets;
    caps |= GPUCaps::StreamOutput;
    caps |= GPUCaps::AlphaToCoverage;
    caps |= GPUCaps::SeparateBlend;
    caps |= GPUCaps::StructuredBuffer;
    caps |= GPUCaps::RWStructuredBuffer;

    caps |= GPUCaps::TextureCompressionDXT;
    caps |= GPUCaps::TextureFormatInteger;
    caps |= GPUCaps::TextureFormatFloat;

    if (featureLevel >= D3D_FEATURE_LEVEL_10_0) {
        caps |= GPUCaps::GeometryShader;
        caps |= GPUCaps::ComputeShader;
        caps |= GPUCaps::TextureArray;
    }

    if (featureLevel >= D3D_FEATURE_LEVEL_10_1) {
        caps |= GPUCaps::CubemapArray;
    }

    if (featureLevel >= D3D_FEATURE_LEVEL_11_0) {
        caps |= GPUCaps::TessellationShader;
    }

    return caps;
}

//-------------------------------------------------------------------------------------------------
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
        size_t totalSize = macrosSize * sizeof(D3D_SHADER_MACRO) + sizeof(D3D_SHADER_MACRO);
        d3dmacros = reinterpret_cast<D3D_SHADER_MACRO*>(allocate(totalSize));
        std::memset(d3dmacros, 0, totalSize);

        for (size_t i = 0; i < macrosSize; ++i) {
            d3dmacros[i].Name       = macros[i].name;
            d3dmacros[i].Definition = macros[i].value;
        }
    }

    ID3DBlob* outBlob   = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT d3dFlags = 0;
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
        if (errorReport != nullptr && errorBlob != nullptr) {
            char* error = reinterpret_cast<char*>(allocate(errorBlob->GetBufferSize() + 1));
            std::memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            error[errorBlob->GetBufferSize()] = '\0';
            errorReport(error);
            deallocate(error);
        }
        if (errorBlob != nullptr) errorBlob->Release();
        if (d3dmacros != nullptr) deallocate(d3dmacros);
        return false;
    }

    outDataSize = outBlob->GetBufferSize();
    outData = reinterpret_cast<uint8_t*>(allocate(outDataSize));
    std::memcpy(outData, outBlob->GetBufferPointer(), outDataSize);
    outBlob->Release();

    if (d3dmacros != nullptr) deallocate(d3dmacros);
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
    SurfaceShaderImpl* impl = sgfx::sgfx_new<SurfaceShaderImpl>();
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
        sgfx_delete(impl);
    }
}

ComputeQueueHandle createComputeQueue(ComputeShaderHandle shader)
{
    internal::ComputeQueue* queue = sgfx::sgfx_new<internal::ComputeQueue>();
    queue->shader = shader;

    return ComputeQueueHandle(queue);
}

void releaseComputeQueue(ComputeQueueHandle handle)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue = static_cast<internal::ComputeQueue*>(handle.value);
        sgfx_delete(queue);
    }
}

void setConstantBuffer(ComputeQueueHandle handle, uint32_t idx, ConstantBufferHandle buffer)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue = static_cast<internal::ComputeQueue*>(handle.value);
        queue->setConstantBuffer(idx, buffer);
    }
}

void setResource(ComputeQueueHandle handle, uint32_t idx, BufferHandle resource)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue = static_cast<internal::ComputeQueue*>(handle.value);
        queue->setResource(idx, resource);
    }
}

void setResource(ComputeQueueHandle handle, uint32_t idx, TextureHandle resource)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue = static_cast<internal::ComputeQueue*>(handle.value);
        queue->setResource(idx, resource);
    }
}

void setResourceRW(ComputeQueueHandle handle, uint32_t idx, BufferHandle resource)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue = static_cast<internal::ComputeQueue*>(handle.value);
        queue->setResourceRW(idx, resource);
    }
}

void submit(ComputeQueueHandle handle, uint32_t x, uint32_t y, uint32_t z)
{
    if (handle != ComputeQueueHandle::invalidHandle()) {
        internal::ComputeQueue* queue   = static_cast<internal::ComputeQueue*>(handle.value);
        ID3D11ComputeShader*    shader  = static_cast<ID3D11ComputeShader*>(queue->shader.value);

        // constant buffers are ID3D11Buffers effectively
        ID3D11Buffer* constantBuffers[internal::ComputeQueue::kMaxConstantBuffers] = { nullptr };
        for (size_t i = 0; i < internal::ComputeQueue::kMaxConstantBuffers; ++i)
            constantBuffers[i] = static_cast<ID3D11Buffer*>(queue->constantBuffers[i].value);

        // TODO: add statecache here!
        g_pImmediateContext->CSSetConstantBuffers(0, internal::ComputeQueue::kMaxConstantBuffers, constantBuffers);

        // shader resources and textures
        ID3D11ShaderResourceView* shaderResources[internal::ComputeQueue::kMaxShaderResources] = { nullptr };
        for (size_t i = 0; i < internal::ComputeQueue::kMaxShaderResources; ++i) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(queue->shaderResources[i].value);
            if (buffer != nullptr)
                shaderResources[i] = buffer->dataView;
        }

        // TODO: add statecache here!
        g_pImmediateContext->CSSetShaderResources(0, internal::ComputeQueue::kMaxShaderResources, shaderResources);

        // UAVs
        ID3D11UnorderedAccessView* shaderUAVs[internal::ComputeQueue::kMaxShaderResourcesRW] = { nullptr };
        for (size_t i = 0; i < internal::ComputeQueue::kMaxShaderResourcesRW; ++i) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(queue->shaderResourcesRW[i].value);
            if (buffer != nullptr)
                shaderUAVs[i] = buffer->dataUAV;
        }

        // TODO: add statecache here!
        uint32_t shaderUAVCounters[internal::ComputeQueue::kMaxShaderResourcesRW] = { 0 };
        g_pImmediateContext->CSSetUnorderedAccessViews(0, internal::ComputeQueue::kMaxShaderResourcesRW, shaderUAVs, shaderUAVCounters);

        // dispatch
        g_pImmediateContext->CSSetShader(shader, nullptr, 0);
        g_pImmediateContext->Dispatch(x, y, z);
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
        inputData[i].SemanticName         = elements[i].semanticName;
        inputData[i].SemanticIndex        = elements[i].semanticIndex;
        inputData[i].Format               = MapDataFormat[static_cast<size_t>(elements[i].format)];
        inputData[i].InputSlot            = elements[i].slot;
        inputData[i].AlignedByteOffset    = static_cast<UINT>(elements[i].offset);
        inputData[i].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
        inputData[i].InstanceDataStepRate = 0;

        stride += dxFormatStride(elements[i].format);
    }

    // validate first
    if (FAILED(g_pd3dDevice->CreateInputLayout(inputData, static_cast<UINT>(size), shaderBytecode, shaderBytecodeSize, nullptr))) {
        if (errorReport != nullptr) errorReport("Warning: VertexFormat validation failed!");
    }

    ID3D11InputLayout* layout = nullptr;
    if (FAILED(g_pd3dDevice->CreateInputLayout(inputData, static_cast<UINT>(size), shaderBytecode, shaderBytecodeSize, &layout))) {
        if (errorReport != nullptr) errorReport("Failed to create vertex format!");
        return VertexFormatHandle::invalidHandle();
    }

    VertexFormatImpl* impl = sgfx_new<VertexFormatImpl>();
    impl->inputLayout = layout;
    impl->stride      = stride;
    return VertexFormatHandle(impl);
}

void releaseVertexFormat(VertexFormatHandle handle)
{
    if (handle != VertexFormatHandle::invalidHandle()) {
        VertexFormatImpl* impl = static_cast<VertexFormatImpl*>(handle.value);
        impl->inputLayout->Release();
        sgfx_delete(impl);
    }
}

PipelineStateHandle createPipelineState(const PipelineStateDescriptor& desc)
{
    const RasterizerState& rsState = desc.rasterizerState;

    D3D11_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = MapFillMode[static_cast<size_t>(rsState.fillMode)];
    rasterizerDesc.CullMode              = MapCullMode[static_cast<size_t>(rsState.cullMode)];
    rasterizerDesc.FrontCounterClockwise = MapCounterDirection[static_cast<size_t>(rsState.counterDirection)];
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
        blendDesc.RenderTarget[0].RenderTargetWriteMask = static_cast<UINT8>(bsState.renderTargetBlendDesc[0].writeMask);
        blendDesc.RenderTarget[0].SrcBlend              = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.srcBlend)];
        blendDesc.RenderTarget[0].DestBlend             = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.dstBlend)];
        blendDesc.RenderTarget[0].BlendOp               = MapBlendOp[static_cast<size_t>(bsState.blendDesc.blendOp)];
        blendDesc.RenderTarget[0].SrcBlendAlpha         = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.srcBlendAlpha)];
        blendDesc.RenderTarget[0].DestBlendAlpha        = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.dstBlendAlpha)];
        blendDesc.RenderTarget[0].BlendOpAlpha          = MapBlendOp[static_cast<size_t>(bsState.blendDesc.blendOpAlpha)];
    //}

    if (bsState.separateBlendEnabled) {
        for (size_t i = 0; i < 8; ++i) {
            blendDesc.RenderTarget[i].BlendEnable           = bsState.renderTargetBlendDesc[i].blendEnabled;
            blendDesc.RenderTarget[i].RenderTargetWriteMask = static_cast<UINT8>(bsState.renderTargetBlendDesc[i].writeMask);
            blendDesc.RenderTarget[i].SrcBlend              = MapBlendFactor[static_cast<size_t>(bsState.renderTargetBlendDesc[i].srcBlend)];
            blendDesc.RenderTarget[i].DestBlend             = MapBlendFactor[static_cast<size_t>(bsState.renderTargetBlendDesc[i].dstBlend)];
            blendDesc.RenderTarget[i].BlendOp               = MapBlendOp[static_cast<size_t>(bsState.renderTargetBlendDesc[i].blendOp)];
            blendDesc.RenderTarget[i].SrcBlendAlpha         = MapBlendFactor[static_cast<size_t>(bsState.renderTargetBlendDesc[i].srcBlendAlpha)];
            blendDesc.RenderTarget[i].DestBlendAlpha        = MapBlendFactor[static_cast<size_t>(bsState.renderTargetBlendDesc[i].dstBlendAlpha)];
            blendDesc.RenderTarget[i].BlendOpAlpha          = MapBlendOp[static_cast<size_t>(bsState.renderTargetBlendDesc[i].blendOpAlpha)];
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
    depthStencilDesc.DepthWriteMask               = MapDepthWriteMask[static_cast<size_t>(dsState.writeMask)];
    depthStencilDesc.DepthFunc                    = MapComparisonFunc[static_cast<size_t>(dsState.depthFunc)];
    depthStencilDesc.StencilEnable                = dsState.stencilEnabled;
    depthStencilDesc.StencilReadMask              = dsState.stencilReadMask;
    depthStencilDesc.StencilWriteMask             = dsState.stencilWriteMask;
    depthStencilDesc.FrontFace.StencilFunc        = MapComparisonFunc[static_cast<size_t>(dsState.frontFaceStencilDesc.stencilFunc)];
    depthStencilDesc.FrontFace.StencilFailOp      = MapStencilOp[static_cast<size_t>(dsState.frontFaceStencilDesc.failOp)];
    depthStencilDesc.FrontFace.StencilDepthFailOp = MapStencilOp[static_cast<size_t>(dsState.frontFaceStencilDesc.depthFailOp)];
    depthStencilDesc.FrontFace.StencilPassOp      = MapStencilOp[static_cast<size_t>(dsState.frontFaceStencilDesc.passOp)];
    depthStencilDesc.BackFace.StencilFunc         = MapComparisonFunc[static_cast<size_t>(dsState.backFaceStencilDesc.stencilFunc)];
    depthStencilDesc.BackFace.StencilFailOp       = MapStencilOp[static_cast<size_t>(dsState.backFaceStencilDesc.failOp)];
    depthStencilDesc.BackFace.StencilDepthFailOp  = MapStencilOp[static_cast<size_t>(dsState.backFaceStencilDesc.depthFailOp)];
    depthStencilDesc.BackFace.StencilPassOp       = MapStencilOp[static_cast<size_t>(dsState.backFaceStencilDesc.passOp)];

    ID3D11DepthStencilState* depthStencilState = nullptr;
    if (FAILED(g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &depthStencilState))) {
        rasterizerState->Release();
        blendState->Release();
        return PipelineStateHandle::invalidHandle();
    }

    PipelineStateImpl* impl = sgfx::sgfx_new<PipelineStateImpl>();
    impl->rasterizerState   = rasterizerState;
    impl->blendState        = blendState;
    impl->depthStencilState = depthStencilState;

    impl->shader       = static_cast<SurfaceShaderImpl*>(desc.shader.value);
    impl->vertexFormat = static_cast<VertexFormatImpl*>(desc.vertexFormat.value);

    impl->stencilRef = dsState.stencilRef;

    impl->stateCache.vs = impl->shader->vs != nullptr;
    impl->stateCache.hs = impl->shader->hs != nullptr;
    impl->stateCache.ds = impl->shader->ds != nullptr;
    impl->stateCache.gs = impl->shader->gs != nullptr;
    impl->stateCache.ps = impl->shader->ps != nullptr;

    return PipelineStateHandle(impl);
}

void releasePipelineState(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        PipelineStateImpl* impl = static_cast<PipelineStateImpl*>(handle.value);
        impl->rasterizerState->Release();
        impl->blendState->Release();
        impl->depthStencilState->Release();
        sgfx::sgfx_delete(impl);
    }
}

BufferHandle createBuffer(uint32_t flags, const void* mem, size_t size, size_t stride)
{
    D3D11_USAGE bufferUsage    = D3D11_USAGE_IMMUTABLE;
    UINT        bufferCPUFlags = 0;
    UINT        bufferBindFlag = 0;
    UINT        bufferMiscFlag = 0;

    bool isStructured = false;
    bool isUAV        = false;
    bool isCounter    = false;

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
        bufferBindFlag |= D3D11_BIND_UNORDERED_ACCESS;
        bufferMiscFlag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        isUAV           = true;

        if (flags & BufferFlags::GPUCounter)
            isCounter = true;
    }
    if (flags & BufferFlags::CPURead) {
        bufferUsage     = D3D11_USAGE_STAGING;
        bufferCPUFlags |= D3D11_CPU_ACCESS_READ;
    }
    if (flags & BufferFlags::CPUWrite) {
        bufferUsage     = D3D11_USAGE_DYNAMIC;
        bufferCPUFlags |= D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_BUFFER_DESC bufferDesc;
    std::memset(&bufferDesc, 0, sizeof(bufferDesc));

    bufferDesc.ByteWidth           = static_cast<UINT>(size);
    bufferDesc.Usage               = bufferUsage;
    bufferDesc.BindFlags           = bufferBindFlag;
    bufferDesc.CPUAccessFlags      = bufferCPUFlags;
    bufferDesc.MiscFlags           = bufferMiscFlag;
    bufferDesc.StructureByteStride = static_cast<UINT>(stride);

    D3D11_SUBRESOURCE_DATA bufferData;
    std::memset(&bufferData, 0, sizeof(bufferData));
    bufferData.pSysMem = mem;

    ID3D11Buffer* d3dbuffer = nullptr;
    if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, (bufferData.pSysMem == nullptr) ? nullptr : &bufferData, &d3dbuffer))) {
        // TODO: error handling
        return BufferHandle::invalidHandle();
    }

    DXSharedBuffer* buffer = sgfx_new<DXSharedBuffer>();
    buffer->dataBuffer = d3dbuffer;

    if (isStructured) buffer->createView(size / stride);
    if (isUAV)        buffer->createUAV(size / stride, isCounter);

    return BufferHandle(buffer);
}

void releaseBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        sgfx::sgfx_delete(buffer);
    }
}

void* mapBuffer(BufferHandle handle, MapType type)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        D3D11_MAPPED_SUBRESOURCE mappedData;
        std::memset(&mappedData, 0, sizeof(mappedData));

        if (FAILED(g_pImmediateContext->Map(buffer->dataBuffer, 0, MapMapType[static_cast<size_t>(type)], 0, &mappedData))) {
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

void copyBufferData(BufferHandle handle, size_t offset, size_t size, const void* mem)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        D3D11_BOX box;
        box.left   = static_cast<UINT>(offset);
        box.right  = static_cast<UINT>(offset + size);
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        g_pImmediateContext->UpdateSubresource(
            buffer->dataBuffer,
            0,
            &box, 
            mem,
            0, 0
        );
    }
}

void clearBufferRW(BufferHandle handle, uint32_t value)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        uint32_t d3dValues[4] = { value, 0, 0, 0 };

        if (buffer->dataUAV != nullptr)
            g_pImmediateContext->ClearUnorderedAccessViewUint(buffer->dataUAV, d3dValues);
    }
}

void clearBufferRW(BufferHandle handle, float value)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        float d3dValues[4] = { value, 0.F, 0.0F, 0.0F };

        if (buffer->dataUAV != nullptr)
            g_pImmediateContext->ClearUnorderedAccessViewFloat(buffer->dataUAV, d3dValues);
    }
}

ConstantBufferHandle createConstantBuffer(const void* mem, size_t size)
{
    D3D11_BUFFER_DESC bufferDesc;
    std::memset(&bufferDesc, 0, sizeof(bufferDesc));
    bufferDesc.ByteWidth           = static_cast<UINT>(size);
    bufferDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags      = 0;
    bufferDesc.MiscFlags           = 0;
    bufferDesc.StructureByteStride = static_cast<UINT>(size);

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

void updateConstantBuffer(ConstantBufferHandle handle, const void* mem)
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

SamplerStateHandle createSamplerState(const SamplerStateDescriptor& desc)
{
    D3D11_SAMPLER_DESC samplerDesc;
    std::memset(&samplerDesc, 0, sizeof(samplerDesc));

    samplerDesc.Filter         = MapTextureFilter[static_cast<size_t>(desc.filter)];
    samplerDesc.AddressU       = MapAddressMode[static_cast<size_t>(desc.addressU)];
    samplerDesc.AddressV       = MapAddressMode[static_cast<size_t>(desc.addressV)];
    samplerDesc.AddressW       = MapAddressMode[static_cast<size_t>(desc.addressW)];
    samplerDesc.MipLODBias     = desc.lodBias;
    samplerDesc.MaxAnisotropy  = desc.maxAnisotropy;
    samplerDesc.ComparisonFunc = MapComparisonFunc[static_cast<uint32_t>(desc.comparisonFunc)];
    samplerDesc.BorderColor[0] = static_cast<float>((desc.borderColor >> 0)  & 0xFF) / 255.0F;
    samplerDesc.BorderColor[1] = static_cast<float>((desc.borderColor >> 8)  & 0xFF) / 255.0F;
    samplerDesc.BorderColor[2] = static_cast<float>((desc.borderColor >> 16) & 0xFF) / 255.0F;
    samplerDesc.BorderColor[3] = static_cast<float>((desc.borderColor >> 24) & 0xFF) / 255.0F;
    samplerDesc.MinLOD         = desc.minLod;
    samplerDesc.MaxLOD         = desc.maxLod;

    ID3D11SamplerState* sampler = nullptr;
    if (FAILED(g_pd3dDevice->CreateSamplerState(&samplerDesc, &sampler))) {
        // TODO: error handling
        return SamplerStateHandle::invalidHandle();
    }

    return SamplerStateHandle(sampler);
}

void releaseSamplerState(SamplerStateHandle handle)
{
    if (handle != SamplerStateHandle::invalidHandle()) {
        ID3D11SamplerState* samplerState = static_cast<ID3D11SamplerState*>(handle.value);
        samplerState->Release();
    }
}

Texture1DHandle createTexture1D(uint32_t width, DataFormat format, uint32_t numMipmaps, uint32_t flags)
{
    UINT        bindFlags   = D3D11_BIND_SHADER_RESOURCE;
    D3D11_USAGE usageFlags  = D3D11_USAGE_DEFAULT;
    UINT        cpuAccess   = 0;
    bool        isStaging   = false;
    bool        isUAV       = false;

    if (flags & TextureFlags::RenderTarget)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (flags & TextureFlags::DepthStencil)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    if (flags & TextureFlags::CPURead) {
        bindFlags   = 0;
        usageFlags  = D3D11_USAGE_STAGING;
        cpuAccess   = D3D11_CPU_ACCESS_READ;
        isStaging   = true;
    }

    if (flags & TextureFlags::GPUWrite) {
        bindFlags  |= D3D11_BIND_UNORDERED_ACCESS;
        isUAV       = true;
    }

    D3D11_TEXTURE1D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.ArraySize      = 1;
    textureDesc.Format         = MapDataFormat[static_cast<size_t>(format)];
    textureDesc.Usage          = usageFlags;
    textureDesc.BindFlags      = bindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags      = 0;

    ID3D11Texture1D* d3dTexture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture1D(&textureDesc, nullptr, &d3dTexture))) {
        // TODO: error handling
        return Texture1DHandle::invalidHandle();
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
    std::memset(&viewDesc, 0, sizeof(viewDesc));

    viewDesc.Format                    = MapDataFormat[static_cast<size_t>(format)];
    viewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE1D;
    viewDesc.Texture1D.MipLevels       = numMipmaps;
    //viewDesc.Texture1D.MostDetailedMip = -1;

    ID3D11ShaderResourceView* d3dResourceView = nullptr;
    if (!isStaging) {
        if (FAILED(g_pd3dDevice->CreateShaderResourceView(d3dTexture, &viewDesc, &d3dResourceView))) {
            // TODO: error handling
            d3dTexture->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    std::memset(&uavDesc, 0, sizeof(uavDesc));

    uavDesc.Format                      = MapDataFormat[static_cast<size_t>(format)];
    uavDesc.ViewDimension               = D3D11_UAV_DIMENSION_TEXTURE1D;
    uavDesc.Texture1D.MipSlice          = 0; // TODO: handle this!

    ID3D11UnorderedAccessView* d3dUAV = nullptr;
    if (isUAV) {
        if (FAILED(g_pd3dDevice->CreateUnorderedAccessView(d3dTexture, &uavDesc, &d3dUAV))) {
            // TODO: error handling
            d3dTexture->Release();
            if (d3dResourceView != nullptr)
                d3dResourceView->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    DXSharedBuffer* texture = sgfx::sgfx_new<DXSharedBuffer>();
    texture->dataBuffer = d3dTexture;
    texture->dataView   = d3dResourceView;
    texture->dataUAV    = d3dUAV;

    return Texture1DHandle(texture);
}

Texture2DHandle createTexture2D(uint32_t width, uint32_t height, DataFormat format, uint32_t numMipmaps, uint32_t flags)
{
    UINT        bindFlags   = D3D11_BIND_SHADER_RESOURCE;
    D3D11_USAGE usageFlags  = D3D11_USAGE_DEFAULT;
    UINT        cpuAccess   = 0;
    bool        isStaging   = false;
    bool        isUAV       = false;

    if (flags & TextureFlags::RenderTarget)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (flags & TextureFlags::DepthStencil)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    if (flags & TextureFlags::CPURead) {
        bindFlags   = 0;
        usageFlags  = D3D11_USAGE_STAGING;
        cpuAccess   = D3D11_CPU_ACCESS_READ;
        isStaging   = true;
    }

    if (flags & TextureFlags::GPUWrite) {
        bindFlags  |= D3D11_BIND_UNORDERED_ACCESS;
        isUAV       = true;
    }

    DXGI_FORMAT dataFormat = MapDataFormat[static_cast<size_t>(format)];

    DXGI_FORMAT textureFormat = dataFormat;
    DXGI_FORMAT viewFormat    = dataFormat;

    if (format == DataFormat::D16) {
        textureFormat = DXGI_FORMAT_R16_TYPELESS;
        viewFormat    = DXGI_FORMAT_R16_UNORM;
    }

    if (format == DataFormat::D24S8) {
        // TODO: X8 part of the texture is not accessible ATM, try to handle this properly
        textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
        viewFormat    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    }

    if (format == DataFormat::D32F) {
        textureFormat = DXGI_FORMAT_R32_TYPELESS;
        viewFormat    = DXGI_FORMAT_R32_FLOAT;
    }

    D3D11_TEXTURE2D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.Height         = height;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.ArraySize      = 1;
    textureDesc.Format         = textureFormat;
    textureDesc.Usage          = usageFlags;
    textureDesc.BindFlags      = bindFlags;
    textureDesc.CPUAccessFlags = cpuAccess;
    textureDesc.MiscFlags      = 0;

    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;

    ID3D11Texture2D* d3dTexture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &d3dTexture))) {
        // TODO: error handling
        return Texture1DHandle::invalidHandle();
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
    std::memset(&viewDesc, 0, sizeof(viewDesc));

    viewDesc.Format                    = viewFormat;
    viewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels       = numMipmaps;
    //viewDesc.Texture2D.MostDetailedMip = -1;

    ID3D11ShaderResourceView* d3dResourceView = nullptr;
    if (!isStaging) {
        if (FAILED(g_pd3dDevice->CreateShaderResourceView(d3dTexture, &viewDesc, &d3dResourceView))) {
            // TODO: error handling
            d3dTexture->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    std::memset(&uavDesc, 0, sizeof(uavDesc));

    uavDesc.Format                      = MapDataFormat[static_cast<size_t>(format)];
    uavDesc.ViewDimension               = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice          = 0; // TODO: handle this!

    ID3D11UnorderedAccessView* d3dUAV = nullptr;
    if (isUAV) {
        if (FAILED(g_pd3dDevice->CreateUnorderedAccessView(d3dTexture, &uavDesc, &d3dUAV))) {
            // TODO: error handling
            d3dTexture->Release();
            if (d3dResourceView != nullptr)
                d3dResourceView->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    DXSharedBuffer* texture = sgfx::sgfx_new<DXSharedBuffer>();
    texture->dataBuffer = d3dTexture;
    texture->dataView   = d3dResourceView;
    texture->dataUAV    = d3dUAV;

    return Texture2DHandle(texture);
}

Texture3DHandle createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DataFormat format, uint32_t numMipmaps, uint32_t flags)
{
    UINT        bindFlags   = D3D11_BIND_SHADER_RESOURCE;
    D3D11_USAGE usageFlags  = D3D11_USAGE_DEFAULT;
    UINT        cpuAccess   = 0;
    bool        isStaging   = false;
    bool        isUAV       = false;

    if (flags & TextureFlags::RenderTarget)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (flags & TextureFlags::DepthStencil)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    if (flags & TextureFlags::CPURead) {
        bindFlags   = 0;
        usageFlags  = D3D11_USAGE_STAGING;
        cpuAccess   = D3D11_CPU_ACCESS_READ;
        isStaging   = true;
    }

    if (flags & TextureFlags::GPUWrite) {
        bindFlags  |= D3D11_BIND_UNORDERED_ACCESS;
        isUAV       = true;
    }

    D3D11_TEXTURE3D_DESC textureDesc;
    std::memset(&textureDesc, 0, sizeof(textureDesc));

    textureDesc.Width          = width;
    textureDesc.Height         = height;
    textureDesc.Depth          = depth;
    textureDesc.MipLevels      = numMipmaps;
    textureDesc.Format         = MapDataFormat[static_cast<size_t>(format)];
    textureDesc.Usage          = usageFlags;
    textureDesc.BindFlags      = bindFlags;
    textureDesc.CPUAccessFlags = cpuAccess;
    textureDesc.MiscFlags      = 0;

    ID3D11Texture3D* d3dTexture = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture3D(&textureDesc, nullptr, &d3dTexture))) {
        // TODO: error handling
        return Texture1DHandle::invalidHandle();
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
    std::memset(&viewDesc, 0, sizeof(viewDesc));

    viewDesc.Format                    = MapDataFormat[static_cast<size_t>(format)];
    viewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
    viewDesc.Texture3D.MipLevels       = numMipmaps;
    //viewDesc.Texture3D.MostDetailedMip = -1;

    ID3D11ShaderResourceView* d3dResourceView = nullptr;
    if (!isStaging) {
        if (FAILED(g_pd3dDevice->CreateShaderResourceView(d3dTexture, &viewDesc, &d3dResourceView))) {
            // TODO: error handling
            d3dTexture->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    std::memset(&uavDesc, 0, sizeof(uavDesc));

    uavDesc.Format                      = MapDataFormat[static_cast<size_t>(format)];
    uavDesc.ViewDimension               = D3D11_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice          = 0; // TODO: handle this!
    uavDesc.Texture3D.FirstWSlice       = 0; // TODO: handle this!
    uavDesc.Texture3D.WSize             = 0; // TODO: handle this!

    ID3D11UnorderedAccessView* d3dUAV = nullptr;
    if (isUAV) {
        if (FAILED(g_pd3dDevice->CreateUnorderedAccessView(d3dTexture, &uavDesc, &d3dUAV))) {
            // TODO: error handling
            d3dTexture->Release();
            if (d3dResourceView != nullptr)
                d3dResourceView->Release();
            return Texture1DHandle::invalidHandle();
        }
    }

    DXSharedBuffer* texture = sgfx::sgfx_new<DXSharedBuffer>();
    texture->dataBuffer = d3dTexture;
    texture->dataView   = d3dResourceView;
    texture->dataUAV    = d3dUAV;

    return Texture3DHandle(texture);
}

void clearTextureRW(TextureHandle handle, uint32_t value)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        uint32_t d3dValues[4] = { value, 0, 0, 0 };

        if (buffer->dataUAV != nullptr)
            g_pImmediateContext->ClearUnorderedAccessViewUint(buffer->dataUAV, d3dValues);
    }
}

void clearTextureRW(TextureHandle handle, float value)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        float d3dValues[4] = { value, 0.F, 0.0F, 0.0F };

        if (buffer->dataUAV != nullptr)
            g_pImmediateContext->ClearUnorderedAccessViewFloat(buffer->dataUAV, d3dValues);
    }
}

void* mapTexture(TextureHandle handle, MapType type)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        D3D11_MAPPED_SUBRESOURCE mappedData;
        std::memset(&mappedData, 0, sizeof(mappedData));

        if (FAILED(g_pImmediateContext->Map(buffer->dataBuffer, 0, MapMapType[static_cast<size_t>(type)], 0, &mappedData))) {
            return nullptr;
        }

        return mappedData.pData;
    }
    return nullptr;
}

void unmapTexture(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);

        g_pImmediateContext->Unmap(buffer->dataBuffer, 0);
    }
}

void updateTexture(
    TextureHandle handle, const void* mem,
    uint32_t mip,
    size_t offsetX,  size_t sizeX,
    size_t offsetY,  size_t sizeY,
    size_t offsetZ,  size_t sizeZ,
    size_t rowPitch, size_t depthPitch
)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* texture  = static_cast<DXSharedBuffer*>(handle.value);

        D3D11_BOX box;
        box.left   = static_cast<UINT>(offsetX);
        box.right  = static_cast<UINT>(offsetX + sizeX);
        box.top    = static_cast<UINT>(offsetY);
        box.bottom = static_cast<UINT>(offsetY + sizeY);
        box.front  = static_cast<UINT>(offsetZ);
        box.back   = static_cast<UINT>(offsetZ + sizeZ);

        g_pImmediateContext->UpdateSubresource(texture->dataBuffer, mip, &box, mem, static_cast<UINT>(rowPitch), static_cast<UINT>(depthPitch));
    }
}

void releaseTexture(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* texture = static_cast<DXSharedBuffer*>(handle.value);
        sgfx::sgfx_delete(texture);
    }
}

void copyResource(TextureHandle src, TextureHandle dst)
{
    if (src != dst && src != TextureHandle::invalidHandle()) {
        DXSharedBuffer* dxSrc = static_cast<DXSharedBuffer*>(src.value);
        DXSharedBuffer* dxDst = static_cast<DXSharedBuffer*>(dst.value);

        g_pImmediateContext->CopyResource(dxDst->dataBuffer, dxSrc->dataBuffer);
    }
}

void copyResource(BufferHandle src, BufferHandle dst)
{
    if (src != dst && src != BufferHandle::invalidHandle()) {
        DXSharedBuffer* dxSrc = static_cast<DXSharedBuffer*>(src.value);
        DXSharedBuffer* dxDst = static_cast<DXSharedBuffer*>(dst.value);

        g_pImmediateContext->CopyResource(dxDst->dataBuffer, dxSrc->dataBuffer);
    }
}

void copyResource(ConstantBufferHandle src, ConstantBufferHandle dst)
{
    if (src != dst && src != ConstantBufferHandle::invalidHandle()) {
        ID3D11Buffer* dxSrc = static_cast<ID3D11Buffer*>(src.value);
        ID3D11Buffer* dxDst = static_cast<ID3D11Buffer*>(dst.value);

        g_pImmediateContext->CopyResource(dxDst, dxSrc);
    }
}

Texture2DHandle getBackBuffer()
{
    DXSharedBuffer* buffer = sgfx_new<DXSharedBuffer>();
    if (FAILED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&buffer->dataBuffer))) {
        // TODO: error handling
        return Texture2DHandle::invalidHandle();
    }

    return Texture2DHandle(buffer);
}

RenderTargetHandle createRenderTarget(const RenderTargetDescriptor& desc)
{
    RenderTargetImpl* impl = sgfx_new<RenderTargetImpl>();

    impl->numRenderTargets = desc.numColorTextures;

    for (uint32_t i = 0; i < desc.numColorTextures; ++i) {
        DXSharedBuffer* textureResource = static_cast<DXSharedBuffer*>(desc.colorTextures[i].value);

        D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
        std::memset(&rtDesc, 0, sizeof(rtDesc));

        rtDesc.Format             = DXGI_FORMAT_UNKNOWN;
        rtDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtDesc.Texture2D.MipSlice = 0;

        ID3D11RenderTargetView* renderTargetView = nullptr;
        if (FAILED(g_pd3dDevice->CreateRenderTargetView(textureResource->dataBuffer, &rtDesc, &renderTargetView))) {
            // TODO: error handling
            sgfx_delete(impl);
            return RenderTargetHandle::invalidHandle();
        }

        impl->renderTargetViews[i] = renderTargetView;
    }

    DXSharedBuffer* depthStencilResource = static_cast<DXSharedBuffer*>(desc.depthStencilTexture.value);

    if (depthStencilResource != nullptr) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
        std::memset(&dsDesc, 0, sizeof(dsDesc));

        D3D11_SHADER_RESOURCE_VIEW_DESC depthTextureDesc;
        depthStencilResource->dataView->GetDesc(&depthTextureDesc);

        DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        switch (depthTextureDesc.Format) {
        case DXGI_FORMAT_R16_UNORM:             { depthFormat = DXGI_FORMAT_D16_UNORM; } break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: { depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; } break;
        case DXGI_FORMAT_R32_FLOAT:             { depthFormat = DXGI_FORMAT_D32_FLOAT; } break;
        default: {} break;
        }

        dsDesc.Format             = depthFormat;
        dsDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsDesc.Texture2D.MipSlice = 0;

        ID3D11DepthStencilView* depthStencilView = nullptr;
        if (FAILED(g_pd3dDevice->CreateDepthStencilView(depthStencilResource->dataBuffer, &dsDesc, &depthStencilView))) {
            // TODO: error handling
            sgfx_delete(impl);
            return RenderTargetHandle::invalidHandle();
        }

        impl->depthStencilView = depthStencilView;
    }

    return RenderTargetHandle(impl);
}

void releaseRenderTarget(RenderTargetHandle handle)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);
        sgfx_delete(rtimpl);
    }
}

void setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    D3D11_VIEWPORT vp;

    vp.Width    = static_cast<float>(width);
    vp.Height   = static_cast<float>(height);
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;

    g_pImmediateContext->RSSetViewports(1, &vp);
}

void setResourceRW(RenderTargetHandle handle, uint32_t idx, BufferHandle resource)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);

        ID3D11UnorderedAccessView* uav = nullptr;
        if (resource != BufferHandle::invalidHandle()) {
            DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(resource.value);

            uav = buffer->dataUAV;
        }

        rtimpl->unorderedAccessViews[idx] = uav;
    }
}

void setResourceRW(RenderTargetHandle handle, uint32_t idx, TextureHandle resource)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);

        ID3D11UnorderedAccessView* uav = nullptr;
        if (resource != TextureHandle::invalidHandle()) {
            DXSharedBuffer* texture = static_cast<DXSharedBuffer*>(resource.value);

            uav = texture->dataUAV;
        }

        rtimpl->unorderedAccessViews[idx] = uav;
    }
}

void setRenderTarget(RenderTargetHandle handle)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);

        // reset previous RTs
        ID3D11RenderTargetView*     rtViews[RenderTargetSlot::Count] = { nullptr };
        //ID3D11UnorderedAccessView*  uaViews[RenderTargetSlot::Count] = { nullptr };

        //g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(
        //    RenderTargetSlot::Count, rtViews,
        //    nullptr,
        //    RenderTargetSlot::Count, RenderTargetSlot::Count - 1, uaViews, nullptr
        //);
        g_pImmediateContext->OMSetRenderTargets(RenderTargetSlot::Count, rtViews, nullptr);

        g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(
            rtimpl->numRenderTargets, rtimpl->renderTargetViews,
            rtimpl->depthStencilView,
            rtimpl->numRenderTargets, RenderTargetSlot::Count - 1, rtimpl->unorderedAccessViews,
            rtimpl->uavCounters
        );
    }
}

void clearRenderTarget(RenderTargetHandle handle, uint32_t color)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);
        float fcolor[4];
        fcolor[0] = static_cast<float>((color >> 0)  & 0xFF) / 255.0F;
        fcolor[1] = static_cast<float>((color >> 8)  & 0xFF) / 255.0F;
        fcolor[2] = static_cast<float>((color >> 16) & 0xFF) / 255.0F;
        fcolor[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0F;

        for (size_t i = 0; i < RenderTargetSlot::Count; ++i) {
            if (rtimpl->renderTargetViews[i] != nullptr)
                g_pImmediateContext->ClearRenderTargetView(rtimpl->renderTargetViews[i], fcolor);
        }
    }
}

void clearRenderTarget(RenderTargetHandle handle, uint32_t slot, uint32_t color)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);
        float fcolor[4];
        fcolor[0] = static_cast<float>((color >> 0)  & 0xFF) / 255.0F;
        fcolor[1] = static_cast<float>((color >> 8)  & 0xFF) / 255.0F;
        fcolor[2] = static_cast<float>((color >> 16) & 0xFF) / 255.0F;
        fcolor[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0F;

        if (rtimpl->renderTargetViews[slot] != nullptr)
            g_pImmediateContext->ClearRenderTargetView(rtimpl->renderTargetViews[slot], fcolor);
    }
}

void clearDepthStencil(RenderTargetHandle handle, float depth, uint8_t stencil)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* rtimpl = static_cast<RenderTargetImpl*>(handle.value);

        if (rtimpl->depthStencilView != nullptr)
            g_pImmediateContext->ClearDepthStencilView(rtimpl->depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
    }
}

void present(uint32_t swapInterval)
{
    g_pSwapChain->Present(swapInterval, 0);
}

// draw queue stuff is similar for all APIs

DrawQueueHandle createDrawQueue(PipelineStateHandle state)
{
    internal::DrawQueue* queue = sgfx_new<internal::DrawQueue>(state);
    return DrawQueueHandle(queue);
}

void releaseDrawQueue(DrawQueueHandle handle)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        sgfx_delete(queue);
    }
}

void setSamplerState(DrawQueueHandle handle, uint32_t idx, SamplerStateHandle sampler)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setSamplerState(idx, sampler);
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

void setResource(DrawQueueHandle handle, uint32_t idx, TextureHandle resource)
{
    if (handle != DrawQueueHandle::invalidHandle()) {
        internal::DrawQueue* queue = static_cast<internal::DrawQueue*>(handle.value);
        queue->setResource(idx, resource);
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
        if (queue->getDrawCalls().GetSize() != 0) {
            dxProcessDrawQueue(queue);
            queue->clear();
        }
    }
}

void beginPerfEvent(const wchar_t* name)
{
#ifdef SGFX_USE_D3D11_1
    if (g_debugAnnotation)
        g_debugAnnotation->BeginEvent(name);
#endif
}

void endPerfEvent()
{
#ifdef SGFX_USE_D3D11_1
    if (g_debugAnnotation)
        g_debugAnnotation->EndEvent();
#endif
}

// D3D11 interop
namespace d3d11
{

ID3D11Buffer* getNativeBuffer(ConstantBufferHandle handle)
{
    return static_cast<ID3D11Buffer*>(handle.value);
}

ID3D11Resource* getNativeResource(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataBuffer;
    }
    return nullptr;
}

ID3D11ShaderResourceView* getNativeSRV(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataView;
    }
    return nullptr;
}

ID3D11UnorderedAccessView* getNativeUAV(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataUAV;
    }
    return nullptr;
}

ID3D11Resource* getNativeResource(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataBuffer;
    }
    return nullptr;
}

ID3D11ShaderResourceView* getNativeSRV(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataView;
    }
    return nullptr;
}

ID3D11UnorderedAccessView* getNativeUAV(TextureHandle handle)
{
    if (handle != TextureHandle::invalidHandle()) {
        DXSharedBuffer* buffer = static_cast<DXSharedBuffer*>(handle.value);
        return buffer->dataUAV;
    }
    return nullptr;
}

ID3D11SamplerState* getNativeSamplerState(SamplerStateHandle handle)
{
    return static_cast<ID3D11SamplerState*>(handle.value);
}

ID3D11RenderTargetView* getNativeRTV(RenderTargetHandle handle, size_t idx)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* impl = static_cast<RenderTargetImpl*>(handle.value);
        return impl->renderTargetViews[idx];
    }
    return nullptr;
}

ID3D11DepthStencilView* getNativeDSV(RenderTargetHandle handle)
{
    if (handle != RenderTargetHandle::invalidHandle()) {
        RenderTargetImpl* impl = static_cast<RenderTargetImpl*>(handle.value);
        return impl->depthStencilView;
    }
    return nullptr;
}

}

}

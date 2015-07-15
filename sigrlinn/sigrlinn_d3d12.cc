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
#include "sigrlinn.hh"

#include <d3d12.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <memory>
#include <cstdlib>

namespace sgfx
{

//=============================================================================

//static D3D12_MAP MapMapType[MapType::Count] = {
//    D3D12_MAP_READ,
//    D3D12_MAP_WRITE_DISCARD
//};
//static_assert((sizeof(MapMapType) / sizeof(D3D11_MAP)) == static_cast<uint32_t>(MapType::Count), "Mapping is broken!");

static D3D12_PRIMITIVE_TOPOLOGY_TYPE MapPrimitiveTopology[PrimitiveTopology::Count] = {
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED // TODO: handle this
};
static_assert((sizeof(MapPrimitiveTopology) / sizeof(D3D12_PRIMITIVE_TOPOLOGY)) == static_cast<size_t>(PrimitiveTopology::Count), "Mapping is broken!");

static D3D12_FILTER MapTextureFilter[TextureFilter::Count] = {
    D3D12_FILTER_MIN_MAG_MIP_POINT,
    D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
    D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
    D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
    D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    D3D12_FILTER_ANISOTROPIC
};
static_assert((sizeof(MapTextureFilter) / sizeof(D3D12_FILTER)) == static_cast<size_t>(TextureFilter::Count), "Mapping is broken!");

static D3D12_TEXTURE_ADDRESS_MODE MapAddressMode[AddressMode::Count] = {
    D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    D3D12_TEXTURE_ADDRESS_MODE_BORDER
};
static_assert((sizeof(MapAddressMode) / sizeof(D3D12_TEXTURE_ADDRESS_MODE)) == static_cast<size_t>(AddressMode::Count), "Mapping is broken!");

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

static D3D12_FILL_MODE MapFillMode[FillMode::Count] = {
    D3D12_FILL_MODE_SOLID,
    D3D12_FILL_MODE_WIREFRAME
};
static_assert((sizeof(MapFillMode) / sizeof(D3D12_FILL_MODE)) == static_cast<size_t>(FillMode::Count), "Mapping is broken!");

static D3D12_CULL_MODE MapCullMode[CullMode::Count] = {
    D3D12_CULL_MODE_BACK,
    D3D12_CULL_MODE_FRONT,
    D3D12_CULL_MODE_NONE
};
static_assert((sizeof(MapCullMode) / sizeof(D3D12_CULL_MODE)) == static_cast<size_t>(CullMode::Count), "Mapping is broken!");

static BOOL MapCounterDirection[CounterDirection::Count] = {
    TRUE,
    FALSE
};
static_assert((sizeof(MapCounterDirection) / sizeof(BOOL)) == static_cast<size_t>(CounterDirection::Count), "Mapping is broken!");

static D3D12_BLEND MapBlendFactor[BlendFactor::Count] = {
    D3D12_BLEND_ZERO,
    D3D12_BLEND_ONE,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_DEST_ALPHA,
    D3D12_BLEND_INV_SRC_ALPHA,
    D3D12_BLEND_INV_DEST_ALPHA,
    D3D12_BLEND_SRC_COLOR,
    D3D12_BLEND_DEST_COLOR,
    D3D12_BLEND_INV_SRC_COLOR,
    D3D12_BLEND_INV_DEST_COLOR
};
static_assert((sizeof(MapBlendFactor) / sizeof(D3D12_BLEND)) == static_cast<size_t>(BlendFactor::Count), "Mapping is broken!");

static D3D12_BLEND_OP MapBlendOp[BlendOp::Count] = {
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_OP_SUBTRACT,
    D3D12_BLEND_OP_REV_SUBTRACT,
    D3D12_BLEND_OP_MIN,
    D3D12_BLEND_OP_MAX
};
static_assert((sizeof(MapBlendOp) / sizeof(D3D12_BLEND_OP)) == static_cast<size_t>(BlendOp::Count), "Mapping is broken!");

static D3D12_DEPTH_WRITE_MASK MapDepthWriteMask[DepthWriteMask::Count] = {
    D3D12_DEPTH_WRITE_MASK_ZERO,
    D3D12_DEPTH_WRITE_MASK_ALL
};
static_assert((sizeof(MapDepthWriteMask) / sizeof(D3D12_DEPTH_WRITE_MASK)) == static_cast<size_t>(DepthWriteMask::Count), "Mapping is broken!");

static D3D12_COMPARISON_FUNC MapComparisonFunc[ComparisonFunc::Count] = {
    D3D12_COMPARISON_FUNC_ALWAYS,
    D3D12_COMPARISON_FUNC_NEVER,
    D3D12_COMPARISON_FUNC_LESS,
    D3D12_COMPARISON_FUNC_LESS_EQUAL,
    D3D12_COMPARISON_FUNC_GREATER,
    D3D12_COMPARISON_FUNC_GREATER_EQUAL,
    D3D12_COMPARISON_FUNC_EQUAL,
    D3D12_COMPARISON_FUNC_NOT_EQUAL
};
static_assert((sizeof(MapComparisonFunc) / sizeof(D3D12_COMPARISON_FUNC)) == static_cast<size_t>(ComparisonFunc::Count), "Mapping is broken!");

static D3D12_STENCIL_OP MapStencilOp[StencilOp::Count] = {
    D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP_ZERO,
    D3D12_STENCIL_OP_REPLACE,
    D3D12_STENCIL_OP_INCR,
    D3D12_STENCIL_OP_DECR
};
static_assert((sizeof(MapStencilOp) / sizeof(D3D12_STENCIL_OP)) == static_cast<size_t>(StencilOp::Count), "Mapping is broken!");

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

//=============================================================================
static inline void* sgfx_malloc(size_t size)
{
    return malloc(size);
}

static inline void sgfx_free(void* ptr)
{
    return free(ptr);
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
struct DXDescriptorAllocator final
{
    ID3D12DescriptorHeap*   d3dHeap   = nullptr;
    size_t                  stride    = 0;
    size_t                  poolSize  = 0;

    void**                  freeList  = nullptr;

    inline void init(ID3D12DescriptorHeap* heap, size_t hstride, size_t hpoolSize)
    {
        d3dHeap  = heap;
        stride   = hstride;
        poolSize = hpoolSize;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = d3dHeap->GetCPUDescriptorHandleForHeapStart();

        freeList = reinterpret_cast<void**>(cpuHandle.ptr);

        void** ptr = freeList;
        for (size_t i = 0; i < poolSize - 1; ++i) {
            size_t address = reinterpret_cast<size_t>(ptr);
            *ptr           = reinterpret_cast<void*>(address + stride);
            ptr            = reinterpret_cast<void**>(*ptr);
        }
        *ptr = nullptr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE allocateDescriptor()
    {
        if (freeList == nullptr)
            return { 0 };

        void* ptr = freeList;
        freeList  = reinterpret_cast<void**>(*freeList);

        return { reinterpret_cast<size_t>(ptr) };
    }

    void freeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        void* ptr = reinterpret_cast<void*>(handle.ptr);

        *(reinterpret_cast<void**>(ptr)) = freeList;
        freeList = reinterpret_cast<void**>(ptr);
    }
};

//=============================================================================
ID3D12Device*           g_pd3dDevice            = nullptr;
ID3D12DescriptorHeap*   g_srvDescriptorHeap     = nullptr;
ID3D12DescriptorHeap*   g_samplerDescriptorHeap = nullptr;
ID3D12DescriptorHeap*   g_rtvDescriptorHeap     = nullptr;
ID3D12DescriptorHeap*   g_dsvDescriptorHeap     = nullptr;

DXDescriptorAllocator   g_srvAllocator;
DXDescriptorAllocator   g_samplerAllocator;
DXDescriptorAllocator   g_rtvAllocator;
DXDescriptorAllocator   g_dsvAllocator;

AllocFunc               g_allocFunc = sgfx_malloc;
FreeFunc                g_freeFunc = sgfx_free;

//=============================================================================
struct DXCommonShaderImpl final
{
    void*  bytecode = nullptr;
    size_t bytecodeSize = 0;

    inline ~DXCommonShaderImpl()
    {
        sgfx_free(bytecode);
    }
};

struct DXSurfaceShaderImpl final
{
    DXCommonShaderImpl* vs = nullptr;
    DXCommonShaderImpl* hs = nullptr;
    DXCommonShaderImpl* ds = nullptr;
    DXCommonShaderImpl* gs = nullptr;
    DXCommonShaderImpl* ps = nullptr;

    inline DXSurfaceShaderImpl() {}
    inline ~DXSurfaceShaderImpl() {}
};

struct DXInputLayoutImpl final
{
    D3D12_INPUT_ELEMENT_DESC* inputElements     = nullptr;
    size_t                    inputElementsSize = 0;
    size_t                    stride            = 0;

    inline ~DXInputLayoutImpl()
    {
        sgfx_free(inputElements);
    }
};

struct DXSharedBuffer final
{
    ID3D12Resource* dataBuffer = nullptr;
    size_t          dataSize   = 0;
    size_t          dataStride = 0;

    D3D12_RANGE     mappedRange;
    MapType         mappedType;
    bool            mapped = false;

    D3D12_SHADER_RESOURCE_VIEW_DESC  dataView;
    D3D12_UNORDERED_ACCESS_VIEW_DESC dataUAV;

    D3D12_CPU_DESCRIPTOR_HANDLE      dataViewHandle = { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE      dataUAVHandle  = { 0 };

    inline ~DXSharedBuffer()
    {
        if (dataViewHandle.ptr != 0)
            g_srvAllocator.freeDescriptor(dataViewHandle);
        if (dataUAVHandle.ptr != 0)
            g_srvAllocator.freeDescriptor(dataUAVHandle);

        if (dataBuffer != nullptr)
            dataBuffer->Release();
    }

    inline void* map(MapType type)
    {
        mappedType  = type;
        mappedRange = { 0, dataSize };

        void* data = nullptr;
        HRESULT hr = dataBuffer->Map(0, &mappedRange, &data);
        if (SUCCEEDED(hr)) {
            mapped = true;
            return data;
        }

        return nullptr;
    }

    inline void unmap()
    {
        if (mapped) {
            mapped = false;
            if (mappedType == MapType::Write)
                dataBuffer->Unmap(0, &mappedRange);
            else
                dataBuffer->Unmap(0, nullptr);
        }
    }
};

struct DXConstantBuffer final
{
    ID3D12Resource*                 dataBuffer = nullptr;
    D3D12_CONSTANT_BUFFER_VIEW_DESC dataView;
    D3D12_CPU_DESCRIPTOR_HANDLE     dataHandle = { 0 };

    inline ~DXConstantBuffer()
    {
        if (dataHandle.ptr != 0)
            g_srvAllocator.freeDescriptor(dataHandle);

        if (dataBuffer != nullptr)
            dataBuffer->Release();
    }
};

template <typename T>
static inline T dxCreateCommonShader(const void* bytecode, size_t bytecodeSize)
{
    DXCommonShaderImpl* impl = sgfx_new<DXCommonShaderImpl>();
    impl->bytecodeSize = bytecodeSize;
    impl->bytecode     = sgfx_malloc(impl->bytecodeSize);
    std::memcpy(impl->bytecode, bytecode, impl->bytecodeSize);

    return T(impl);
}

static inline D3D12_SHADER_BYTECODE dxGetShaderBytecode(DXCommonShaderImpl* impl)
{
    if (impl != nullptr)
        return { impl->bytecode, impl->bytecodeSize };
    return { nullptr, 0 };
}

//=============================================================================
bool initD3D12(void* d3dDevice)
{
    g_pd3dDevice = static_cast<ID3D12Device*>(d3dDevice);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
    std::memset(&descriptorHeapDesc, 0, sizeof(descriptorHeapDesc));

    descriptorHeapDesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NumDescriptors   = 512; // TODO: handle this properly
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask         = 0;

    HRESULT hr = g_pd3dDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&g_srvDescriptorHeap)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return false;
    }

    descriptorHeapDesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descriptorHeapDesc.NumDescriptors   = 512;
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask         = 0;

    hr = g_pd3dDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&g_samplerDescriptorHeap)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        g_srvDescriptorHeap->Release();
        return false;
    }

    descriptorHeapDesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.NumDescriptors   = 512;
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask         = 0;

    hr = g_pd3dDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&g_rtvDescriptorHeap)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        g_srvDescriptorHeap->Release();
        g_samplerDescriptorHeap->Release();
        return false;
    }

    descriptorHeapDesc.Type             = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    descriptorHeapDesc.NumDescriptors   = 512;
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask         = 0;

    hr = g_pd3dDevice->CreateDescriptorHeap(
        &descriptorHeapDesc,
        __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&g_dsvDescriptorHeap)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        g_srvDescriptorHeap->Release();
        g_samplerDescriptorHeap->Release();
        g_rtvDescriptorHeap->Release();
        return false;
    }

    // TODO: remove hardcoded size
    g_srvAllocator.init(
        g_srvDescriptorHeap,
        g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        512
    );
    g_samplerAllocator.init(
        g_samplerDescriptorHeap,
        g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
        512
    );
    g_rtvAllocator.init(
        g_rtvDescriptorHeap,
        g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
        512
    );
    g_dsvAllocator.init(
        g_dsvDescriptorHeap,
        g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
        512
    );

    return true;
}

void shutdown()
{
    if (g_srvDescriptorHeap != nullptr)     g_srvDescriptorHeap->Release();
    if (g_samplerDescriptorHeap != nullptr) g_samplerDescriptorHeap->Release();
    if (g_rtvDescriptorHeap != nullptr)     g_rtvDescriptorHeap->Release();
    if (g_dsvDescriptorHeap != nullptr)     g_dsvDescriptorHeap->Release();
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
    return 0; // TODO: implement me!
}

//=============================================================================
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
        d3dmacros = reinterpret_cast<D3D_SHADER_MACRO*>(alloca(totalSize));
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
        return false;
    }

    outDataSize = outBlob->GetBufferSize();
    outData = reinterpret_cast<uint8_t*>(allocate(outDataSize));
    std::memcpy(outData, outBlob->GetBufferPointer(), outDataSize);
    outBlob->Release();

    return true;
}

VertexShaderHandle createVertexShader(const void* data, size_t dataSize)
{
    return dxCreateCommonShader<VertexShaderHandle>(data, dataSize);
}

void releaseVertexShader(VertexShaderHandle handle)
{
    if (handle != VertexShaderHandle::invalidHandle()) {
        DXCommonShaderImpl* impl = static_cast<DXCommonShaderImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

HullShaderHandle createHullShader(const void* data, size_t dataSize)
{
    return dxCreateCommonShader<HullShaderHandle>(data, dataSize);
}

void releaseHullShader(HullShaderHandle handle)
{
    if (handle != HullShaderHandle::invalidHandle()) {
        DXCommonShaderImpl* impl = static_cast<DXCommonShaderImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

DomainShaderHandle createDomainShader(const void* data, size_t dataSize)
{
    return dxCreateCommonShader<DomainShaderHandle>(data, dataSize);
}

void releaseDomainShader(DomainShaderHandle handle)
{
    if (handle != DomainShaderHandle::invalidHandle()) {
        DXCommonShaderImpl* impl = static_cast<DXCommonShaderImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

GeometryShaderHandle createGeometryShader(const void* data, size_t dataSize)
{
    return dxCreateCommonShader<GeometryShaderHandle>(data, dataSize);
}

void releaseGeometryShader(GeometryShaderHandle handle)
{
    if (handle != GeometryShaderHandle::invalidHandle()) {
        DXCommonShaderImpl* impl = static_cast<DXCommonShaderImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

SurfaceShaderHandle  linkSurfaceShader(
    VertexShaderHandle   vs,
    HullShaderHandle     hs,
    DomainShaderHandle   ds,
    GeometryShaderHandle gs,
    PixelShaderHandle    ps
)
{
    DXSurfaceShaderImpl* impl = sgfx_new<DXSurfaceShaderImpl>();
    impl->vs = static_cast<DXCommonShaderImpl*>(vs.value);
    impl->hs = static_cast<DXCommonShaderImpl*>(hs.value);
    impl->ds = static_cast<DXCommonShaderImpl*>(ds.value);
    impl->gs = static_cast<DXCommonShaderImpl*>(gs.value);
    impl->ps = static_cast<DXCommonShaderImpl*>(ps.value);

    return SurfaceShaderHandle(impl);
}

void releaseSurfaceShader(SurfaceShaderHandle handle)
{
    if (handle != SurfaceShaderHandle::invalidHandle()) {
        DXSurfaceShaderImpl* impl = static_cast<DXSurfaceShaderImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

ComputeShaderHandle createComputeShader(const void* data, size_t dataSize)
{
    return ComputeShaderHandle::invalidHandle(); // TODO: implement me!
}

void releaseComputeShader(ComputeShaderHandle handle)
{}

void dispatchComputeShader(ComputeShaderHandle handle)
{}

VertexFormatHandle createVertexFormat(
    VertexElementDescriptor* elements,
    size_t                   size,
    void* shaderBytecode, size_t shaderBytecodeSize,
    ErrorReportFunc          errorReport
)
{
    if (elements == nullptr || size == 0)
        return VertexFormatHandle::invalidHandle();

    DXInputLayoutImpl* impl = sgfx_new<DXInputLayoutImpl>();

    size_t totalSize = size * sizeof(D3D12_INPUT_ELEMENT_DESC);
    D3D12_INPUT_ELEMENT_DESC* inputData = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(sgfx_malloc(totalSize));
    std::memset(inputData, 0, totalSize);

    UINT stride = 0;

    for (size_t i = 0; i < size; ++i) {
        inputData[i].SemanticName         = elements[i].semanticName;
        inputData[i].SemanticIndex        = elements[i].semanticIndex;
        inputData[i].Format               = MapDataFormat[static_cast<size_t>(elements[i].format)];
        inputData[i].InputSlot            = elements[i].slot;
        inputData[i].AlignedByteOffset    = static_cast<UINT>(elements[i].offset);
        inputData[i].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputData[i].InstanceDataStepRate = 0;

        stride += dxFormatStride(elements[i].format);
    }

    impl->inputElements     = inputData;
    impl->inputElementsSize = size;
    impl->stride            = stride;

    return VertexFormatHandle(impl);
}

void releaseVertexFormat(VertexFormatHandle handle)
{
    if (handle != VertexFormatHandle::invalidHandle()) {
        DXInputLayoutImpl* impl = static_cast<DXInputLayoutImpl*>(handle.value);
        sgfx_delete(impl);
    }
}

PipelineStateHandle createPipelineState(const PipelineStateDescriptor& desc)
{
    ID3DBlob* outBlob   = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // create root signature
    ID3D12RootSignature* rootSignature = nullptr;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc; // TODO: implement me!

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &outBlob,
        &errorBlob
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return PipelineStateHandle::invalidHandle();
    }

    hr = g_pd3dDevice->CreateRootSignature(
        0,
        outBlob->GetBufferPointer(), outBlob->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return PipelineStateHandle::invalidHandle();
    }

    DXInputLayoutImpl*   inputLayout = static_cast<DXInputLayoutImpl*>(desc.vertexFormat.value);
    DXSurfaceShaderImpl* shader      = static_cast<DXSurfaceShaderImpl*>(desc.shader.value);

    // rasterizer state
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = MapFillMode[static_cast<size_t>(desc.rasterizerState.fillMode)];
    rasterizerDesc.CullMode              = MapCullMode[static_cast<size_t>(desc.rasterizerState.cullMode)];
    rasterizerDesc.FrontCounterClockwise = MapCounterDirection[static_cast<size_t>(desc.rasterizerState.counterDirection)];
    rasterizerDesc.DepthBias             = 0;
    rasterizerDesc.DepthBiasClamp        = 0.0F;
    rasterizerDesc.SlopeScaledDepthBias  = 0.0F;
    rasterizerDesc.DepthClipEnable       = TRUE;
    rasterizerDesc.MultisampleEnable     = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount     = 0;
    rasterizerDesc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // blend state
    const BlendState& bsState = desc.blendState;

    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable  = bsState.alphaToCoverageEnabled;
    blendDesc.IndependentBlendEnable = bsState.separateBlendEnabled;
    std::memset(&blendDesc.RenderTarget, 0, 8 * sizeof(D3D12_RENDER_TARGET_BLEND_DESC));

    blendDesc.RenderTarget[0].BlendEnable           = bsState.blendDesc.blendEnabled;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = static_cast<UINT8>(bsState.renderTargetBlendDesc[0].writeMask);
    blendDesc.RenderTarget[0].SrcBlend              = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.srcBlend)];
    blendDesc.RenderTarget[0].DestBlend             = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.dstBlend)];
    blendDesc.RenderTarget[0].BlendOp               = MapBlendOp[static_cast<size_t>(bsState.blendDesc.blendOp)];
    blendDesc.RenderTarget[0].SrcBlendAlpha         = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.srcBlendAlpha)];
    blendDesc.RenderTarget[0].DestBlendAlpha        = MapBlendFactor[static_cast<size_t>(bsState.blendDesc.dstBlendAlpha)];
    blendDesc.RenderTarget[0].BlendOpAlpha          = MapBlendOp[static_cast<size_t>(bsState.blendDesc.blendOpAlpha)];

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

    // depth-stencil state
    const DepthStencilState& dsState = desc.depthStencilState;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
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

    // create PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    std::memset(&psoDesc, 0, sizeof(psoDesc));

    psoDesc.pRootSignature        = rootSignature;
    psoDesc.VS                    = dxGetShaderBytecode(shader->vs);
    psoDesc.HS                    = dxGetShaderBytecode(shader->hs);
    psoDesc.DS                    = dxGetShaderBytecode(shader->ds);
    psoDesc.GS                    = dxGetShaderBytecode(shader->gs);
    psoDesc.PS                    = dxGetShaderBytecode(shader->ps);
    //psoDesc.StreamOutput
    psoDesc.BlendState            = blendDesc;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.RasterizerState       = rasterizerDesc;
    psoDesc.DepthStencilState     = depthStencilDesc;
    psoDesc.InputLayout           = { inputLayout->inputElements, inputLayout->inputElementsSize };
    psoDesc.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // TODO: implement me!
    psoDesc.NumRenderTargets      = 1; // TODO: implement me!
    psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count      = 1;
    //psoDesc.CachedPSO
    //psoDesc.Flags                 = 0;

    ID3D12PipelineState* pipelineState = nullptr;
    hr = g_pd3dDevice->CreateGraphicsPipelineState(
        &psoDesc,
        __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipelineState)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return PipelineStateHandle::invalidHandle();
    }

    // drop the root signature because we don't need it anymore
    rootSignature->Release();

    return PipelineStateHandle(pipelineState);
}

void releasePipelineState(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        ID3D12PipelineState* impl = static_cast<ID3D12PipelineState*>(handle.value);
        impl->Release();
    }
}

BufferHandle createBuffer(uint32_t flags, const void* mem, size_t size, size_t stride)
{
    bool isStructured = false;
    bool isUAV        = false;

    D3D12_HEAP_PROPERTIES heapProperties;
    std::memset(&heapProperties, 0, sizeof(heapProperties));

    heapProperties.Type             = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty  = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
    heapProperties.CreationNodeMask = 0;
    heapProperties.VisibleNodeMask  = 0;

    D3D12_HEAP_FLAGS      heapFlags             = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    D3D12_RESOURCE_FLAGS  resourceFlags         = D3D12_RESOURCE_FLAG_NONE;
    D3D12_RESOURCE_STATES initialResourceStates = D3D12_RESOURCE_STATE_COMMON;

    if (flags & BufferFlags::CPURead) {
        heapProperties.Type            = D3D12_HEAP_TYPE_READBACK;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    }

    if (flags & BufferFlags::CPUWrite) {
        heapProperties.Type            = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
    }

    if (flags & BufferFlags::GPUWrite) {
        resourceFlags         |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        initialResourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        isUAV                 = true;
    }

    if (!(flags & BufferFlags::StructuredBuffer)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        isStructured  = true;
    }

    if (flags & BufferFlags::VertexBuffer) {
        initialResourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (flags & BufferFlags::IndexBuffer) {
        initialResourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }

    // if it is neither structured buffer nor UAV it is not a pixel shader resource
    // otherwise it MAY be a pixel shader resource, thus set an appropriate flag
    if ((flags & BufferFlags::StructuredBuffer) || (flags & BufferFlags::GPUWrite)) {
        initialResourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    } else {
        initialResourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    D3D12_RESOURCE_DESC resourceDesc;
    std::memset(&resourceDesc, 0, sizeof(resourceDesc));

    resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment          = 0;
    resourceDesc.Width              = size;
    resourceDesc.Height             = 0;
    resourceDesc.DepthOrArraySize   = 0;
    resourceDesc.MipLevels          = 0;
    resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count   = 0;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags              = resourceFlags;

    ID3D12Resource* d3dResource = nullptr;

    HRESULT hr = g_pd3dDevice->CreateCommittedResource(
        &heapProperties, heapFlags,
        &resourceDesc,
        initialResourceStates,
        nullptr,
        __uuidof(ID3D12Resource), reinterpret_cast<void**>(&d3dResource)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return BufferHandle::invalidHandle();
    }

    if (mem != nullptr) {
        D3D12_RANGE mappedRange;
        mappedRange.Begin = 0;
        mappedRange.End   = size;

        void* mappedData = nullptr;

        hr = d3dResource->Map(0, &mappedRange, &mappedData);

        if (SUCCEEDED(hr)) {
            std::memcpy(mappedData, mem, size);
            d3dResource->Unmap(0, &mappedRange);
        }
    }

    DXSharedBuffer* buffer = sgfx_new<DXSharedBuffer>();
    buffer->dataBuffer = d3dResource;
    buffer->dataSize   = size;
    buffer->dataStride = stride;

    if (isStructured) {
        buffer->dataView.Format                     = DXGI_FORMAT_UNKNOWN;
        buffer->dataView.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        buffer->dataView.Shader4ComponentMapping    = 0;

        buffer->dataView.Buffer.FirstElement        = 0;
        buffer->dataView.Buffer.NumElements         = size / stride;
        buffer->dataView.Buffer.StructureByteStride = stride;
        buffer->dataView.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

        buffer->dataViewHandle = g_srvAllocator.allocateDescriptor();

        g_pd3dDevice->CreateShaderResourceView(
            buffer->dataBuffer,
            &buffer->dataView,
            buffer->dataViewHandle
        );
    }

    if (isUAV) {
        buffer->dataUAV.Format                      = DXGI_FORMAT_UNKNOWN;
        buffer->dataUAV.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;

        buffer->dataUAV.Buffer.FirstElement         = 0;
        buffer->dataUAV.Buffer.NumElements          = size / stride;
        buffer->dataUAV.Buffer.StructureByteStride  = stride;
        buffer->dataUAV.Buffer.CounterOffsetInBytes = 0; // TODO: implement me!
        buffer->dataUAV.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;

        buffer->dataUAVHandle = g_srvAllocator.allocateDescriptor();

        g_pd3dDevice->CreateUnorderedAccessView(
            buffer->dataBuffer, nullptr,
            &buffer->dataUAV,
            buffer->dataUAVHandle
        );
    }

    return BufferHandle(buffer);
}

void releaseBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* impl = static_cast<DXSharedBuffer*>(handle.value);
        sgfx_delete(impl);
    }
}

void* mapBuffer(BufferHandle handle, MapType type)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* impl = static_cast<DXSharedBuffer*>(handle.value);
        return impl->map(type);
    }

    return nullptr;
}

void unmapBuffer(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* impl = static_cast<DXSharedBuffer*>(handle.value);
        impl->unmap();
    }
}

void copyBufferData(BufferHandle handle, size_t offset, size_t size, const void* mem)
{
    if (handle != BufferHandle::invalidHandle()) {
        DXSharedBuffer* impl = static_cast<DXSharedBuffer*>(handle.value);

        D3D12_RANGE mappedRange = { offset, size };
        void*       mappedData  = nullptr;

        HRESULT hr = impl->dataBuffer->Map(0, &mappedRange, &mappedData);
        if (SUCCEEDED(hr)) {
            std::memcpy(mappedData, mem, size);
            impl->dataBuffer->Unmap(0, &mappedRange);
        }
    }
}

ConstantBufferHandle createConstantBuffer(const void* mem, size_t size)
{
    D3D12_HEAP_PROPERTIES heapProperties;
    std::memset(&heapProperties, 0, sizeof(heapProperties));

    heapProperties.Type             = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty  = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
    heapProperties.CreationNodeMask = 0;
    heapProperties.VisibleNodeMask  = 0;

    D3D12_HEAP_FLAGS      heapFlags             = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    D3D12_RESOURCE_FLAGS  resourceFlags         = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES initialResourceStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    D3D12_RESOURCE_DESC resourceDesc;
    std::memset(&resourceDesc, 0, sizeof(resourceDesc));

    resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment          = 0;
    resourceDesc.Width              = size;
    resourceDesc.Height             = 0;
    resourceDesc.DepthOrArraySize   = 0;
    resourceDesc.MipLevels          = 0;
    resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count   = 0;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags              = resourceFlags;

    ID3D12Resource* d3dResource = nullptr;

    HRESULT hr = g_pd3dDevice->CreateCommittedResource(
        &heapProperties, heapFlags,
        &resourceDesc,
        initialResourceStates,
        nullptr,
        __uuidof(ID3D12Resource), reinterpret_cast<void**>(&d3dResource)
    );
    if (FAILED(hr)) {
        // TODO: error handling
        return ConstantBufferHandle::invalidHandle();
    }

    if (mem != nullptr) {
        D3D12_RANGE mappedRange;
        mappedRange.Begin = 0;
        mappedRange.End   = size;

        void* mappedData = nullptr;

        hr = d3dResource->Map(0, &mappedRange, &mappedData);

        if (SUCCEEDED(hr)) {
            std::memcpy(mappedData, mem, size);
            d3dResource->Unmap(0, &mappedRange);
        }
    }

    DXConstantBuffer* buffer = sgfx_new<DXConstantBuffer>();
    buffer->dataBuffer = d3dResource;

    buffer->dataHandle              = g_srvAllocator.allocateDescriptor();
    buffer->dataView.BufferLocation = d3dResource->GetGPUVirtualAddress();
    buffer->dataView.SizeInBytes    = size;

    return ConstantBufferHandle(buffer);
}

}

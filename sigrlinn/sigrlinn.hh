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
#pragma once

#include <stdint.h>
#include <wchar.h>

namespace sgfx
{

template <typename T, int tag>
struct Handle
{
    T value = static_cast<T>(0);
    inline explicit Handle(const T& newValue) : value(newValue) {}
    inline Handle() {}
    inline Handle(const Handle& d) : value(d.value) {}
    inline Handle& operator=(const Handle& d)    { value = d.value; return *this; }
    inline Handle& operator=(const T& d)         { value = d; return *this;       }
    inline bool operator==(const Handle& handle) { return value == handle.value; }
    inline bool operator!=(const Handle& handle) { return value != handle.value; }
    inline static Handle invalidHandle()         { return Handle(static_cast<T>(0)); }

    inline friend bool operator==(const Handle& h0, const Handle& h1) { return h0.value == h1.value; }
};

typedef Handle<void*,  1> VertexShaderHandle;
typedef Handle<void*,  2> HullShaderHandle;
typedef Handle<void*,  3> DomainShaderHandle;
typedef Handle<void*,  4> GeometryShaderHandle;
typedef Handle<void*,  5> PixelShaderHandle;
typedef Handle<void*,  6> SurfaceShaderHandle; // VS+HS+DS+GS+PS
typedef Handle<void*,  7> ComputeShaderHandle;
typedef Handle<void*,  8> PipelineStateHandle;

typedef Handle<void*,  9> VertexFormatHandle;

typedef Handle<void*, 10> SamplerStateHandle;

// same tag for textures is intended
typedef Handle<void*, 11> TextureHandle;
typedef Handle<void*, 11> Texture1DHandle;
typedef Handle<void*, 11> Texture2DHandle;
typedef Handle<void*, 11> Texture3DHandle;
typedef Handle<void*, 11> CubemapHandle;

// render target
typedef Handle<void*, 12> RenderTargetHandle;

// different tag for buffers is intended
typedef Handle<void*, 13> BufferHandle;
typedef Handle<void*, 14> ConstantBufferHandle;

// draw queue
typedef Handle<void*, 15> DrawQueueHandle;

// compute queue
typedef Handle<void*, 16> ComputeQueueHandle;

// buffers
namespace BufferFlags {
enum : uint32_t {
    VertexBuffer     = (1U << 0), // can be set as a vertex buffer
    IndexBuffer      = (1U << 1), // can be set as an index buffer
    StructuredBuffer = (1U << 2), // can be set as a structured buffer
    CPURead          = (1U << 3), // can be mapped to be read by the CPU
    CPUWrite         = (1U << 4), // can be mapped to be written by the CPU
    GPUWrite         = (1U << 5), // can be written by the GPU
    GPUCounter       = (1U << 6), // can be written by the GPU with atomic counter usage
    GPUAppend        = (1U << 7), // can be appended by the GPU
    StreamOutput     = (1U << 8), // can be used as a stream output buffer
    IndirectArgs     = (1U << 9), // can be used as a drawIndirect args buffer
};
}

enum class MapType : size_t
{
    Read  = 0,
    Write = 1,

    Count
};

namespace TextureFlags {
enum : uint32_t {
    RenderTarget = (1U << 0),
    DepthStencil = (1U << 1),
    CPURead      = (1U << 2),
    GPUWrite     = (1U << 3),
    GPUCounter   = (1U << 4)
};
}

// formats
enum class PrimitiveTopology : size_t
{
    TriangleList,
    TriangleStrip,

    PointList,

    Count
};

enum class TextureFilter : size_t
{
    MinMagMip_Point,
    MinMag_Point_Mip_Linear,
    Min_Point_Mag_Linear_Mip_Point,
    Min_Point_MagMip_Linear,
    Min_Linear_MagMip_Point,
    Min_Linear_Mag_Point_Mip_Linear,
    MinMag_Linear_Mip_Point,
    MinMagMip_Linear,
    Anisotropic,

    Count
};

enum class AddressMode : size_t
{
    Wrap,
    Mirror,
    Clamp,
    Border,

    Count
};

enum class DataFormat : size_t
{
    BC1,    // DXT1
    BC2,    // DXT3
    BC3,    // DXT5
    BC4,    // LATC1/ATI1
    BC5,    // LATC2/ATI2
    BC6H,   // BC6H
    BC7,    // BC7
    ETC1,   // ETC1 RGB8
    ETC2,   // ETC2 RGB8
    ETC2A,  // ETC2 RGBA8
    ETC2A1, // ETC2 RGB8A1
    PTC12,  // PVRTC1 RGB 2BPP
    PTC14,  // PVRTC1 RGB 4BPP
    PTC12A, // PVRTC1 RGBA 2BPP
    PTC14A, // PVRTC1 RGBA 4BPP
    PTC22,  // PVRTC2 RGBA 2BPP
    PTC24,  // PVRTC2 RGBA 4BPP

    UnknownCompressed, // compressed formats above

    R1,
    R8,
    R16,
    R16F,
    R32I,
    R32U,
    R32F,
    RG8,
    RG16,
    RG16F,
    RG32I,
    RG32U,
    RG32F,
    RGB32I,
    RGB32U,
    RGB32F,
    RGBA8,
    RGBA16,
    RGBA16F,
    RGBA32I,
    RGBA32U,
    RGBA32F,
    R11G11B10F,

    UnknownDepth, // depth formats below

    D16,
    D24S8,
    D32F,

    Count
};

inline bool isCompressedFormat(DataFormat format) { return format < DataFormat::UnknownCompressed; }
inline bool isDepthFormat(DataFormat format)      { return format > DataFormat::UnknownDepth;      }

// render state
enum class FillMode : size_t
{
    Solid,
    Wireframe,

    Count
};

enum class CullMode : size_t
{
    Back,
    Front,
    None,

    Count
};

enum class CounterDirection : size_t
{
    CW,
    CCW,

    Count
};

enum class BlendFactor : size_t
{
    Zero,
    One,
    SrcAlpha,
    DstAlpha,
    OneMinusSrcAlpha,
    OneMinusDstAlpha,
    SrcColor,
    DstColor,
    OneMinusSrcColor,
    OneMinusDstColor,

    Count
};

enum class BlendOp : size_t
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,

    Count
};

namespace RenderTargetSlot {
enum {
    Count = 8
};
}

enum class ColorWriteMask : uint8_t
{
    Red   = (1U << 0),
    Green = (1U << 1),
    Blue  = (1U << 2),
    Alpha = (1U << 3),

    All   = Red | Green | Blue | Alpha
};


enum class DepthWriteMask : size_t
{
    Zero,
    All,

    Count
};

enum class ComparisonFunc : size_t
{
    Always,
    Never,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,

    Count
};
typedef ComparisonFunc DepthFunc;
typedef ComparisonFunc StencilFunc;
typedef ComparisonFunc SamplerFunc;

enum class StencilOp : size_t
{
    Keep,
    Zero,
    Replace,
    Increment,
    Decrement,

    Count
};

struct RasterizerState
{
    FillMode         fillMode         = FillMode::Solid;
    CullMode         cullMode         = CullMode::Back;
    CounterDirection counterDirection = CounterDirection::CCW;
};

struct BlendDesc
{
    bool           blendEnabled  = false;
    ColorWriteMask writeMask     = ColorWriteMask::All;
    BlendFactor    srcBlend      = BlendFactor::One;
    BlendFactor    dstBlend      = BlendFactor::Zero;
    BlendOp        blendOp       = BlendOp::Add;
    BlendFactor    srcBlendAlpha = BlendFactor::One;
    BlendFactor    dstBlendAlpha = BlendFactor::Zero;
    BlendOp        blendOpAlpha  = BlendOp::Add;
};

struct BlendState
{
    BlendDesc blendDesc;

    bool      separateBlendEnabled   = false;
    BlendDesc renderTargetBlendDesc[RenderTargetSlot::Count];

    bool      alphaToCoverageEnabled = false;
};

struct StencilDesc
{
    StencilFunc stencilFunc = StencilFunc::Always;
    StencilOp   failOp      = StencilOp::Keep;
    StencilOp   depthFailOp = StencilOp::Keep;
    StencilOp   passOp      = StencilOp::Keep;
};

struct DepthStencilState
{
    bool           depthEnabled   = true;
    DepthWriteMask writeMask      = DepthWriteMask::All;
    DepthFunc      depthFunc      = DepthFunc::Less;

    bool           stencilEnabled = false;
    uint32_t       stencilRef;
    uint8_t        stencilReadMask; // not implemented
    uint8_t        stencilWriteMask;
    StencilDesc    frontFaceStencilDesc;
    StencilDesc    backFaceStencilDesc;
};

struct PipelineStateDescriptor
{
    RasterizerState     rasterizerState;
    BlendState          blendState;
    DepthStencilState   depthStencilState;
    SurfaceShaderHandle shader;
    VertexFormatHandle  vertexFormat;
};

// vertex stage

struct VertexElementDescriptor // TODO: rework to make it more compatible with GL and DX
{
    const char*       semanticName;  // not used on GL
    uint32_t          semanticIndex; // not used on GL
    DataFormat        format;
    uint32_t          slot;          // not used on GL
    uint64_t          offset;
};

struct SamplerStateDescriptor
{
    TextureFilter   filter         = TextureFilter::MinMagMip_Linear;
    AddressMode     addressU       = AddressMode::Clamp;
    AddressMode     addressV       = AddressMode::Clamp;
    AddressMode     addressW       = AddressMode::Clamp;
    float           lodBias        = 0.0F;
    uint32_t        maxAnisotropy  = 1;
    ComparisonFunc  comparisonFunc = ComparisonFunc::Never;
    uint32_t        borderColor    = 0xFFFFFFFF;
    float           minLod         = -3.402823466e+38F;
    float           maxLod         = 3.402823466e+38F;
};

struct RenderTargetDescriptor
{
    uint32_t            numColorTextures = 0;
    sgfx::TextureHandle colorTextures[RenderTargetSlot::Count];
    sgfx::TextureHandle depthStencilTexture;
};

// caps
namespace GPUCaps {
enum : uint64_t {
    // general GPU features
    GeometryShader        = (1UL << 0),
    TessellationShader    = (1UL << 1),
    ComputeShader         = (1UL << 2),
    MultipleRenderTargets = (1UL << 3),
    TextureArray          = (1UL << 4),
    CubemapArray          = (1UL << 5),
    StreamOutput          = (1UL << 6),
    AlphaToCoverage       = (1UL << 7),
    SeparateBlend         = (1UL << 8),
    StructuredBuffer      = (1UL << 9),
    RWStructuredBuffer    = (1UL << 10),

    // texture compression support
    TextureCompressionDXT = (1UL << 11),
    TextureCompressionPVR = (1UL << 12),
    TextureCompressionETC = (1UL << 13),

    // texture format support
    TextureFormatInteger  = (1UL << 14),
    TextureFormatFloat    = (1UL << 15)
};
}

// misc
typedef void(*ErrorReportFunc)(const char*);

//=============================================================================
bool initD3D11(void* d3dDevice, void* d3dContext, void* d3dSwapChain);
bool initD3D12(void* d3dDevice);
bool initOpenGL();
#ifdef NDA_CODE_AMD_MANTLE
// NDACodeStripper v0.17: 1 line removed
#endif

void shutdown();

typedef void* (*AllocFunc)(size_t size);
typedef void  (*FreeFunc)(void* ptr);

// memory
void  setAllocator(AllocFunc nalloc, FreeFunc nfree);
void* allocate(size_t size);
void  deallocate(void* ptr);

uint64_t getGPUCaps();

// shader compiler
enum class ShaderCompileVersion : size_t
{
    v4_0,
    v5_0
};

enum class ShaderCompileTarget : size_t
{
    VS,
    HS,
    DS,
    GS,
    PS,
    CS
};

struct ShaderCompileMacro
{
    const char* name;
    const char* value;
};

namespace ShaderCompileFlags {
enum : uint64_t {
    Debug     = (1UL << 0),
    Strict    = (1UL << 1),
    IEEStrict = (1UL << 2),
    Optimize0 = (1UL << 3),
    Optimize1 = (1UL << 4),
    Optimize2 = (1UL << 5),
    Optimize3 = (1UL << 6)
};
}

bool compileShader(
    const char*          sourceCode,
    size_t               sourceCodeSize,
    ShaderCompileVersion version,
    ShaderCompileTarget  target,
    ShaderCompileMacro*  macros,
    size_t               macrosSize,
    uint64_t             flags,
    ErrorReportFunc      errorFunc,

    void*&  outData, // use deallocate() to dispose this
    size_t& outDataSize
);

// shaders
VertexShaderHandle      createVertexShader(const void* data, size_t dataSize);
void                    releaseVertexShader(VertexShaderHandle handle);

HullShaderHandle        createHullShader(const void* data, size_t dataSize);
void                    releaseHullShader(HullShaderHandle handle);

DomainShaderHandle      createDomainShader(const void* data, size_t dataSize);
void                    releaseDomainShader(DomainShaderHandle handle);

GeometryShaderHandle    createGeometryShader(const void* data, size_t dataSize);
void                    releaseGeometryShader(GeometryShaderHandle handle);

PixelShaderHandle       createPixelShader(const void* data, size_t dataSize);
void                    releasePixelShader(PixelShaderHandle handle);

SurfaceShaderHandle     linkSurfaceShader(
    VertexShaderHandle   vs,
    HullShaderHandle     hs,
    DomainShaderHandle   ds,
    GeometryShaderHandle gs,
    PixelShaderHandle    ps
);
void                    releaseSurfaceShader(SurfaceShaderHandle handle);

// compute shader stuff
ComputeQueueHandle      createComputeQueue(ComputeShaderHandle shader);
void                    releaseComputeQueue(ComputeQueueHandle handle);

void                    setConstantBuffer(ComputeQueueHandle handle, uint32_t idx, ConstantBufferHandle buffer);
void                    setResource(ComputeQueueHandle handle, uint32_t idx, BufferHandle resource);
void                    setResource(ComputeQueueHandle handle, uint32_t idx, TextureHandle resource);
void                    setResourceRW(ComputeQueueHandle handle, uint32_t idx, BufferHandle resource);

void                    submit(ComputeQueueHandle handle, uint32_t x, uint32_t y, uint32_t z);

ComputeShaderHandle     createComputeShader(const void* data, size_t dataSize);
void                    releaseComputeShader(ComputeShaderHandle handle);

// pipeline state
VertexFormatHandle      createVertexFormat(
    VertexElementDescriptor* elements,
    size_t                   size,
    void* shaderBytecode, size_t shaderBytecodeSize,
    ErrorReportFunc          errorReport
);
void                    releaseVertexFormat(VertexFormatHandle handle);

PipelineStateHandle     createPipelineState(const PipelineStateDescriptor& desc);
void                    releasePipelineState(PipelineStateHandle handle);

// buffers
BufferHandle            createBuffer(uint32_t flags, const void* mem, size_t size, size_t stride);
void                    releaseBuffer(BufferHandle handle);
void*                   mapBuffer(BufferHandle handle, MapType type);
void                    unmapBuffer(BufferHandle handle);
void                    copyBufferData(BufferHandle handle, size_t offset, size_t size, const void* mem);

void                    clearBufferRW(BufferHandle handle, uint32_t value);
void                    clearBufferRW(BufferHandle handle, float    value);

ConstantBufferHandle    createConstantBuffer(const void* mem, size_t size);
void                    updateConstantBuffer(ConstantBufferHandle handle, const void* mem);
void                    releaseConstantBuffer(ConstantBufferHandle handle);

// textures
SamplerStateHandle      createSamplerState(const SamplerStateDescriptor& desc);
void                    releaseSamplerState(SamplerStateHandle handle);

Texture1DHandle         createTexture1D(uint32_t width, DataFormat format, uint32_t numMipmaps, uint32_t flags);
Texture2DHandle         createTexture2D(uint32_t width, uint32_t height, DataFormat format, uint32_t numMipmaps, uint32_t flags);
Texture3DHandle         createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DataFormat format, uint32_t numMipmaps, uint32_t flags);

void                    clearTextureRW(TextureHandle handle, uint32_t value);
void                    clearTextureRW(TextureHandle handle, float    value);

void*                   mapTexture(TextureHandle handle, MapType type);
void                    unmapTexture(TextureHandle handle);

void                    updateTexture(
    TextureHandle handle, const void* mem,
    uint32_t mip,
    size_t offsetX,  size_t sizeX,
    size_t offsetY,  size_t sizeY,
    size_t offsetZ,  size_t sizeZ,
    size_t rowPitch, size_t depthPitch
);
void                    releaseTexture(TextureHandle handle);

// async buffer copying
void                    copyResource(TextureHandle        src, TextureHandle        dst);
void                    copyResource(BufferHandle         src, BufferHandle         dst);
void                    copyResource(ConstantBufferHandle src, ConstantBufferHandle dst);

// render targets
Texture2DHandle         getBackBuffer();

RenderTargetHandle      createRenderTarget(const RenderTargetDescriptor& desc);
void                    releaseRenderTarget(RenderTargetHandle handle);

void                    setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth);

void                    setResourceRW(RenderTargetHandle handle, uint32_t slot, BufferHandle resource);
void                    setResourceRW(RenderTargetHandle handle, uint32_t slot, TextureHandle resource);

void                    setRenderTarget(RenderTargetHandle handle);
void                    clearRenderTarget(RenderTargetHandle handle, uint32_t color);
void                    clearRenderTarget(RenderTargetHandle handle, uint32_t slot, uint32_t color);
void                    clearDepthStencil(RenderTargetHandle handle, float depth, uint8_t stencil);
void                    present(uint32_t swapInterval);

// drawing
DrawQueueHandle         createDrawQueue(PipelineStateHandle state);
void                    releaseDrawQueue(DrawQueueHandle handle);

// per queue
void                    setSamplerState(DrawQueueHandle handle, uint32_t idx, SamplerStateHandle sampler);

// per draw call
void                    setPrimitiveTopology(DrawQueueHandle qd, PrimitiveTopology topology);
void                    setVertexBuffer(DrawQueueHandle dq, BufferHandle vb);
void                    setIndexBuffer(DrawQueueHandle dq, BufferHandle ib);

void                    setConstantBuffer(DrawQueueHandle handle, uint32_t idx, ConstantBufferHandle buffer);
void                    setResource(DrawQueueHandle handle, uint32_t idx, BufferHandle resource);
void                    setResource(DrawQueueHandle handle, uint32_t idx, TextureHandle resource);

void                    draw(DrawQueueHandle dq, uint32_t count, uint32_t startVertex);
void                    drawIndexed(DrawQueueHandle dq, uint32_t count, uint32_t startIndex, uint32_t startVertex);

void                    drawInstanced(DrawQueueHandle dq, uint32_t instanceCount, uint32_t count, uint32_t startVertex);
void                    drawIndexedInstanced(DrawQueueHandle dq, uint32_t instanceCount, uint32_t count, uint32_t startIndex, uint32_t startVertex);

void                    drawInstancedIndirect(DrawQueueHandle dq, BufferHandle indirectArgs, size_t argsOffset);
void                    drawIndexedInstancedIndirect(DrawQueueHandle dq, BufferHandle indirectArgs, size_t argsOffset);

void                    submit(DrawQueueHandle handle);

// performance markers
void                    beginPerfEvent(const wchar_t* name);
void                    endPerfEvent();

// optional interop with D3D11
#ifdef SGFX_D3D11_INTEROP
namespace d3d11
{

ID3D11Buffer*               getNativeBuffer(ConstantBufferHandle handle);

ID3D11Resource*             getNativeResource(BufferHandle handle);
ID3D11ShaderResourceView*   getNativeSRV(BufferHandle handle);
ID3D11UnorderedAccessView*  getNativeUAV(BufferHandle handle);

ID3D11Resource*             getNativeResource(TextureHandle handle);
ID3D11ShaderResourceView*   getNativeSRV(TextureHandle handle);
ID3D11UnorderedAccessView*  getNativeUAV(TextureHandle handle);

ID3D11SamplerState*         getNativeSamplerState(SamplerStateHandle handle);

ID3D11RenderTargetView*     getNativeRTV(RenderTargetHandle handle, size_t idx);
ID3D11DepthStencilView*     getNativeDSV(RenderTargetHandle handle);

}
#endif

// internal classes and data
#ifdef SGFX_INTERNAL_IMPLEMENTATION

namespace SGFX_NS_INTERNAL
{

// default array allocator
struct DefaultAllocator
{
    static inline uint8_t* Allocate(size_t size) { return reinterpret_cast<uint8_t*>(allocate(size)); }
    static inline void     Free(uint8_t* ptr)    { deallocate(ptr); }
};

///
/// ImmutableArray represents an abstract sequence container that cannot change in size.
///
/// It is guaranteed that all the data in this container uses contiguous storage locations for
/// its elements, which means that the elements can also be accessed using offsets on regular
/// pointers to its elements, and just as efficiently as in C arrays.
///
template <typename T>
class ImmutableArray
{
protected:

    size_t capacity  = 0;
    size_t size      = 0;

    T* pointer = nullptr;

    ImmutableArray() {}
    ImmutableArray(const ImmutableArray& other) = delete;
    virtual ~ImmutableArray() {}

public:

    SGFX_FORCE_INLINE T*       GetData()       { return pointer; }
    SGFX_FORCE_INLINE const T* GetData() const { return pointer; }

    SGFX_FORCE_INLINE       T& operator[](size_t index)       { return pointer[index]; }
    SGFX_FORCE_INLINE const T& operator[](size_t index) const { return pointer[index]; }

    // range for support
    SGFX_FORCE_INLINE T* begin() const { return pointer; }
    SGFX_FORCE_INLINE T* end()   const { return pointer + size; }

    // size and capacity
    SGFX_FORCE_INLINE size_t GetSize() const     { return size; }
    SGFX_FORCE_INLINE size_t GetCapacity() const { return capacity; }

    // utils
    SGFX_FORCE_INLINE bool IsEmpty() const { return size == 0; }

    SGFX_FORCE_INLINE ptrdiff_t Find(const T& e)
    {
        for (size_t i = 0; i < GetSize(); ++i)
            if (pointer[i] == e)
                return i;
        return -1;
    }

    template <typename Pred>
    SGFX_FORCE_INLINE ptrdiff_t Find(const Pred& pred)
    {
        for (size_t i = 0; i < GetSize(); ++i)
            if (pred(pointer[i]))
                return i;
        return -1;
    }
};

///
/// DynamicArray is a concrete sequence container representing a contiguous array that can change in
/// size.
///
/// Just like ImmutableArray, DynamicArray uses contiguous storage locations for its elements.
/// But unlike immutable arrays, dynamic array size can change dynamically, with its storage being
/// handled automatically by the container.
///
/// Internally, DynamicArray uses a dynamically allocated array to store its elements. This array
/// may need to be reallocated in order to grow in size when new elements are inserted, which
/// implies allocating a new array and moving all elements to it. This is a relatively expensive
/// task in terms of processing time, and thus, dynamic arrays do not reallocate each time an
/// element is added to the container, instead DynamicArray::kGrowAmount is used to control growing
/// size.
///
/// DynamicArray is very efficient with random access pattern, while adding and removing elements
/// is less effective compared to List.
///
template <typename T, size_t I = 32, size_t G = 64, typename A = DefaultAllocator>
class DynamicArray final : public ImmutableArray<T>
{
protected:

    using ImmutableArray<T>::capacity;
    using ImmutableArray<T>::size;
    using ImmutableArray<T>::pointer;

private:

    enum
    {
        kInplaceStorageSize = I,
        kGrowAmount         = G
    };

    uint8_t _inplaceStorage[kInplaceStorageSize * sizeof(T)];

    SGFX_FORCE_INLINE void DeleteContents()
    {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(pointer);
        if (ptr != _inplaceStorage) {
            A::Free(ptr);
            pointer = reinterpret_cast<T*>(_inplaceStorage);
        }
    }

public:

    using ImmutableArray<T>::GetData;
    using ImmutableArray<T>::IsEmpty;
    using ImmutableArray<T>::Find;

    SGFX_FORCE_INLINE DynamicArray()
    {
        capacity = kInplaceStorageSize;
        pointer  = reinterpret_cast<T*>(_inplaceStorage);
    }

    SGFX_FORCE_INLINE DynamicArray(const DynamicArray& other)
    {
        capacity = kInplaceStorageSize;
        pointer  = reinterpret_cast<T*>(_inplaceStorage);

        Resize(other.GetSize());
        for (size_t i = 0; i < size; ++i)
            ::new (&pointer[i]) T(other[i]);
    }

    SGFX_FORCE_INLINE DynamicArray& operator=(const DynamicArray& other)
    {
        Resize(other.GetSize());
        for (size_t i = 0; i < size; ++i)
            ::new (&pointer[i]) T(other[i]);
        return *this;
    }

    SGFX_FORCE_INLINE DynamicArray& operator=(const ImmutableArray<T>& other)
    {
        Resize(other.GetSize());
        for (size_t i = 0; i < size; ++i)
            ::new (&pointer[i]) T(other[i]);
        return *this;
    }

    SGFX_FORCE_INLINE ~DynamicArray()
    {
        Purge();
    }

    SGFX_FORCE_INLINE void Clear()
    {
        for (size_t i = 0; i < size; ++i)
            pointer[i].~T();
        size = 0;
    }

    SGFX_FORCE_INLINE void Purge()
    {
        Clear();
        DeleteContents();

        size = 0;
        capacity = kInplaceStorageSize;
    }

    SGFX_FORCE_INLINE void Resize(size_t newSize)
    {
        if (newSize == size) return; // fool protection

        if (newSize > capacity) {
            Grow(newSize - size);
        }

        if (newSize < size) {
            for (size_t i = newSize; i < size; ++i)
                pointer[i].~T();
        } else {
            for (size_t i = size; i < newSize; ++i)
                ::new (&pointer[i]) T();
        }
        size = newSize;
    }

    SGFX_FORCE_INLINE void Reserve(size_t numElements)
    {
        if (numElements > capacity)
            Grow(numElements - capacity);
    }

    SGFX_FORCE_INLINE void Grow(size_t numElements)
    {
        size_t newCapacity = size + numElements;
        capacity = newCapacity;

        if (newCapacity > kInplaceStorageSize) {
            T* ptr = reinterpret_cast<T*>(A::Allocate(newCapacity * sizeof(T)));

            for (size_t i = 0; i < size; ++i)
                ::new (&ptr[i]) T(static_cast<T&&>(pointer[i]));

            DeleteContents();

            pointer = ptr;
        }
    }

    SGFX_FORCE_INLINE void Merge(const DynamicArray& other)
    {
        if (!other.IsEmpty()) {
            Reserve(GetSize() + other.GetSize());
            for (const T& element : other)
                Add(element);
        }
    }

    SGFX_FORCE_INLINE void Add(const T& element)
    {
        if (size >= capacity)
            Grow(kGrowAmount);

        T* ptr = pointer + size;
        ::new (ptr)T(element);
        size++;
    }

    template <typename ...Args>
    SGFX_FORCE_INLINE void EmplaceAdd(Args&&... args)
    {
        if (size >= capacity)
            Grow(kGrowAmount);

        T* ptr = pointer + size;
        ::new (ptr)T(static_cast<Args&&>(args)...);
        size++;
    }

    SGFX_FORCE_INLINE void Remove(size_t index)
    {
        if (index < size) {
            pointer[index].~T();

            T* ptr = pointer + index;
            for (size_t i = index + 1; i < size; ++i) {
                ::new (ptr)T(static_cast<T&&>(pointer[i]));
                ptr++;
                ptr->~T();
            }

            size--;
        }
    }

    SGFX_FORCE_INLINE void Remove(const T& element)
    {
        ptrdiff_t index = Find(element);
        if (index != -1)
            Remove(index);
    }
};

// emulated draw queues for pre-DX12 APIs (DX11 and GL4)
struct ShaderResource final
{
    bool  isTexture = false;
    void* value     = nullptr;

    inline ShaderResource() {}
    inline ShaderResource(bool texture, void* newHandle)
        : isTexture(texture), value(newHandle)
    {}
};

struct DrawCall final
{
    enum Type : uint32_t
    {
        Draw                            = 0,
        DrawIndexed                     = 1,
        DrawInstanced                   = 2,
        DrawIndexedInstanced            = 3,

        DrawInstancedIndirect           = 4,
        DrawIndexedInstancedIndirect    = 5
    };

    enum
    {
        kMaxConstantBuffers     = 8,
        kMaxShaderResources     = 128,
    };

    ConstantBufferHandle    constantBuffers[kMaxConstantBuffers];
    ShaderResource          shaderResources[kMaxShaderResources];

    BufferHandle      vertexBuffer;
    BufferHandle      indexBuffer;
    BufferHandle      indirectArgsBuffer;
    size_t            indirectArgsOffset;
    PrimitiveTopology primitiveTopology;

    uint32_t instanceCount;
    uint32_t count;
    uint32_t startVertex;
    uint32_t startIndex;
    Type     type;
};

class DrawQueue final
{
public:

    typedef DynamicArray<DrawCall, 4096, 4096> DrawCallArray;

private:
    PipelineStateHandle state;
    DrawCall            currentDrawCall;
    DrawCallArray       drawCalls;

public:

    enum
    {
        kMaxSamplerStates = 8
    };
    SamplerStateHandle samplerStates[kMaxSamplerStates];

    DrawQueue(PipelineStateHandle _state) : state(_state) {}

    SGFX_FORCE_INLINE PipelineStateHandle  getState() const     { return state; }
    SGFX_FORCE_INLINE const DrawCallArray& getDrawCalls() const { return drawCalls; }

    SGFX_FORCE_INLINE void clear()
    {
        drawCalls.Clear();
    }

    SGFX_FORCE_INLINE void setPrimitiveTopology(PrimitiveTopology topology)     { currentDrawCall.primitiveTopology = topology; }
    SGFX_FORCE_INLINE void setVertexBuffer(BufferHandle handle)                 { currentDrawCall.vertexBuffer = handle; }
    SGFX_FORCE_INLINE void setIndexBuffer(BufferHandle handle)                  { currentDrawCall.indexBuffer  = handle; }

    SGFX_FORCE_INLINE void setSamplerState(uint32_t idx, SamplerStateHandle handle)
    {
        samplerStates[idx] = handle;
    }

    SGFX_FORCE_INLINE void setConstantBuffer(uint32_t idx, ConstantBufferHandle resource)
    {
        currentDrawCall.constantBuffers[idx] = resource;
    }

    SGFX_FORCE_INLINE void setResource(uint32_t idx, BufferHandle resource)
    {
        currentDrawCall.shaderResources[idx] = ShaderResource(false, resource.value);
    }

    SGFX_FORCE_INLINE void setResource(uint32_t idx, TextureHandle resource)
    {
        currentDrawCall.shaderResources[idx] = ShaderResource(true, resource.value);
    }

    SGFX_FORCE_INLINE void draw(uint32_t count, uint32_t startVertex)
    {
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = 0;
        currentDrawCall.type        = DrawCall::Draw;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    SGFX_FORCE_INLINE void drawIndexed(uint32_t count, uint32_t startIndex, uint32_t startVertex)
    {
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = startIndex;
        currentDrawCall.type        = DrawCall::DrawIndexed;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    SGFX_FORCE_INLINE void drawInstanced(uint32_t instanceCount, uint32_t count, uint32_t startVertex)
    {
        currentDrawCall.instanceCount = instanceCount;
        currentDrawCall.count         = count;
        currentDrawCall.startVertex   = startVertex;
        currentDrawCall.startIndex    = 0;
        currentDrawCall.type          = DrawCall::DrawInstanced;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    SGFX_FORCE_INLINE void drawIndexedInstanced(uint32_t instanceCount, uint32_t count, uint32_t startIndex, uint32_t startVertex)
    {
        currentDrawCall.instanceCount = instanceCount;
        currentDrawCall.count         = count;
        currentDrawCall.startVertex   = startVertex;
        currentDrawCall.startIndex    = startIndex;
        currentDrawCall.type          = DrawCall::DrawIndexedInstanced;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    SGFX_FORCE_INLINE void drawInstancedIndirect(BufferHandle indirectArgs, size_t argsOffset)
    {
        currentDrawCall.type                = DrawCall::DrawInstancedIndirect;
        currentDrawCall.indirectArgsBuffer  = indirectArgs;
        currentDrawCall.indirectArgsOffset  = argsOffset;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    SGFX_FORCE_INLINE void drawIndexedInstancedIndirect(BufferHandle indirectArgs, size_t argsOffset)
    {
        currentDrawCall.type                = DrawCall::DrawIndexedInstancedIndirect;
        currentDrawCall.indirectArgsBuffer  = indirectArgs;
        currentDrawCall.indirectArgsOffset  = argsOffset;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }
};

struct ComputeQueue final
{
    enum
    {
        kMaxSamplerStates = 8
    };

    enum
    {
        kMaxConstantBuffers = 8,
        kMaxShaderResources = 128,
        kMaxShaderResourcesRW = 8
    };

    SamplerStateHandle      samplerStates[kMaxSamplerStates];
    ConstantBufferHandle    constantBuffers[kMaxConstantBuffers];
    ShaderResource          shaderResources[kMaxShaderResources];
    ShaderResource          shaderResourcesRW[kMaxShaderResourcesRW];

    ComputeShaderHandle     shader;

    SGFX_FORCE_INLINE void setConstantBuffer(uint32_t idx, ConstantBufferHandle resource)
    {
        constantBuffers[idx] = resource;
    }

    SGFX_FORCE_INLINE void setResource(uint32_t idx, BufferHandle resource)
    {
        shaderResources[idx] = ShaderResource(false, resource.value);
    }

    SGFX_FORCE_INLINE void setResource(uint32_t idx, TextureHandle resource)
    {
        shaderResources[idx] = ShaderResource(true, resource.value);
    }

    SGFX_FORCE_INLINE void setResourceRW(uint32_t idx, BufferHandle resource)
    {
        shaderResourcesRW[idx] = ShaderResource(false, resource.value);
    }
};

}
#endif

}

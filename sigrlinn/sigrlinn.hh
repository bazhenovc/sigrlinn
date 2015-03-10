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

// same tag for buffers is intended
typedef Handle<void*, 13> BufferHandle;
typedef Handle<void*, 14> ConstantBufferHandle;

// draw queue
typedef Handle<void*, 15> DrawQueueHandle;

// buffers
namespace BufferFlags {
enum : uint32_t {
    VertexBuffer     = (1U << 0), // can be set as a vertex buffer
    IndexBuffer      = (1U << 1), // can be set as an index buffer
    StructuredBuffer = (1U << 2), // can be set as a structured buffer
    CPURead          = (1U << 3), // can be mapped to be read by the CPU
    CPUWrite         = (1U << 4), // can be mapped to be written by the CPU
    GPUWrite         = (1U << 5), // can be written by the GPU
    StreamOutput     = (1U << 6), // can be used as a stream output buffer
};
}

enum class MapType : uint32_t
{
    Read  = 0,
    Write = 1,

    Count
};

namespace TextureFlags {
enum : uint32_t {
    RenderTarget = (1U << 0),
    DepthStencil = (1U << 1)
};
}

// formats
enum class PrimitiveTopology : uint64_t
{
    TriangleList,
    TriangleStrip,

    Count
};

enum class TextureFilter : uint32_t
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

enum class AddressMode : uint32_t
{
    Wrap,
    Mirror,
    Clamp,
    Border,

    Count
};

enum class DataFormat : uint32_t
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
    RG32,
    RG32F,
    RGB32,
    RGB32F,
    RGBA8,
    RGBA16,
    RGBA16F,
    RGBA32,
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
enum class FillMode : uint64_t
{
    Solid,
    Wireframe,

    Count
};

enum class CullMode : uint64_t
{
    Back,
    Front,
    None,

    Count
};

enum class CounterDirection : uint64_t
{
    CW,
    CCW,

    Count
};

enum class BlendFactor : uint64_t
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

enum class BlendOp : uint64_t
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,

    Count
};

enum RenderTargetSlot :uint64_t
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,

    Count
};

enum class ColorWriteMask : uint8_t // unsupported
{
    Red,
    Green,
    Blue,
    Alpha,
    All,

    Count
};

enum class DepthWriteMask : uint64_t
{
    Zero,
    All,

    Count
};

enum class ComparisonFunc : uint64_t
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

enum class StencilOp : uint64_t
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
    ColorWriteMask writeMask     = ColorWriteMask::All; // not implemented
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
enum class VertexElementType : uint64_t
{
    PerVertex,
    PerInstance,

    Count
};

struct VertexElementDescriptor
{
    const char*       semanticName;
    uint32_t          semanticIndex;
    DataFormat        format;
    uint32_t          slot;
    uint64_t          offset;
    VertexElementType type;
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
enum class GPUCaps : uint64_t
{
    GeometryShader        = (1UL << 0),
    TessellationShader    = (1UL << 1),
    ComputeShader         = (1UL << 2),
    MultipleRenderTargets = (1UL << 3)
};

// misc
typedef void(*ErrorReportFunc)(const char*);

//=============================================================================
bool initD3D11(void* d3dDevice, void* d3dContext, void* d3dSwapChain);
bool initOpenGL();
#ifdef NDA_CODE_AMD_MANTLE
// NDACodeStripper v0.17: 1 line removed
#endif

void shutdown();

uint64_t getGPUCaps();

// shader compiler
enum class ShaderCompileVersion : uint64_t
{
    v4_0,
    v5_0
};

enum class ShaderCompileTarget : uint64_t
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

    void*&  outData, // delete []
    size_t& outDataSize
);

// shaders
VertexShaderHandle   createVertexShader(const void* data, size_t dataSize);
void                 releaseVertexShader(VertexShaderHandle handle);

HullShaderHandle     createHullShader(const void* data, size_t dataSize);
void                 releaseHullShader(HullShaderHandle handle);

DomainShaderHandle   createDomainShader(const void* data, size_t dataSize);
void                 releaseDomainShader(DomainShaderHandle handle);

GeometryShaderHandle createGeometryShader(const void* data, size_t dataSize);
void                 releaseGeometryShader(GeometryShaderHandle handle);

PixelShaderHandle    createPixelShader(const void* data, size_t dataSize);
void                 releasePixelShader(PixelShaderHandle handle);

SurfaceShaderHandle  linkSurfaceShader(
    VertexShaderHandle   vs,
    HullShaderHandle     hs,
    DomainShaderHandle   ds,
    GeometryShaderHandle gs,
    PixelShaderHandle    ps
);
void releaseSurfaceShader(SurfaceShaderHandle handle);

ComputeShaderHandle createComputeShader(const void* data, size_t dataSize);
void                releaseComputeShader(ComputeShaderHandle handle);
void                dispatchComputeShader(ComputeShaderHandle handle, uint32_t x, uint32_t y, uint32_t z);

// pipeline state
VertexFormatHandle createVertexFormat(
    VertexElementDescriptor* elements,
    size_t                   size,
    void* shaderBytecode, size_t shaderBytecodeSize,
    ErrorReportFunc          errorReport
);
void                  releaseVertexFormat(VertexFormatHandle handle);

PipelineStateHandle   createPipelineState(const PipelineStateDescriptor& desc);
void                  releasePipelineState(PipelineStateHandle handle);

// buffers
BufferHandle          createBuffer(uint32_t flags, const void* mem, size_t size, size_t stride);
void                  releaseBuffer(BufferHandle handle); // use this to release all kinds of buffers
void*                 mapBuffer(BufferHandle handle, MapType type);
void                  unmapBuffer(BufferHandle handle);

ConstantBufferHandle  createConstantBuffer(const void* mem, size_t size);
void                  updateConstantBuffer(ConstantBufferHandle handle, const void* mem);
void                  releaseConstantBuffer(ConstantBufferHandle handle);

// textures
SamplerStateHandle createSamplerState(const SamplerStateDescriptor& desc);
void               releaseSamplerState(SamplerStateHandle handle);

Texture1DHandle createTexture1D(uint32_t width, DataFormat format, size_t numMipmaps, uint32_t flags);
Texture2DHandle createTexture2D(uint32_t width, uint32_t height, DataFormat format, size_t numMipmaps, uint32_t flags);
Texture3DHandle createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DataFormat format, size_t numMipmaps, uint32_t flags);

void            updateTexture(
    TextureHandle handle, const void* mem,
    uint32_t mip,
    size_t offsetX,  size_t sizeX,
    size_t offsetY,  size_t sizeY,
    size_t offsetZ,  size_t sizeZ,
    size_t rowPitch, size_t depthPitch
);
void            releaseTexture(TextureHandle handle);

// render targets
Texture2DHandle    getBackBuffer();

RenderTargetHandle createRenderTarget(const RenderTargetDescriptor& desc);
void               releaseRenderTarget(RenderTargetHandle handle);

void               setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth);

void               setRenderTarget(RenderTargetHandle handle);
void               clearRenderTarget(RenderTargetHandle handle, uint32_t color);
void               clearRenderTarget(RenderTargetHandle handle, uint32_t slot, uint32_t color);
void               clearDepthStencil(RenderTargetHandle handle, float depth, uint8_t stencil);
void               present();

// drawing
DrawQueueHandle createDrawQueue(PipelineStateHandle state);
void            releaseDrawQueue(DrawQueueHandle handle);

// per queue
void            setSamplerState(DrawQueueHandle handle, uint32_t idx, SamplerStateHandle sampler);

// per draw call
void            setPrimitiveTopology(DrawQueueHandle qd, PrimitiveTopology topology);
void            setVertexBuffer(DrawQueueHandle dq, BufferHandle vb);
void            setIndexBuffer(DrawQueueHandle dq, BufferHandle ib);

void            setConstantBuffer(DrawQueueHandle handle, uint32_t idx, ConstantBufferHandle buffer);
void            setResource(DrawQueueHandle handle, uint32_t idx, BufferHandle resource);
void            setResource(DrawQueueHandle handle, uint32_t idx, TextureHandle resource);
void            setResourceRW(DrawQueueHandle handle, uint32_t idx, BufferHandle resource);

void            draw(DrawQueueHandle dq, uint32_t count, uint32_t startVertex);
void            drawIndexed(DrawQueueHandle dq, uint32_t count, uint32_t startIndex, uint32_t startVertex);

void            drawInstanced(DrawQueueHandle dq, uint32_t instanceCount, uint32_t count, uint32_t startVertex);
void            drawIndexedInstanced(DrawQueueHandle dq, uint32_t instanceCount, uint32_t count, uint32_t startIndex, uint32_t startVertex);

void            submit(DrawQueueHandle handle);

}

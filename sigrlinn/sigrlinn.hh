
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

typedef Handle<void*, 10> SamplerStateHandle;

// same tag for textures is intended
typedef Handle<void*, 11> TextureHandle;
typedef Handle<void*, 11> Texture1DHandle;
typedef Handle<void*, 11> Texture2DHandle;
typedef Handle<void*, 11> Texture3DHandle;
typedef Handle<void*, 11> CubemapHandle;

// same tag for buffers is intended
typedef Handle<void*, 12> BufferHandle;          // cannot be changed after creation
typedef Handle<void*, 12> TransientBufferHandle; // can be updated
typedef Handle<void*, 12> DynamicBufferHandle;   // can be mapped

// draw queue
typedef Handle<void*, 13> DrawQueueHandle;

// buffers
enum class BufferType
{
    Vertex,
    Index
};

enum class TransientBufferType
{
    Vertex,
    Index,
    Constant,
    Storage
};

typedef TransientBufferType DynamicBufferType;

// formats

enum class PrimitiveTopology : uint64_t
{
    TriangleList,
    TriangleStrip,

    Count
};

enum class DataFormat : uint64_t
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
inline bool isDepthFormat(DataFormat format)      { return format < DataFormat::UnknownDepth;      }

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
};

// caps
enum class GPUCaps : uint64_t
{
    GeometryShader        = 1UL << 0,
    TessellationShader    = 1UL << 1,
    ComputeShader         = 1UL << 2,
    MultipleRenderTargets = 1UL << 3
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

enum class ShaderCompileFlags : uint64_t
{
    Debug     = 1 << 0,
    Strict    = 1 << 1,
    IEEStrict = 1 << 2,
    Optimize0 = 1 << 3,
    Optimize1 = 1 << 4,
    Optimize2 = 1 << 5,
    Optimize3 = 1 << 6
};

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
PipelineStateHandle createPipelineState(const PipelineStateDescriptor& desc);
void                releasePipelineState(PipelineStateHandle handle);

// buffers
BufferHandle          createBuffer(BufferType type, void* mem, size_t size, size_t stride);
void                  releaseBuffer(BufferHandle handle); // use this to release all kinds of buffers

#if 0 // do not use, transient buffers are temporary not compatible with vertex/index buffers
TransientBufferHandle createTransientBuffer(TransientBufferType type, void* mem, size_t size);
void                  updateTransientBuffer(TransientBufferHandle handle, void* mem, size_t size, size_t offset);
#endif

// textures
Texture1DHandle createTexture1D(uint32_t width, DataFormat format, size_t numMipmaps);
Texture2DHandle createTexture2D(uint32_t width, uint32_t height, DataFormat format, size_t numMipmaps);
Texture3DHandle createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DataFormat format, size_t numMipmaps);
CubemapHandle   createCubemap(uint32_t sideWidth, uint32_t sideHeight, DataFormat format, size_t numMipmaps);

// drawing
DrawQueueHandle createDrawQueue(PipelineStateHandle state);
void            releaseDrawQueue(DrawQueueHandle handle);

void            setPrimitiveTopology(DrawQueueHandle qd, PrimitiveTopology topology);
void            setVertexBuffer(DrawQueueHandle dq, BufferHandle vb);
void            setIndexBuffer(DrawQueueHandle dq, BufferHandle ib);
void            setConstants(DrawQueueHandle dq, uint32_t idx, void* constantsData, size_t constantsSize);

void            draw(DrawQueueHandle dq, uint32_t count, uint32_t startVertex);
void            drawIndexed(DrawQueueHandle dq, uint32_t count, uint32_t startIndex, uint32_t startVertex);

void            submit(DrawQueueHandle handle);

}


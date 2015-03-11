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

#include "GL/glew.h"
#include <memory>

#pragma comment(lib, "opengl32.lib")

namespace sgfx
{

struct GLTexFilter final
{
    GLenum min;
    GLenum mag;

    GLTexFilter(GLenum nmin, GLenum nmag)
        : min(nmin), mag(nmag)
    {}
};

static GLenum MapMapType[MapType::Count] = {
    GL_READ_ONLY,
    GL_WRITE_ONLY
};
static_assert((sizeof(MapMapType) / sizeof(GLenum)) == static_cast<uint32_t>(MapType::Count), "Mapping is broken!");

static GLenum MapPrimitiveTopology[PrimitiveTopology::Count] = {
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP
};
static_assert((sizeof(MapPrimitiveTopology) / sizeof(GLenum)) == static_cast<size_t>(PrimitiveTopology::Count), "Mapping is broken!");

static GLTexFilter MapTextureFilter[TextureFilter::Count] = {
    GLTexFilter(GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST),
    GLTexFilter(GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR),
    GLTexFilter(GL_NEAREST, GL_LINEAR_MIPMAP_NEAREST),
    GLTexFilter(GL_NEAREST, GL_LINEAR_MIPMAP_LINEAR),
    GLTexFilter(GL_LINEAR,  GL_NEAREST_MIPMAP_NEAREST),
    GLTexFilter(GL_LINEAR,  GL_NEAREST_MIPMAP_LINEAR),
    GLTexFilter(GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST),
    GLTexFilter(GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR),
    GLTexFilter(GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR) // Anisotropic filter is set implicitly after GL_MAX_ANISOTROPY is specified
};
static_assert((sizeof(MapTextureFilter) / sizeof(GLTexFilter)) == static_cast<size_t>(TextureFilter::Count), "Mapping is broken!");

static GLenum MapAddressMode[AddressMode::Count] = {
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER
};
static_assert((sizeof(MapAddressMode) / sizeof(GLenum)) == static_cast<size_t>(AddressMode::Count), "Mapping is broken!");

static GLenum MapDataFormat[DataFormat::Count] = {
    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,         // DXT1
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,        // DXT3
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,        // DXT5
    GL_COMPRESSED_LUMINANCE_LATC1_EXT,       // LATC1/ATI1
    GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT, // LATC2/ATI2
    GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,     // BC6H
    GL_COMPRESSED_RGBA_BPTC_UNORM,           // BC7
    GL_NONE,                                 // ETC1 RGB8
    GL_NONE,                                 // ETC2 RGB8
    GL_NONE,                                 // ETC2 RGBA8
    GL_NONE,                                 // ETC2 RGB8A1
    GL_NONE,                                 // PVRTC1 RGB 2BPP
    GL_NONE,                                 // PVRTC1 RGB 4BPP
    GL_NONE,                                 // PVRTC1 RGBA 2BPP
    GL_NONE,                                 // PVRTC1 RGBA 4BPP
    GL_NONE,                                 // PVRTC2 RGBA 2BPP
    GL_NONE,                                 // PVRTC2 RGBA 4BPP

    GL_NONE, // compressed formats above

    GL_R,
    GL_R8,
    GL_R16,
    GL_R16F,
    GL_R32I,
    GL_R32UI,
    GL_R32F,
    GL_RG8,
    GL_RG16,
    GL_RG16F,
    GL_RG32I,
    GL_RG32F,
    GL_RGB32I,
    GL_RGB32F,
    GL_RGBA8,
    GL_RGBA16,
    GL_RGBA16F,
    GL_RGBA32I,
    GL_RGBA32F,
    GL_R11F_G11F_B10F,

    GL_NONE, // depth formats below

    GL_DEPTH_COMPONENT16,
    GL_DEPTH24_STENCIL8,
    GL_DEPTH_COMPONENT32F,
};
static_assert((sizeof(MapDataFormat) / sizeof(GLenum)) == static_cast<size_t>(DataFormat::Count), "Mapping is broken!");

static GLenum MapFillMode[FillMode::Count] = {
    GL_FILL,
    GL_LINE
};
static_assert((sizeof(MapFillMode) / sizeof(GLenum)) == static_cast<size_t>(FillMode::Count), "Mapping is broken!");

static GLenum MapCullMode[CullMode::Count] = {
    GL_BACK,
    GL_FRONT
};
static_assert((sizeof(MapCullMode) / sizeof(GLenum)) == static_cast<size_t>(CullMode::Count), "Mapping is broken!");

static GLenum MapCounterDirection[CounterDirection::Count] = {
    GL_CW,
    GL_CCW
};
static_assert((sizeof(MapCounterDirection) / sizeof(GLenum)) == static_cast<size_t>(CounterDirection::Count), "Mapping is broken!");

static GLenum MapBlendFactor[BlendFactor::Count] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_ONE_MINUS_DST_COLOR
};
static_assert((sizeof(MapBlendFactor) / sizeof(GLenum)) == static_cast<size_t>(BlendFactor::Count), "Mapping is broken!");

static GLenum MapBlendOp[BlendOp::Count] = {
    GL_ADD,
    GL_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX
};
static_assert((sizeof(MapBlendOp) / sizeof(GLenum)) == static_cast<size_t>(BlendOp::Count), "Mapping is broken!");

static GLboolean MapDepthWriteMask[DepthWriteMask::Count] = {
    GL_FALSE,
    GL_TRUE
};
static_assert((sizeof(MapBlendOp) / sizeof(GLenum)) == static_cast<size_t>(BlendOp::Count), "Mapping is broken!");

static GLenum MapComparisonFunc[ComparisonFunc::Count] = {
    GL_ALWAYS,
    GL_NEVER,
    GL_LESS,
    GL_LEQUAL,
    GL_GREATER,
    GL_GEQUAL,
    GL_EQUAL,
    GL_NOTEQUAL
};
static_assert((sizeof(MapComparisonFunc) / sizeof(GLenum)) == static_cast<size_t>(ComparisonFunc::Count), "Mapping is broken!");

static GLenum MapStencilOp[StencilOp::Count] = {
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR,
    GL_DECR
};
static_assert((sizeof(MapStencilOp) / sizeof(GLenum)) == static_cast<size_t>(StencilOp::Count), "Mapping is broken!");

//-------------------------------------------------------------------------------------------------

struct GLSamplerStateImpl final
{
    GLuint samplerID = 0;

    inline GLSamplerStateImpl()  { glGenSamplers(1, &samplerID); }
    inline ~GLSamplerStateImpl() { glDeleteSamplers(1, &samplerID); }
};

struct GLShaderImpl final
{
    GLuint shaderID = 0;

    inline GLShaderImpl(GLuint shader) : shaderID(shader) {}
    inline ~GLShaderImpl() { glDeleteShader(shaderID); }
};

struct GLBufferImpl final
{
    GLuint bufferID = 0;

    bool isImmutable  = false;
    bool isStructured = false;
    bool isConstant   = false;

    size_t dataSize   = 0;
    size_t dataStride = 0;

    inline GLBufferImpl()  { glGenBuffers(1, &bufferID); }
    inline ~GLBufferImpl() { glDeleteBuffers(1, &bufferID); }
};

struct GLVertexFormatImpl final // VF is a VAO with bound attribs
{
    GLuint vaoID = 0;

    inline GLVertexFormatImpl()  { glGenVertexArrays(1, &vaoID); }
    inline ~GLVertexFormatImpl() { glDeleteVertexArrays(1, &vaoID); }
};

//-------------------------------------------------------------------------------------------------

void GL_setPipelineState(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        PipelineStateDescriptor* state = static_cast<PipelineStateDescriptor*>(handle.value);

        // rasterizer state
        const RasterizerState& rs = state->rasterizerState;
        glPolygonMode(GL_FRONT_AND_BACK, MapFillMode[static_cast<uint32_t>(rs.fillMode)]);
        glEnable(GL_CULL_FACE);
        glCullFace(MapCullMode[static_cast<uint32_t>(rs.cullMode)]);
        glFrontFace(MapCounterDirection[static_cast<uint32_t>(rs.counterDirection)]);

        // blend state
        const BlendState& bs = state->blendState;

        if (bs.blendDesc.blendEnabled || bs.separateBlendEnabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        if (bs.blendDesc.blendEnabled) {
            uint8_t colorMask = static_cast<uint8_t>(bs.blendDesc.writeMask);
            glColorMask(
                colorMask & static_cast<uint8_t>(ColorWriteMask::Red),
                colorMask & static_cast<uint8_t>(ColorWriteMask::Green),
                colorMask & static_cast<uint8_t>(ColorWriteMask::Blue),
                colorMask & static_cast<uint8_t>(ColorWriteMask::Alpha)
            );
            glBlendFuncSeparate(
                MapBlendFactor[static_cast<uint32_t>(bs.blendDesc.srcBlend)],
                MapBlendFactor[static_cast<uint32_t>(bs.blendDesc.dstBlend)],
                MapBlendFactor[static_cast<uint32_t>(bs.blendDesc.srcBlendAlpha)],
                MapBlendFactor[static_cast<uint32_t>(bs.blendDesc.dstBlendAlpha)]
            );
            glBlendEquationSeparate(
                MapBlendOp[static_cast<uint32_t>(bs.blendDesc.blendOp)],
                MapBlendOp[static_cast<uint32_t>(bs.blendDesc.blendOpAlpha)]
            );
        }

        if (bs.separateBlendEnabled) {
            for (size_t i = 0; i < RenderTargetSlot::Count; ++i) {
                uint8_t colorMask = static_cast<uint8_t>(bs.renderTargetBlendDesc[i].writeMask);
                glColorMaski(
                    i,
                    colorMask & static_cast<uint8_t>(ColorWriteMask::Red),
                    colorMask & static_cast<uint8_t>(ColorWriteMask::Green),
                    colorMask & static_cast<uint8_t>(ColorWriteMask::Blue),
                    colorMask & static_cast<uint8_t>(ColorWriteMask::Alpha)
                );
                glBlendFuncSeparatei(
                    i,
                    MapBlendFactor[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].srcBlend)],
                    MapBlendFactor[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].dstBlend)],
                    MapBlendFactor[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].srcBlendAlpha)],
                    MapBlendFactor[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].dstBlendAlpha)]
                );
                glBlendEquationSeparatei(
                    i,
                    MapBlendOp[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].blendOp)],
                    MapBlendOp[static_cast<uint32_t>(bs.renderTargetBlendDesc[i].blendOpAlpha)]
                );
            }
        }

        if (bs.alphaToCoverageEnabled)
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        else
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

        // depth stencil state
        const DepthStencilState& ds = state->depthStencilState;

        if (ds.depthEnabled) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(MapComparisonFunc[static_cast<uint32_t>(ds.depthFunc)]);
            glDepthMask(MapDepthWriteMask[static_cast<uint32_t>(ds.writeMask)]);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        if (ds.stencilEnabled) {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(ds.stencilWriteMask);
            glStencilFuncSeparate(
                GL_FRONT,
                MapComparisonFunc[static_cast<uint32_t>(ds.frontFaceStencilDesc.stencilFunc)],
                ds.stencilRef,
                0xFF
            );
            glStencilFuncSeparate(
                GL_BACK,
                MapComparisonFunc[static_cast<uint32_t>(ds.backFaceStencilDesc.stencilFunc)],
                ds.stencilRef,
                0xFF
            );
            glStencilOpSeparate(
                GL_FRONT,
                MapStencilOp[static_cast<uint32_t>(ds.frontFaceStencilDesc.failOp)],
                MapStencilOp[static_cast<uint32_t>(ds.frontFaceStencilDesc.depthFailOp)],
                MapStencilOp[static_cast<uint32_t>(ds.frontFaceStencilDesc.passOp)]
            );
            glStencilOpSeparate(
                GL_BACK,
                MapStencilOp[static_cast<uint32_t>(ds.backFaceStencilDesc.failOp)],
                MapStencilOp[static_cast<uint32_t>(ds.backFaceStencilDesc.depthFailOp)],
                MapStencilOp[static_cast<uint32_t>(ds.backFaceStencilDesc.passOp)]
            );
        } else {
            glDisable(GL_STENCIL_TEST);
        }
    }
}

//-------------------------------------------------------------------------------------------------
bool compileShader_GL(
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
)
{
    return false;
}

VertexFormatHandle createVertexFormat_GL(
    VertexElementDescriptor* elements,
    size_t                   size,
    void* shaderBytecode, size_t shaderBytecodeSize,
    ErrorReportFunc          errorReport
)
{
    GLVertexFormatImpl* impl = new GLVertexFormatImpl;
    // TODO: implement
    return VertexFormatHandle(impl);
}

void releaseVertexFormat_GL(VertexFormatHandle handle)
{
    if (handle != VertexFormatHandle::invalidHandle()) {
        GLVertexFormatImpl* impl = static_cast<GLVertexFormatImpl*>(handle.value);
        delete impl;
    }
}

PipelineStateHandle createPipelineState_GL(const PipelineStateDescriptor& desc)
{
    PipelineStateDescriptor* ret = new PipelineStateDescriptor;
    std::memcpy(ret, &desc, sizeof(PipelineStateDescriptor));
    return PipelineStateHandle(ret);
}

void releasePipelineState_GL(PipelineStateHandle handle)
{
    if (handle != PipelineStateHandle::invalidHandle()) {
        PipelineStateDescriptor* desc = static_cast<PipelineStateDescriptor*>(handle.value);
        delete desc;
    }
}

BufferHandle createBuffer_GL(uint32_t flags, const void* mem, size_t size, size_t stride)
{
    GLBufferImpl* impl = new GLBufferImpl;

    enum class AccessFrequency { Static, Dynamic };
    enum class AccessNature    { Draw,   Read, Copy };

    AccessFrequency freq   = AccessFrequency::Static;
    AccessNature    nature = AccessNature::Draw;

    bool isImmutable  = true;
    bool isStructured = false;

    if ((flags & BufferFlags::VertexBuffer) ||
        (flags & BufferFlags::IndexBuffer)  ||
        (flags & BufferFlags::StreamOutput) ||
        (flags & BufferFlags::StructuredBuffer)) {
        nature = AccessNature::Draw;
    }

    if ((flags & BufferFlags::CPUWrite) ||
        (flags & BufferFlags::GPUWrite)) {
        freq   = AccessFrequency::Dynamic;
        nature = AccessNature::Copy;
        isImmutable = false;
    }

    if ((flags & BufferFlags::CPURead)) {
        freq   = AccessFrequency::Dynamic;
        nature = AccessNature::Read;
    }

    GLenum glUsage = 0;
    switch (freq) {
    case AccessFrequency::Static: {
        switch (nature) {
        case AccessNature::Draw: { glUsage = GL_STATIC_DRAW; } break;
        case AccessNature::Read: { glUsage = GL_STATIC_READ; } break;
        case AccessNature::Copy: { glUsage = GL_STATIC_COPY; } break;
        };
    } break;
    case AccessFrequency::Dynamic: {
        switch (nature) {
        case AccessNature::Draw: { glUsage = GL_DYNAMIC_DRAW; } break;
        case AccessNature::Read: { glUsage = GL_DYNAMIC_READ; } break;
        case AccessNature::Copy: { glUsage = GL_DYNAMIC_COPY; } break;
        };
    } break;
    };

    impl->isImmutable  = isImmutable; // TODO: proper immutable buffers with glBufferStorage
    impl->isStructured = (flags & BufferFlags::StructuredBuffer) != 0;
    impl->dataSize     = size;
    impl->dataStride   = stride;

    glNamedBufferDataEXT(impl->bufferID, size, mem, glUsage);

    return BufferHandle(impl);
}

void releaseBuffer_GL(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        GLBufferImpl* impl = static_cast<GLBufferImpl*>(handle.value);
        delete impl;
    }
}

void* mapBuffer_GL(BufferHandle handle, MapType type)
{
    if (handle != BufferHandle::invalidHandle()) {
        GLBufferImpl* impl = static_cast<GLBufferImpl*>(handle.value);

        return glMapNamedBufferEXT(impl->bufferID, MapMapType[static_cast<uint64_t>(type)]);
    }

    return nullptr;
}

void unmapBuffer_GL(BufferHandle handle)
{
    if (handle != BufferHandle::invalidHandle()) {
        GLBufferImpl* impl = static_cast<GLBufferImpl*>(handle.value);

        glUnmapNamedBufferEXT(impl->bufferID);
    }
}

ConstantBufferHandle createConstantBuffer_GL(const void* mem, size_t size)
{
    GLBufferImpl* impl = new GLBufferImpl;

    impl->isImmutable  = false;
    impl->isStructured = false;
    impl->isConstant   = false;
    impl->dataSize     = size;

    glNamedBufferDataEXT(impl->bufferID, size, mem, GL_DYNAMIC_DRAW);

    return ConstantBufferHandle(impl);
}

void updateConstantBuffer_GL(ConstantBufferHandle handle, const void* mem)
{
    if (handle != ConstantBufferHandle::invalidHandle()) {
        GLBufferImpl* impl = static_cast<GLBufferImpl*>(handle.value);

        glNamedBufferSubDataEXT(impl->bufferID, 0, impl->dataSize, mem);
    }
}

void releaseConstantBuffer_GL(ConstantBufferHandle handle)
{
    if (handle != ConstantBufferHandle::invalidHandle()) {
        GLBufferImpl* impl = static_cast<GLBufferImpl*>(handle.value);
        delete impl;
    }
}

SamplerStateHandle createSamplerState_GL(const SamplerStateDescriptor& desc)
{
    GLSamplerStateImpl* impl = new GLSamplerStateImpl;

    // filter
    const GLTexFilter& filterImpl = MapTextureFilter[static_cast<uint64_t>(desc.filter)];

    // color
    float fcolor[4];
    fcolor[0] = static_cast<float>((desc.borderColor >> 0)  & 0xFF) / 255.0F;
    fcolor[1] = static_cast<float>((desc.borderColor >> 8)  & 0xFF) / 255.0F;
    fcolor[2] = static_cast<float>((desc.borderColor >> 16) & 0xFF) / 255.0F;
    fcolor[3] = static_cast<float>((desc.borderColor >> 24) & 0xFF) / 255.0F;

    // init sampler
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_MIN_FILTER, filterImpl.min);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_MAG_FILTER, filterImpl.mag);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_WRAP_S, MapAddressMode[static_cast<uint64_t>(desc.addressU)]);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_WRAP_T, MapAddressMode[static_cast<uint64_t>(desc.addressV)]);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_WRAP_R, MapAddressMode[static_cast<uint64_t>(desc.addressW)]);
    glSamplerParameterf (impl->samplerID, GL_TEXTURE_LOD_BIAS, desc.lodBias);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.maxAnisotropy);
    glSamplerParameteri (impl->samplerID, GL_TEXTURE_COMPARE_MODE, MapComparisonFunc[static_cast<uint64_t>(desc.comparisonFunc)]);
    glSamplerParameterfv(impl->samplerID, GL_TEXTURE_BORDER_COLOR, fcolor);
    glSamplerParameterf (impl->samplerID, GL_TEXTURE_MIN_LOD, desc.minLod);
    glSamplerParameterf (impl->samplerID, GL_TEXTURE_MAX_LOD, desc.maxLod);

    return SamplerStateHandle(impl);
}

void releaseSamplerState_GL(SamplerStateHandle handle)
{
    if (handle != SamplerStateHandle::invalidHandle()) {
        GLSamplerStateImpl* impl = static_cast<GLSamplerStateImpl*>(handle.value);
        delete impl;
    }
}

}

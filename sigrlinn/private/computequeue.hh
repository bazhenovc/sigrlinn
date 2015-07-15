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

#include "../sigrlinn.hh"

namespace sgfx
{
namespace internal
{

struct ComputeQueue final
{
    enum
    {
        kMaxSamplerStates = 8
    };

    enum
    {
        kMaxConstantBuffers     = 8,
        kMaxShaderResources     = 128,
        kMaxShaderResourcesRW   = 8
    };

    SamplerStateHandle      samplerStates[kMaxSamplerStates];
    ConstantBufferHandle    constantBuffers[kMaxConstantBuffers];
    ShaderResource          shaderResources[kMaxShaderResources];
    ShaderResource          shaderResourcesRW[kMaxShaderResourcesRW];

    ComputeShaderHandle     shader;

    inline void setConstantBuffer(uint32_t idx, ConstantBufferHandle resource)
    {
        constantBuffers[idx] = resource;
    }

    inline void setResource(uint32_t idx, BufferHandle resource)
    {
        shaderResources[idx] = ShaderResource(false, resource.value);
    }

    inline void setResource(uint32_t idx, TextureHandle resource)
    {
        shaderResources[idx] = ShaderResource(true, resource.value);
    }

    inline void setResourceRW(uint32_t idx, BufferHandle resource)
    {
        shaderResourcesRW[idx] = ShaderResource(false, resource.value);
    }
};

}
}

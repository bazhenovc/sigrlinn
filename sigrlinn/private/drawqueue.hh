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
#include "array.hh"

#include <memory>

namespace sgfx
{
namespace internal
{

// emulated draw queues for pre-DX12 APIs (DX11 and GL4)
struct DrawCall final
{
    enum Type : uint32_t
    {
        Draw                 = 0,
        DrawIndexed          = 1,
        DrawInstanced        = 2,
        DrawIndexedInstanced = 3
    };

    enum
    {
        kMaxConstantBuffers = 8,
        kMaxShaderResources = 128
    };

    ConstantBufferHandle constantBuffers[kMaxConstantBuffers];
    BufferHandle         shaderResources[kMaxShaderResources];
    BufferHandle         rwShaderResources[kMaxShaderResources];

    BufferHandle      vertexBuffer;
    BufferHandle      indexBuffer;
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

    DrawQueue(PipelineStateHandle _state) : state(_state) {}

    inline PipelineStateHandle  getState() const     { return state; }
    inline const DrawCallArray& getDrawCalls() const { return drawCalls; }

    inline void clear()
    {
        drawCalls.Clear();
    }

    inline void setPrimitiveTopology(PrimitiveTopology topology)     { currentDrawCall.primitiveTopology = topology; }
    inline void setVertexBuffer(BufferHandle handle)                 { currentDrawCall.vertexBuffer = handle; }
    inline void setIndexBuffer(BufferHandle handle)                  { currentDrawCall.indexBuffer  = handle; }

    inline void setConstantBuffer(uint32_t idx, ConstantBufferHandle resource)
    {
        currentDrawCall.constantBuffers[idx] = resource;
    }

    inline void setResource(uint32_t idx, BufferHandle resource)
    {
        currentDrawCall.shaderResources[idx] = resource;
    }

    inline void setResource(uint32_t idx, TextureHandle handle)
    {
        currentDrawCall.shaderResources[idx] = BufferHandle(handle.value);
    }

    inline void setResourceRW(uint32_t idx, BufferHandle resource)
    {
        currentDrawCall.rwShaderResources[idx] = resource;
    }

    inline void draw(uint32_t count, uint32_t startVertex)
    {
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = 0;
        currentDrawCall.type        = DrawCall::Draw;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    inline void drawIndexed(uint32_t count, uint32_t startIndex, uint32_t startVertex)
    {
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = startIndex;
        currentDrawCall.type        = DrawCall::DrawIndexed;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    inline void drawInstanced(uint32_t instanceCount, uint32_t count, uint32_t startVertex)
    {
        currentDrawCall.instanceCount = instanceCount;
        currentDrawCall.count         = count;
        currentDrawCall.startVertex   = startVertex;
        currentDrawCall.startIndex    = 0;
        currentDrawCall.type          = DrawCall::DrawInstanced;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    inline void drawIndexedInstanced(uint32_t instanceCount, uint32_t count, uint32_t startIndex, uint32_t startVertex)
    {
        currentDrawCall.instanceCount = instanceCount;
        currentDrawCall.count         = count;
        currentDrawCall.startVertex   = startVertex;
        currentDrawCall.startIndex    = startIndex;
        currentDrawCall.type          = DrawCall::DrawIndexedInstanced;
        drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }
};

}
}

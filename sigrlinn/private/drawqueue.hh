
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
        Draw        = 0,
        DrawIndexed = 1
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
};

}
}


#pragma once

#include "../sigrlinn.hh"
#include "array.hh"

#include <memory>

namespace sgfx
{
namespace internal
{

// emulated draw queues for pre-DX12 APIs (DX11 and GL4)
struct DrawCall
{
    enum Type
    {
        Draw,
        DrawIndexed
    };

    enum
    {
        MaxConstantBuffers = 8,
        ConstantBufferSize = 2048 // TODO: remove hardcode
    };

    enum
    {
        MaxTextures = 8
    };

    uint8_t constantBufferData[ConstantBufferSize];
    uint8_t usedConstantBuffers; // mask, each set bit identifies a used constant buffer

    size_t stride;
    size_t offset;

    BufferHandle          vertexBuffer;
    BufferHandle          indexBuffer;
    uint32_t              textures[MaxTextures];
    PrimitiveTopology     primitiveTopology;

    uint32_t count;
    uint32_t startVertex;
    uint32_t startIndex;
    Type     type;
};

class DrawQueue
{
private:
    typedef DynamicArray<DrawCall, 4096, 4096> DrawCallArray;

    PipelineStateHandle state;
    DrawCall            currentDrawCall;
    DrawCallArray       drawCalls;

public:

    DrawQueue(PipelineStateHandle _state) : state(_state) {}

    inline PipelineStateHandle  getState() const     { return state; }
    inline const DrawCallArray& getDrawCalls() const { return drawCalls; }

    inline void clear() { drawCalls.Clear(); }

    inline void setPrimitiveTopology(PrimitiveTopology topology)     { currentDrawCall.primitiveTopology = topology; }
    inline void setVertexBuffer(BufferHandle handle)                 { currentDrawCall.vertexBuffer = handle; }
    inline void setIndexBuffer(BufferHandle handle)                  { currentDrawCall.indexBuffer  = handle; }

    inline void setConstants(uint32_t idx, void* constantsData, size_t constantsSize)
    {
        std::memcpy(
            &currentDrawCall.constantBufferData[idx * DrawCall::ConstantBufferSize],
            constantsData,
            constantsSize
        );
        currentDrawCall.usedConstantBuffers |= (1 << idx);
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

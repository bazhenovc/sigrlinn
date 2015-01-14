
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
    enum Type
    {
        Draw,
        DrawIndexed
    };

    enum
    {
        MaxConstantBuffers = 4,
        ConstantBufferSize = 2048 // TODO: remove hardcode
    };

    enum
    {
        MaxTextures = 8
    };

    uint8_t constantBufferData[MaxConstantBuffers * ConstantBufferSize];
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

    inline bool isValid() const { return vertexBuffer.value != nullptr && indexBuffer.value != nullptr; }
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

    struct BufferPair
    {
        BufferHandle vb;
        BufferHandle ib;

        inline friend bool operator==(const BufferPair& b0, const BufferPair& b1)
        {
            return b0.vb == b1.vb && b0.ib == b1.ib;
        }
    };
    typedef DynamicArray<BufferPair, 128, 512> BufferPairArray;
    BufferPairArray bufferPairs;

    DrawQueue(PipelineStateHandle _state) : state(_state) {}

    inline PipelineStateHandle  getState() const     { return state; }
    inline const DrawCallArray& getDrawCalls() const { return drawCalls; }

    inline void clear()
    {
        drawCalls.Clear();
        bufferPairs.Clear();
    }

    inline void allocateBufferPair()
    {
        BufferPair pair;
        pair.vb = currentDrawCall.vertexBuffer;
        pair.ib = currentDrawCall.indexBuffer;

        if (bufferPairs.Find(pair) == -1)
            bufferPairs.Add(pair);
    }

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
        allocateBufferPair();
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = 0;
        currentDrawCall.type        = DrawCall::Draw;
        if (currentDrawCall.isValid()) drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }

    inline void drawIndexed(uint32_t count, uint32_t startIndex, uint32_t startVertex)
    {
        allocateBufferPair();
        currentDrawCall.count       = count;
        currentDrawCall.startVertex = startVertex;
        currentDrawCall.startIndex  = startIndex;
        currentDrawCall.type        = DrawCall::DrawIndexed;
        if (currentDrawCall.isValid()) drawCalls.Add(currentDrawCall);
        std::memset(&currentDrawCall, 0, sizeof(currentDrawCall));
    }
};

}
}

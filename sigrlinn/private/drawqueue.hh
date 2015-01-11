
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

	size_t stride;
	size_t offset;

	BufferHandle          vertexBuffer;
	BufferHandle          indexBuffer;
	TransientBufferHandle constantBuffers[8];
	TextureHandle         textures[16];
	PrimitiveTopology     primitiveTopology;

	uint32_t count;
	uint32_t startVertex;
	uint32_t startIndex;
	Type     type;
};

class DrawQueue
{
private:
	PipelineStateHandle    state;
	DrawCall               currentDrawCall;
	DynamicArray<DrawCall> drawCalls;

public:

	DrawQueue(PipelineStateHandle _state) : state(_state) {}

	inline PipelineStateHandle           getState() const     { return state; }
	inline const DynamicArray<DrawCall>& getDrawCalls() const { return drawCalls; }

	inline void clear() { drawCalls.Clear(); }

	inline void setPrimitiveTopology(PrimitiveTopology topology)     { currentDrawCall.primitiveTopology = topology; }
	inline void setVertexBuffer(BufferHandle handle)                 { currentDrawCall.vertexBuffer = handle; }
	inline void setIndexBuffer(BufferHandle handle)                  { currentDrawCall.indexBuffer  = handle; }
	inline void setConstantBuffer(uint32_t idx, BufferHandle handle) { currentDrawCall.constantBuffers[idx] = handle; }

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


#include "sigrlinn.hh"

#include "GL/glew.h"
#include <memory>

#pragma comment(lib, "opengl32.lib")

namespace sgfx
{

static GLenum MapFillMode[FillMode::Count] = {
	GL_FILL,
	GL_LINE
};

static GLenum MapCullMode[CullMode::Count] = {
	GL_BACK,
	GL_FRONT
};

static GLenum MapCounterDirection[CounterDirection::Count] = {
	GL_CW,
	GL_CCW
};

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

static GLenum MapBlendOp[BlendOp::Count] = {
	GL_ADD,
	GL_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};

static GLenum MapRenderTargetSlot[RenderTargetSlot::Count] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
};

static GLboolean MapDepthWriteMask[DepthWriteMask::Count] = {
	GL_FALSE,
	GL_TRUE
};

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

static GLenum MapStencilOp[StencilOp::Count] = {
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INCR,
	GL_DECR
};

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

void setPipelineState_GL(PipelineStateHandle handle)
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

}

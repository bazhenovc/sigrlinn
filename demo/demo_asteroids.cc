
#include "common/app.hh"

#include <memory>

#include <DirectXMath.h>
using namespace DirectX;

class AsteroidsApplication : public Application
{
public:

    struct CommonVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 texcoord0;
        XMFLOAT2 texcoord1;
        XMFLOAT3 normal;

        CommonVertex() {}
        CommonVertex(const XMFLOAT3& pos, const XMFLOAT2& tex0, const XMFLOAT2& tex1, const XMFLOAT3& n)
            : position(pos), texcoord0(tex0), texcoord1(tex1), normal(n)
        {}
    };

    struct ConstantBuffer
    {
        uint32_t sgfxInternalData[4]; // first 4 ints are ALWAYS reserved for sgfx internal data
                                      // at this time: VB+IB+DrawCallType+0

        XMMATRIX mWorld;
        XMMATRIX mView;
        XMMATRIX mProjection;
    };

    enum
    {
        kNumMeshes = 16384
    };

    sgfx::VertexShaderHandle  vsHandle;
    sgfx::PixelShaderHandle   psHandle;
    sgfx::SurfaceShaderHandle ssHandle;

    sgfx::BufferHandle        cubeVertexBuffers[kNumMeshes];
    sgfx::BufferHandle        cubeIndexBuffers[kNumMeshes];

    sgfx::PipelineStateHandle pipelineState;
    sgfx::DrawQueueHandle     drawQueue;

    XMMATRIX                  g_World;
    XMMATRIX                  g_View;
    XMMATRIX                  g_Projection;

public:

    void* operator new(size_t size)
    {
        return _mm_malloc(size, 32);
    }

    void operator delete(void* ptr)
    {
        _mm_free(ptr);
    }

    virtual void loadSampleData() override
    {
        // Initialize the world matrix
        g_World = XMMatrixIdentity();

        // Initialize the view matrix
        XMVECTOR Eye = XMVectorSet( -10.0f, 10.0f, -10.0f, 0.0f );
        XMVECTOR At  = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
        XMVECTOR Up  = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
        g_View = XMMatrixLookAtLH( Eye, At, Up );

        // Initialize the projection matrix
        g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );

        // setup Sigrlinn
        sgfx::initD3D11(g_pd3dDevice, g_pImmediateContext, g_pSwapChain);

        CommonVertex cubeVertices[] = {
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3(0.000000, 1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3(-1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, -1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, -1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, -1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, -1.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, 1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, 1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, 1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, 1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, -1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, -1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, -1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 0.000000, 0.000000, -1.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( -1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( -1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( -1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( -1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( -1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, 1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 0.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, 1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 1.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 1.000000, 0.000000, 0.000000 )  // Normal
        ),
        CommonVertex(
            XMFLOAT3( 1.000000, -1.000000, -1.000000 ), // Vertex
            XMFLOAT2( 1.000000, 0.000000 ), // Texcoord 0
            XMFLOAT2( 0, 0 ), // Texcoord 1
            XMFLOAT3( 1.000000, 0.000000, 0.000000 )  // Normal
        )
        };
        size_t verticesSize = sizeof(cubeVertices) / sizeof(CommonVertex);

        static uint32_t cubeIndices[] = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20 };
        size_t indicesSize = sizeof(cubeIndices) / sizeof(uint32_t);

        for (int i = 0; i < kNumMeshes; ++i) { // create a lot of 'unique' meshes
            cubeVertexBuffers[i] = sgfx::createBuffer(sgfx::BufferType::Vertex, cubeVertices, sizeof(CommonVertex) * verticesSize, sizeof(CommonVertex));
            cubeIndexBuffers[i]  = sgfx::createBuffer(sgfx::BufferType::Index, cubeIndices, sizeof(uint32_t) * indicesSize, sizeof(uint32_t));
        }

        vsHandle = loadVS("shaders/sample0.hlsl");
        psHandle = loadPS("shaders/sample0.hlsl");

        if (vsHandle != sgfx::VertexShaderHandle::invalidHandle() && psHandle != sgfx::PixelShaderHandle::invalidHandle()) {
            ssHandle = sgfx::linkSurfaceShader(
                vsHandle,
                sgfx::HullShaderHandle::invalidHandle(),
                sgfx::DomainShaderHandle::invalidHandle(),
                sgfx::GeometryShaderHandle::invalidHandle(),
                psHandle
            );
        }

        if (ssHandle != sgfx::SurfaceShaderHandle::invalidHandle()) {
            sgfx::PipelineStateDescriptor desc;

            desc.rasterizerState.fillMode                           = sgfx::FillMode::Solid;
            desc.rasterizerState.cullMode                           = sgfx::CullMode::Back;
            desc.rasterizerState.counterDirection                   = sgfx::CounterDirection::CCW;

            desc.blendState.blendDesc.blendEnabled                  = false;
            desc.blendState.blendDesc.writeMask                     = sgfx::ColorWriteMask::All; // not implemented
            desc.blendState.blendDesc.srcBlend                      = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlend                      = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOp                       = sgfx::BlendOp::Add;
            desc.blendState.blendDesc.srcBlendAlpha                 = sgfx::BlendFactor::One;
            desc.blendState.blendDesc.dstBlendAlpha                 = sgfx::BlendFactor::Zero;
            desc.blendState.blendDesc.blendOpAlpha                  = sgfx::BlendOp::Add;

            desc.depthStencilState.depthEnabled                     = true;
            desc.depthStencilState.writeMask                        = sgfx::DepthWriteMask::All;
            desc.depthStencilState.depthFunc                        = sgfx::DepthFunc::Less;

            desc.depthStencilState.stencilEnabled                   = false;
            desc.depthStencilState.stencilRef                       = 0;
            desc.depthStencilState.stencilReadMask                  = 0;
            desc.depthStencilState.stencilWriteMask                 = 0;

            desc.depthStencilState.frontFaceStencilDesc.stencilFunc = sgfx::StencilFunc::Always;
            desc.depthStencilState.frontFaceStencilDesc.failOp      = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.depthFailOp = sgfx::StencilOp::Keep;
            desc.depthStencilState.frontFaceStencilDesc.passOp      = sgfx::StencilOp::Keep;

            desc.depthStencilState.backFaceStencilDesc.stencilFunc  = sgfx::StencilFunc::Always;
            desc.depthStencilState.backFaceStencilDesc.failOp       = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.depthFailOp  = sgfx::StencilOp::Keep;
            desc.depthStencilState.backFaceStencilDesc.passOp       = sgfx::StencilOp::Keep;

            desc.shader = ssHandle;

            pipelineState = sgfx::createPipelineState(desc);
            if (pipelineState != sgfx::PipelineStateHandle::invalidHandle()) {
                drawQueue = sgfx::createDrawQueue(pipelineState);
            } else {
                OutputDebugString("Failed to create pipeline state!");
            }
        }
    }

    virtual void releaseSampleData() override
    {
        OutputDebugString("Cleanup\n");
        for (int i = 0; i < kNumMeshes; ++i) sgfx::releaseBuffer(cubeVertexBuffers[i]);
        for (int i = 0; i < kNumMeshes; ++i) sgfx::releaseBuffer(cubeIndexBuffers[i]);
        sgfx::releaseVertexShader(vsHandle);
        sgfx::releasePixelShader(psHandle);
        sgfx::releaseSurfaceShader(ssHandle);
        sgfx::releasePipelineState(pipelineState);
        sgfx::releaseDrawQueue(drawQueue);

        sgfx::shutdown();
    }

    virtual void renderSample() override
    {
        // Update our time
        static float t  = 0.0F;
        static DWORD dwTimeStart = 0;

        DWORD dwTimeCur = GetTickCount();
        t = (dwTimeCur - dwTimeStart) / 1000.0f;
        dwTimeStart = dwTimeCur;

        static bool dtReport = false;
        if (GetAsyncKeyState(VK_F1)) dtReport = !dtReport;

        if (dtReport) {
            char buf[64];
            sprintf_s(buf, "DT: %f\n", t);
            OutputDebugString(buf);
        }

        g_World *= XMMatrixRotationY(t);

        ConstantBuffer constants;
        std::memset(constants.sgfxInternalData, 0, sizeof(constants.sgfxInternalData));
        constants.mWorld      = XMMatrixTranspose(g_World);
        constants.mView       = XMMatrixTranspose(g_View);
        constants.mProjection = XMMatrixTranspose(g_Projection);

        // actually draw some stuff
        size_t counter = 0;
        for (int i = -64; i < 64; ++i) {
            for (int k = -64; k < 64; ++k) {
                constants.mWorld = XMMatrixTranspose(
                    g_World * XMMatrixTranslation(
                        static_cast<float>(i) * 3.0F,
                        0,
                        static_cast<float>(k) * 3.0F
                    )
                );

                // set different textures
                //for (uint32_t j = 0; j < 8; ++j) {
                //    sgfx::setTexture(drawQueue, i, (i + k) + 128 + j); // simulate unique texture
                //}

                sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
                sgfx::setConstants(drawQueue, 0, &constants, sizeof(constants));
                sgfx::setVertexBuffer(drawQueue, cubeVertexBuffers[counter]);
                sgfx::setIndexBuffer(drawQueue, cubeIndexBuffers[counter]);
                sgfx::drawIndexed(drawQueue, 36, 0, 0);

                counter++;
                if (counter == kNumMeshes) counter = 0;
            }
        }
        sgfx::submit(drawQueue);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new AsteroidsApplication;
}

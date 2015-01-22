
#include "common/app.hh"

#include <memory>

#include <DirectXMath.h>
using namespace DirectX;

class CubeApplication : public Application
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
        XMMATRIX mWorld;
        XMMATRIX mView;
        XMMATRIX mProjection;
    };

    sgfx::VertexShaderHandle  vsHandle;
    sgfx::PixelShaderHandle   psHandle;
    sgfx::SurfaceShaderHandle ssHandle;

    sgfx::BufferHandle         cubeVertexBuffer;
    sgfx::BufferHandle         cubeIndexBuffer;
    sgfx::ConstantBufferHandle constantBuffer;
    sgfx::VertexFormatHandle   vertexFormat;

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
        // setup Sigrlinn
        sgfx::initD3D11(g_pd3dDevice, g_pImmediateContext, g_pSwapChain);

        // constant buffer
        constantBuffer = sgfx::createConstantBuffer(nullptr, sizeof(ConstantBuffer));

        // mesh data
        sgfx::VertexElementDescriptor vfElements[] = {
            { "POSITION",  0, sgfx::DataFormat::RGB32F, 0, 0,  sgfx::VertexElementType::PerVertex },
            { "TEXCOORDA", 0, sgfx::DataFormat::RG32F,  0, 12, sgfx::VertexElementType::PerVertex },
            { "TEXCOORDB", 0, sgfx::DataFormat::RG32F,  0, 20, sgfx::VertexElementType::PerVertex },
            { "NORMAL",    0, sgfx::DataFormat::RGB32F, 0, 28, sgfx::VertexElementType::PerVertex },
        };
        size_t vfSize = sizeof(vfElements) / sizeof(sgfx::VertexElementDescriptor);

        vertexFormat = loadVF(vfElements, vfSize, "shaders/sample0.hlsl");

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

        cubeVertexBuffer = sgfx::createBuffer(sgfx::BufferFlags::VertexBuffer, cubeVertices, sizeof(CommonVertex) * verticesSize, sizeof(CommonVertex));
        cubeIndexBuffer  = sgfx::createBuffer(sgfx::BufferFlags::IndexBuffer, cubeIndices, sizeof(uint32_t) * indicesSize, sizeof(uint32_t));

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

            desc.shader       = ssHandle;
            desc.vertexFormat = vertexFormat;

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
        sgfx::releaseBuffer(cubeVertexBuffer);
        sgfx::releaseBuffer(cubeIndexBuffer);
        sgfx::releaseConstantBuffer(constantBuffer);
        sgfx::releaseVertexFormat(vertexFormat);
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
        static float t = 0.0f;

        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;
        t = (dwTimeCur - dwTimeStart) / 1000.0f;

        g_World = XMMatrixRotationY(t);

        // Initialize the view matrix
        XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -5.0f, 0.0f );
        XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
        XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
        g_View = XMMatrixLookAtLH( Eye, At, Up );

        // Initialize the projection matrix
        g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );

        ConstantBuffer constants;
        constants.mWorld      = XMMatrixTranspose(g_World);
        constants.mView       = XMMatrixTranspose(g_View);
        constants.mProjection = XMMatrixTranspose(g_Projection);
        sgfx::updateConstantBuffer(constantBuffer, &constants);

        // actually draw some stuff
        sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
        sgfx::setConstantBuffer(drawQueue, 0, constantBuffer);
        sgfx::setVertexBuffer(drawQueue, cubeVertexBuffer);
        sgfx::setIndexBuffer(drawQueue, cubeIndexBuffer);
        sgfx::drawIndexed(drawQueue, 36, 0, 0);

        sgfx::submit(drawQueue);
    }
};

void sampleApplicationMain()
{
    ApplicationInstance = new CubeApplication;
}

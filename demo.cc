//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
using namespace DirectX;

#pragma comment(lib, "d3d11.lib")

#include "sigrlinn/sigrlinn.hh"

#include <fstream>
#include <iostream>

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst             = NULL;
HWND                    g_hWnd              = NULL;
D3D_DRIVER_TYPE         g_driverType        = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel      = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice        = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain        = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11Texture2D*        g_pDepthStencil     = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;

XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;

//--------------------------------------------------------------------------------------
// Sample data
//--------------------------------------------------------------------------------------
void GenericErrorReporter(const char* msg)
{
    OutputDebugString(msg);
}

bool LoadShader(const char* path, sgfx::ShaderCompileTarget target, void*& outData, size_t& outSize)
{
    std::ifstream ifs(path);
    if (ifs.is_open()) {
        ifs.seekg(0, ifs.end);
        size_t size = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0, ifs.beg);

        char* sourceCode = new char[size + 1];
        std::memset(sourceCode, 0, size + 1);
        ifs.read(sourceCode, size);

        return sgfx::compileShader(
            sourceCode,
            size,
            sgfx::ShaderCompileVersion::v5_0,
            target,
            nullptr, 0,
            0,
            GenericErrorReporter,
            outData,
            outSize
        );
    }
    OutputDebugString("Failed to open file");
    return false;
}

struct VertexStageData
{
    sgfx::VertexShaderHandle vs;
    sgfx::VertexFormatHandle vf;
};

VertexStageData loadVS(const char* path, sgfx::VertexElementDescriptor* formatData, size_t formatSize)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (LoadShader(path, sgfx::ShaderCompileTarget::VS, bytecode, bytecodeSize)) {
        sgfx::VertexShaderHandle vs = sgfx::createVertexShader(bytecode, bytecodeSize);
        sgfx::VertexFormatHandle vf = sgfx::createVertexFormat(formatData, formatSize, bytecode, bytecodeSize, GenericErrorReporter);

        OutputDebugString("Vertex shader compiled.\n");

        delete [] bytecode;
        return { vs, vf };
    }

    return VertexStageData();
}

sgfx::PixelShaderHandle loadPS(const char* path)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (LoadShader(path, sgfx::ShaderCompileTarget::PS, bytecode, bytecodeSize)) {
        sgfx::PixelShaderHandle ret = sgfx::createPixelShader(bytecode, bytecodeSize);
        OutputDebugString("Pixel shader compiled.\n");
        delete [] bytecode;
        return ret;
    }

    return sgfx::PixelShaderHandle::invalidHandle();
}

struct SimpleVertex
{
    XMFLOAT4 Pos;
    XMFLOAT4 Color;
};


struct ConstantBuffer
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

VertexStageData             vsData;
sgfx::PixelShaderHandle     psHandle;
sgfx::SurfaceShaderHandle   ssHandle;

sgfx::BufferHandle          cubeVertexBuffers[6];
sgfx::BufferHandle          cubeIndexBuffers[6];

sgfx::PipelineStateHandle   pipelineState;
sgfx::DrawQueueHandle       drawQueue;

void LoadSampleData()
{
    sgfx::VertexElementDescriptor vertexFormat[] = {
        { "INSTANCE_ID", 0, sgfx::DataFormat::R32U, 0, 0,  sgfx::VertexElementType::PerVertex }
    };
    size_t vertexFormatSize = sizeof(vertexFormat) / sizeof(sgfx::VertexElementDescriptor);

    SimpleVertex vertices[] =
    {
        { XMFLOAT4( -1.0f, 1.0f, -1.0f, 1.0F ),  XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( 1.0f, 1.0f, -1.0f, 1.0F ),   XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0F ),    XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( -1.0f, 1.0f, 1.0f, 1.0F ),   XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( -1.0f, -1.0f, -1.0f, 1.0F ), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( 1.0f, -1.0f, -1.0f, 1.0F ),  XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( 1.0f, -1.0f, 1.0f, 1.0F ),   XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT4( -1.0f, -1.0f, 1.0f, 1.0F ),  XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
    };

    uint32_t indices[] = { 3,1,0, 2,1,3, 0,5,4, 1,5,0, 3,4,7, 0,4,3, 1,6,5, 2,6,1, 2,7,6, 3,7,2, 6,4,5, 7,4,6, };

    XMFLOAT4 colors[] ={ 
        XMFLOAT4( 1.0F, 0.0F, 0.0F, 1.0F ),
        XMFLOAT4( 1.0F, 1.0F, 0.0F, 1.0F ),
        XMFLOAT4( 1.0F, 1.0F, 1.0F, 1.0F ),

        XMFLOAT4( 0.0F, 0.0F, 1.0F, 1.0F ),
        XMFLOAT4( 0.0F, 1.0F, 1.0F, 1.0F ),
        XMFLOAT4( 1.0F, 0.0F, 1.0F, 1.0F ),
    };

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 8; ++j) vertices[j].Color = colors[i];
        cubeVertexBuffers[i] = sgfx::createBuffer(vertices, sizeof(SimpleVertex) * 8, sizeof(SimpleVertex));
        cubeIndexBuffers[i]  = sgfx::createBuffer(indices, sizeof(uint32_t) * 36, sizeof(uint32_t));
    }

    vsData   = loadVS("shaders/sample0.hlsl", vertexFormat, vertexFormatSize);
    psHandle = loadPS("shaders/sample0.hlsl");

    if (vsData.vs != sgfx::VertexShaderHandle::invalidHandle() && psHandle != sgfx::PixelShaderHandle::invalidHandle()) {
        ssHandle = sgfx::linkSurfaceShader(
            vsData.vs,
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
        desc.vertexFormat = vsData.vf;

        pipelineState = sgfx::createPipelineState(desc);
        if (pipelineState != sgfx::PipelineStateHandle::invalidHandle()) {
            drawQueue = sgfx::createDrawQueue(pipelineState);
        } else {
            OutputDebugString("Failed to create pipeline state!");
        }
    }
}

void ReleaseSampleData()
{
    OutputDebugString("Cleanup\n");
    for (int i = 0; i < 6; ++i) sgfx::releaseBuffer(cubeVertexBuffers[i]);
    for (int i = 0; i < 6; ++i) sgfx::releaseBuffer(cubeIndexBuffers[i]);
    sgfx::releaseVertexFormat(vsData.vf);
    sgfx::releaseVertexShader(vsData.vs);
    sgfx::releasePixelShader(psHandle);
    sgfx::releaseSurfaceShader(ssHandle);
    sgfx::releasePipelineState(pipelineState);
    sgfx::releaseDrawQueue(drawQueue);
}

void RenderSample()
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
            if (counter == 6) counter = 0;
        }
    }
    sgfx::submit(drawQueue);
}

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice())) {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Render();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = "SigrlinnWC";
    wcex.hIconSm       = NULL;
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindowA(
        "SigrlinnWC",
        "Sigrlinn D3D11",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL, NULL,
        hInstance,
        NULL
    );
    if (!g_hWnd)
        return E_FAIL;

    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = width;
    sd.BufferDesc.Height                  = height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = g_hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            g_driverType,
            NULL,
            createDeviceFlags,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &g_featureLevel,
            &g_pImmediateContext
        );
        if (SUCCEEDED(hr))
            break;
    }

#if 0 // D3D11.1
    ID3D11Device1* pDevice1 = nullptr;
    ID3D11DeviceContext1* pImmediateContext1 = nullptr;
    hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&pDevice1));
    if (SUCCEEDED(hr)) {
        pDevice1->Release();
        hr = g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&pImmediateContext1));
        if (SUCCEEDED(hr)) {
            pImmediateContext1->Release();
        }
    }
#endif

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width              = width;
    descDepth.Height             = height;
    descDepth.MipLevels          = 1;
    descDepth.ArraySize          = 1;
    descDepth.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count   = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage              = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags     = 0;
    descDepth.MiscFlags          = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
    if (FAILED(hr))
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format             = descDepth.Format;
    descDSV.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

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
    if (!sgfx::initD3D11(g_pd3dDevice, g_pImmediateContext, g_pSwapChain))
        return S_FALSE;

    LoadSampleData();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
    // Just clear the backbuffer
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; //red,green,blue,alpha
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    RenderSample();

    g_pSwapChain->Present(0, 0);
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    ReleaseSampleData();
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pDepthStencil)     g_pDepthStencil->Release();
    if (g_pDepthStencilView) g_pDepthStencilView->Release();
    if (g_pSwapChain)        g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice)        g_pd3dDevice->Release();
}

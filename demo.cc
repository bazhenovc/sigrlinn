//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
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

VertexStageData LoadVS(const char* path, sgfx::VertexElementDescriptor* formatData, size_t formatSize)
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

sgfx::PixelShaderHandle LoadPS(const char* path)
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
	XMFLOAT3 Pos;
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
sgfx::TransientBufferHandle constantBufferHandle;

sgfx::BufferHandle          cubeVertexBuffer;
sgfx::BufferHandle          cubeIndexBuffer;

sgfx::PipelineStateHandle   pipelineState;
sgfx::DrawQueueHandle       drawQueue;

void LoadSampleData()
{
	sgfx::VertexElementDescriptor vertexFormat[] = {
		{ "POSITION", 0, sgfx::DataFormat::RGB32F, 0, 0,  sgfx::VertexElementType::PerVertex },
		{ "COLOR",    0, sgfx::DataFormat::RGBA32F, 0, 12, sgfx::VertexElementType::PerVertex },
	};
	size_t vertexFormatSize = sizeof(vertexFormat) / sizeof(sgfx::VertexElementDescriptor);

	SimpleVertex vertices[] =
	{
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT4( 0.0f, 1.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f ) },
	};

	uint32_t indices[] = { 3,1,0, 2,1,3, 0,5,4, 1,5,0, 3,4,7, 0,4,3, 1,6,5, 2,6,1, 2,7,6, 3,7,2, 6,4,5, 7,4,6, };

	cubeVertexBuffer = sgfx::createBuffer(sgfx::BufferType::Vertex, vertices, sizeof(SimpleVertex) * 8);
	cubeIndexBuffer  = sgfx::createBuffer(sgfx::BufferType::Index, indices, sizeof(uint32_t) * 36);

	vsData   = LoadVS("shaders/sample0.hlsl", vertexFormat, vertexFormatSize);
	psHandle = LoadPS("shaders/sample0.hlsl");

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

	constantBufferHandle = sgfx::createTransientBuffer(sgfx::TransientBufferType::Constant, nullptr, sizeof(ConstantBuffer));
}

void ReleaseSampleData()
{
	OutputDebugString("Cleanup\n");
	sgfx::releaseBuffer(cubeVertexBuffer);
	sgfx::releaseBuffer(cubeIndexBuffer);
	sgfx::releaseVertexFormat(vsData.vf);
	sgfx::releaseVertexShader(vsData.vs);
	sgfx::releasePixelShader(psHandle);
	sgfx::releaseSurfaceShader(ssHandle);
	sgfx::releaseBuffer(constantBufferHandle);
	sgfx::releasePipelineState(pipelineState);
	sgfx::releaseDrawQueue(drawQueue);
}

void RenderSample()
{
	// Update our time
	static float t = 0.0f;

	static DWORD dwTimeStart = 0;
	DWORD dwTimeCur = GetTickCount();
	if (dwTimeStart == 0)
		dwTimeStart = dwTimeCur;
	t = (dwTimeCur - dwTimeStart) / 1000.0f;

	g_World = XMMatrixRotationY(t);

	ConstantBuffer constants;
	constants.mWorld      = XMMatrixTranspose(g_World);
	constants.mView       = XMMatrixTranspose(g_View);
	constants.mProjection = XMMatrixTranspose(g_Projection);

	sgfx::updateTransientBuffer(constantBufferHandle, &constants, sizeof(constants), 0);

	// actually draw some stuff
	{
		sgfx::setPrimitiveTopology(drawQueue, sgfx::PrimitiveTopology::TriangleList);
		sgfx::setConstantBuffer(drawQueue, 0, constantBufferHandle);
		sgfx::setVertexBuffer(drawQueue, cubeVertexBuffer);
		sgfx::setIndexBuffer(drawQueue, cubeIndexBuffer);
		sgfx::drawIndexed(drawQueue, 36, 0, 0);
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
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

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

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

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
	XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -5.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
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
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

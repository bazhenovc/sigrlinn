
#pragma once

#include <sigrlinn.hh>

#include <stdint.h>

#define APP_WIN32
#ifdef APP_WIN32
#include <windows.h>
#include <d3d11.h>
#endif

class Application
{
protected:

#ifdef APP_WIN32
    HINSTANCE            g_hInst             = NULL;
    HWND                 g_hWnd              = NULL;
    D3D_DRIVER_TYPE      g_driverType        = D3D_DRIVER_TYPE_NULL;
    D3D_FEATURE_LEVEL    g_featureLevel      = D3D_FEATURE_LEVEL_10_0;
    ID3D11Device*        g_pd3dDevice        = NULL;
    ID3D11DeviceContext* g_pImmediateContext = NULL;
    IDXGISwapChain*      g_pSwapChain        = NULL;

    ID3D11RenderTargetView* g_pRenderTargetView = NULL;
    ID3D11Texture2D*        g_pDepthStencil     = NULL;
    ID3D11DepthStencilView* g_pDepthStencilView = NULL;
#endif

    uint32_t width = 1024;
    uint32_t height = 768;

    bool loadShader(const char* path, sgfx::ShaderCompileTarget target, void*& outData, size_t& outSize);
    sgfx::VertexShaderHandle   loadVS(const char* path);
    sgfx::GeometryShaderHandle loadGS(const char* path);
    sgfx::PixelShaderHandle    loadPS(const char* path);
    sgfx::ComputeShaderHandle  loadCS(const char* path);

public:

#ifdef APP_WIN32
    HRESULT initWindow(HINSTANCE hInstance, int nCmdShow);
    HRESULT initDevice();

    void cleanupDevice();
#endif

    Application() {}
    virtual ~Application() {}

    virtual void loadSampleData() = 0;
    virtual void releaseSampleData() = 0;
    virtual void renderSample() = 0;

    void render();
};

extern Application* ApplicationInstance;

void sampleApplicationMain();

/// The MIT License (MIT)
///
/// Copyright (c) 2015 Kirill Bazhenov
/// Copyright (c) 2015 BitBox, Ltd.
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
#pragma once

#include <sigrlinn.hh>

#include <stdint.h>

#define APP_WIN32
#ifdef APP_WIN32
#define NOMINMAX
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
#endif

    uint32_t width = 1024;
    uint32_t height = 768;

    static void genericErrorReporter(const char* msg);

public:
    bool loadShader(const char* path, sgfx::ShaderCompileTarget target, void*& outData, size_t& outSize);
    sgfx::VertexShaderHandle   loadVS(const char* path);
    sgfx::GeometryShaderHandle loadGS(const char* path);
    sgfx::PixelShaderHandle    loadPS(const char* path);
    sgfx::ComputeShaderHandle  loadCS(const char* path);

    sgfx::VertexFormatHandle   loadVF(sgfx::VertexElementDescriptor* vfElements, size_t vfElementsSize, const char* shaderPath);

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

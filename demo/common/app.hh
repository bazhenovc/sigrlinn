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

#include <vector>

#define APP_WIN32

#ifdef APP_WIN32
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#endif

namespace util
{
    inline void releaseHandle(sgfx::VertexShaderHandle obj)   { sgfx::releaseVertexShader(obj); }
    inline void releaseHandle(sgfx::HullShaderHandle obj)     { sgfx::releaseHullShader(obj); }
    inline void releaseHandle(sgfx::DomainShaderHandle obj)   { sgfx::releaseDomainShader(obj); }
    inline void releaseHandle(sgfx::GeometryShaderHandle obj) { sgfx::releaseGeometryShader(obj); }
    inline void releaseHandle(sgfx::PixelShaderHandle obj)    { sgfx::releasePixelShader(obj); }
    inline void releaseHandle(sgfx::SurfaceShaderHandle obj)  { sgfx::releaseSurfaceShader(obj); }
    inline void releaseHandle(sgfx::PipelineStateHandle obj)  { sgfx::releasePipelineState(obj); }
    inline void releaseHandle(sgfx::BufferHandle obj)         { sgfx::releaseBuffer(obj); }
    inline void releaseHandle(sgfx::ConstantBufferHandle obj) { sgfx::releaseConstantBuffer(obj); }
    inline void releaseHandle(sgfx::SamplerStateHandle obj)   { sgfx::releaseSamplerState(obj); }
    inline void releaseHandle(sgfx::TextureHandle obj)        { sgfx::releaseTexture(obj); }
    inline void releaseHandle(sgfx::RenderTargetHandle obj)   { sgfx::releaseRenderTarget(obj); }
    inline void releaseHandle(sgfx::DrawQueueHandle obj)      { sgfx::releaseDrawQueue(obj); }
    inline void releaseHandle(sgfx::ComputeQueueHandle obj)   { sgfx::releaseComputeQueue(obj); }
    inline void releaseHandle(sgfx::ComputeShaderHandle obj)  { sgfx::releaseComputeShader(obj); }
    inline void releaseHandle(sgfx::VertexFormatHandle obj)   { sgfx::releaseVertexFormat(obj); }

    ///////////////////////////////////////////////////////////////////////
    // Graphics object handle
    //
    // RAII style object for sgfx handles

    template <typename T>
    struct GraphicsObjectHandle final
    {
    private:
        T gfxObject = T::invalidHandle();

    public:
        inline          GraphicsObjectHandle()                                      {}
        inline explicit GraphicsObjectHandle(T newObject) : gfxObject(newObject)    {}
        inline          GraphicsObjectHandle(const GraphicsObjectHandle& other) = delete;
        inline          ~GraphicsObjectHandle()                                     { releaseHandle(gfxObject); }

        inline GraphicsObjectHandle& operator=(T newObject)                         { releaseHandle(gfxObject); gfxObject = newObject; return *this; }
        inline GraphicsObjectHandle& operator=(const GraphicsObjectHandle& other) = delete;

        inline operator T()                                                         { return gfxObject; }
        inline operator const T() const                                             { return gfxObject; }

        inline bool valid() const                                                   { return gfxObject.value != 0; }
    };

    // useful typedefs
    typedef GraphicsObjectHandle<sgfx::VertexShaderHandle>   VertexShaderHandle;
    typedef GraphicsObjectHandle<sgfx::HullShaderHandle>     HullShaderHandle;
    typedef GraphicsObjectHandle<sgfx::DomainShaderHandle>   DomainShaderHandle;
    typedef GraphicsObjectHandle<sgfx::GeometryShaderHandle> GeometryShaderHandle;
    typedef GraphicsObjectHandle<sgfx::PixelShaderHandle>    PixelShaderHandle;
    typedef GraphicsObjectHandle<sgfx::SurfaceShaderHandle>  SurfaceShaderHandle;
    typedef GraphicsObjectHandle<sgfx::PipelineStateHandle>  PipelineStateHandle;
    typedef GraphicsObjectHandle<sgfx::BufferHandle>         BufferHandle;
    typedef GraphicsObjectHandle<sgfx::ConstantBufferHandle> ConstantBufferHandle;
    typedef GraphicsObjectHandle<sgfx::SamplerStateHandle>   SamplerStateHandle;
    typedef GraphicsObjectHandle<sgfx::TextureHandle>        TextureHandle;
    typedef GraphicsObjectHandle<sgfx::RenderTargetHandle>   RenderTargetHandle;
    typedef GraphicsObjectHandle<sgfx::DrawQueueHandle>      DrawQueueHandle;
    typedef GraphicsObjectHandle<sgfx::ComputeQueueHandle>   ComputeQueueHandle;
    typedef GraphicsObjectHandle<sgfx::ComputeShaderHandle>  ComputeShaderHandle;
    typedef GraphicsObjectHandle<sgfx::VertexFormatHandle>   VertexFormatHandle;
}

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

    typedef std::vector<sgfx::ShaderCompileMacro> ShaderMacroVector;

    bool loadShader(const char* path, const ShaderMacroVector& macros, sgfx::ShaderCompileTarget target, void*& outData, size_t& outSize);
    sgfx::VertexShaderHandle   loadVS(const char* path, const ShaderMacroVector& macros = ShaderMacroVector());
    sgfx::GeometryShaderHandle loadGS(const char* path, const ShaderMacroVector& macros = ShaderMacroVector());
    sgfx::PixelShaderHandle    loadPS(const char* path, const ShaderMacroVector& macros = ShaderMacroVector());
    sgfx::ComputeShaderHandle  loadCS(const char* path, const ShaderMacroVector& macros = ShaderMacroVector());

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
    void setWindowTitle(const char* str);
};

extern Application* ApplicationInstance;

void sampleApplicationMain();

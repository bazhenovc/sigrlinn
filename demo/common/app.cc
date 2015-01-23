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
#include "app.hh"

#include <fstream>
#include <iostream>

Application* ApplicationInstance = nullptr;

void Application::genericErrorReporter(const char* msg)
{
    OutputDebugString(msg);
}

bool Application::loadShader(const char* path, sgfx::ShaderCompileTarget target, void*& outData, size_t& outSize)
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
            Application::genericErrorReporter,
            outData,
            outSize
        );
    }
    OutputDebugString("Failed to open file");
    return false;
}

sgfx::VertexShaderHandle Application::loadVS(const char* path)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (loadShader(path, sgfx::ShaderCompileTarget::VS, bytecode, bytecodeSize)) {
        sgfx::VertexShaderHandle vs = sgfx::createVertexShader(bytecode, bytecodeSize);
        OutputDebugString("Vertex shader compiled.\n");
        delete [] bytecode;
        return vs;
    }

    return sgfx::VertexShaderHandle::invalidHandle();
}

sgfx::GeometryShaderHandle Application::loadGS(const char* path)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (loadShader(path, sgfx::ShaderCompileTarget::GS, bytecode, bytecodeSize)) {
        sgfx::GeometryShaderHandle ret = sgfx::createGeometryShader(bytecode, bytecodeSize);
        OutputDebugString("Geometry shader compiled.\n");
        delete [] bytecode;
        return ret;
    }

    return sgfx::GeometryShaderHandle::invalidHandle();
}

sgfx::PixelShaderHandle Application::loadPS(const char* path)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (loadShader(path, sgfx::ShaderCompileTarget::PS, bytecode, bytecodeSize)) {
        sgfx::PixelShaderHandle ret = sgfx::createPixelShader(bytecode, bytecodeSize);
        OutputDebugString("Pixel shader compiled.\n");
        delete [] bytecode;
        return ret;
    }

    return sgfx::PixelShaderHandle::invalidHandle();
}

sgfx::ComputeShaderHandle Application::loadCS(const char* path)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    if (loadShader(path, sgfx::ShaderCompileTarget::CS, bytecode, bytecodeSize)) {
        sgfx::ComputeShaderHandle ret = sgfx::createComputeShader(bytecode, bytecodeSize);
        OutputDebugString("Compute shader compiled.\n");
        delete [] bytecode;
        return ret;
    }

    return sgfx::ComputeShaderHandle::invalidHandle();
}

sgfx::VertexFormatHandle Application::loadVF(sgfx::VertexElementDescriptor* vfElements, size_t vfElementsSize, const char* shaderPath)
{
    void* bytecode      = nullptr;
    size_t bytecodeSize = 0;

    sgfx::VertexFormatHandle ret = sgfx::VertexFormatHandle::invalidHandle();

    if (loadShader(shaderPath, sgfx::ShaderCompileTarget::VS, bytecode, bytecodeSize)) {
        ret = sgfx::createVertexFormat(
            vfElements, vfElementsSize,
            bytecode, bytecodeSize,
            Application::genericErrorReporter
        );
        delete [] bytecode;
    }

    return ret;
}



#include "app.hh"

#include <fstream>
#include <iostream>

Application* ApplicationInstance = nullptr;

static void genericErrorReporter(const char* msg)
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
            genericErrorReporter,
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


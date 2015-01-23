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
#include "meshloader.hh"

#include <sstream>
#include <fstream>
#include "app.hh"

void MeshData::read(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (file.is_open()) {
        OutputDebugString(("Loading mesh: " + path + "\n").c_str());

        while (!file.eof()) {
            ChunkHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            switch (header.id) {
            case idVertices:
                loadVertices(header, file);
                break;

            case idIndices:
                loadIndices(header, file);
                break;

            default: {
                    std::ostringstream ss;
                    ss << "Unknown chunk ID: " << header.id << " size: " << header.size <<" file: " << path << std::endl;
                    OutputDebugString(ss.str().c_str());
                    size_t curr = static_cast<size_t>(file.tellg());
                    file.seekg(curr + header.size);
                }
                break;
            }
        }

        std::ostringstream ss;
        ss
            << "Mesh loaded."
            << " Vertices: " << _vertices.size()
            << " indices: " << _indices.size()
            << std::endl;
        OutputDebugString(ss.str().c_str());
    } else {
        std::ostringstream ss;
        ss << "Failed to open file: " << path << std::endl;
        OutputDebugString(ss.str().c_str());
    }
}

void MeshData::loadVertices(const ChunkHeader& header, std::ifstream& file)
{
    size_t count = static_cast<size_t>(header.size);
    _vertices.resize(count / sizeof(Vertex));
    file.read(reinterpret_cast<char*>(_vertices.data()), sizeof(Vertex) * _vertices.size());
}

void MeshData::loadIndices(const ChunkHeader& header, std::ifstream& file)
{
    size_t count = static_cast<size_t>(header.size);
    _indices.resize(count / sizeof(Index));
    file.read(reinterpret_cast<char*>(_indices.data()), sizeof(Index) * _indices.size());
}

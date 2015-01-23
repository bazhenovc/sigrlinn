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

#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

class MeshData
{
public:
    // declarations
    enum ChunkID : uint64_t
    {
        idVertices = 1,
        idIndices = 2,
        idBones = 3
    };

    struct ChunkHeader
    {
        uint64_t id;
        uint64_t size;
    };
    static_assert(sizeof(ChunkHeader) == 16, "Invalid ChunkHeader size!");

    struct Vertex
    {
        enum { MaxBones = 4 };

        float position[3];
        float uv0[2];
        float uv1[2];
        float normal[3];
        uint8_t boneIDs[MaxBones];
        float   boneWeights[MaxBones];
        uint8_t color[4];

        Vertex()
        {
            std::memset(boneIDs, 0, sizeof(uint8_t) * MaxBones);
            std::memset(boneWeights, 0, sizeof(float) * MaxBones);
            std::memset(color, 255, sizeof(uint8_t) * 4);
        }
    };
    static_assert(sizeof(Vertex) == 64, "Vertex is supposed to be exactly 64 bytes!");

    typedef uint32_t Index;

    typedef std::vector<Vertex> VertexVector;
    typedef std::vector<Index>  IndexVector;

private:
    // data
    VertexVector _vertices;
    IndexVector  _indices;

    MeshData(const MeshData& other) = delete;
    MeshData& operator=(const MeshData& other) = delete;

public:

    MeshData() {}
    ~MeshData() {}

    void read(const std::string& path);

    inline VertexVector& getVertices() { return _vertices; }
    inline IndexVector&  getIndices()  { return _indices; }

private:

    void loadVertices(const ChunkHeader& header, std::ifstream& file);
    void loadIndices(const ChunkHeader& header, std::ifstream& file);
};

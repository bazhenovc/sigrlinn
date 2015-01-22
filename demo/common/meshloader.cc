//
// Drake3D engine version ${DK_ENGINE_VERSION}
// Copyright (C) 2011-2014 Kirill Bazhenov
//
// This copy is licensed to the following parties:
//
//       User:            ${REGISTERED_USER_NAME}
//       License:         ${REGISTERED_USER_LICENSE}
//       Number of users: ${REGISTERED_USER_MAX}
//
// License granted under the terms of the license agreement
// entered by the registered party.
//
// Unauthorized redistribution of this source code is
// strictly prohibited. Violators will be prosecuted.
//

#include "meshloader.hh"

#include <sstream>
#include <fstream>
#include "app.hh"

void MeshData::read(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (file.is_open()) {
        OutputDebugString(("Loading mesh[background]: " + path + "\n").c_str());

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

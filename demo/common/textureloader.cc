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
#include "textureloader.hh"

#include "sigrlinn.hh"
#include "app.hh"

#include <fstream>
#include <string>
#include <algorithm>

// DirectDraw pixel format
struct DDPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t bpp;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t alpha_mask;
};

// DDS caps
typedef uint32_t DDSCaps[4];

// DirectDraw color key
struct DDColorKey
{
    uint32_t low;
    uint32_t high;
};

// DirectDraw surface descriptor
struct DDSurface
{
    uint32_t      size;
    uint32_t      flags;
    uint32_t      height;
    uint32_t      width;
    uint32_t      pitch;
    uint32_t      depth;
    uint32_t      num_mipmaps;
    uint32_t      alpha_bit_depth;
    uint32_t      dds_reserved;
    uint32_t      surface;
    DDColorKey    dest_overlay;
    DDColorKey    dest_blit;
    DDColorKey    src_overlay;
    DDColorKey    src_blit;
    DDPixelFormat format;
    DDSCaps       caps;
    uint32_t      texture_stage;
};

#define DDS_MAKE_FOURCC(ch0, ch1, ch2, ch3) \
    ((uint32_t)( \
        (((uint32_t)(uint8_t)(ch3) << 24) & 0xFF000000) | \
        (((uint32_t)(uint8_t)(ch2) << 16) & 0x00FF0000) | \
        (((uint32_t)(uint8_t)(ch1) <<  8) & 0x0000FF00) | \
        (((uint32_t)(uint8_t)(ch0)        & 0x000000FF))))

#define DDS_FOURCC_DXT1 DDS_MAKE_FOURCC('D', 'X', 'T', '1')
#define DDS_FOURCC_DXT3 DDS_MAKE_FOURCC('D', 'X', 'T', '3')
#define DDS_FOURCC_DXT5 DDS_MAKE_FOURCC('D', 'X', 'T', '5')
#define DDS_FOURCC_BC4U DDS_MAKE_FOURCC('B', 'C', '4', 'U')
#define DDS_FOURCC_BC4S DDS_MAKE_FOURCC('B', 'C', '4', 'S')
#define DDS_FOURCC_BC5U DDS_MAKE_FOURCC('B', 'C', '5', 'U')
#define DDS_FOURCC_BC5S DDS_MAKE_FOURCC('B', 'C', '5', 'S')
#define DDS_FOURCC_ATI1 DDS_MAKE_FOURCC('A', 'T', 'I', '1')

static inline sgfx::DataFormat Fourcc2DataFormat(uint32_t fourcc)
{
    switch (fourcc)
    {
    case DDS_FOURCC_DXT1: { return sgfx::DataFormat::BC1; } break;
    case DDS_FOURCC_DXT3: { return sgfx::DataFormat::BC2; } break;
    case DDS_FOURCC_DXT5: { return sgfx::DataFormat::BC3; } break;
    case DDS_FOURCC_BC4U: { return sgfx::DataFormat::BC4; } break;
    case DDS_FOURCC_BC4S: { return sgfx::DataFormat::BC4; } break;
    case DDS_FOURCC_BC5U: { return sgfx::DataFormat::BC5; } break;
    case DDS_FOURCC_BC5S: { return sgfx::DataFormat::BC5; } break;
    }
    return sgfx::DataFormat::UnknownCompressed;
}

sgfx::Texture2DHandle loadDDS(const std::string& path)
{
    sgfx::Texture2DHandle texture;

    OutputDebugString(("Loading DDS: " + path + "\n").c_str());

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        size_t unused = 0;

        DDSurface surface;
        char magic[4];

        file.read(magic, 4);
        if (strncmp(magic, "DDS ", 4) != 0) {
            OutputDebugString(("Error: not a DDS texture: " + path + "\n").c_str());
            return sgfx::TextureHandle::invalidHandle();
        }

        file.read(reinterpret_cast<char*>(&surface), sizeof(surface));

        std::streamoff curr = file.tellg();
        file.seekg(0, std::fstream::end);
        std::streamoff end = file.tellg();
        file.seekg(curr);

        size_t bufferSize = static_cast<size_t>(end - curr);

        uint8_t* texels = new uint8_t[bufferSize];
        file.read(reinterpret_cast<char*>(texels), bufferSize);

        uint32_t blockSize = (surface.format.fourcc == DDS_FOURCC_DXT1) ? 8 : 16;
        sgfx::DataFormat format = Fourcc2DataFormat(surface.format.fourcc);

        texture = sgfx::createTexture2D(surface.width, surface.height, format, surface.num_mipmaps, 0U);
        if (texture == sgfx::TextureHandle::invalidHandle()) {
            delete [] texels;
            return sgfx::TextureHandle::invalidHandle();
        }

        size_t   offset    = 0;
        uint32_t mipWidth  = surface.width;
        uint32_t mipHeight = surface.height;
        size_t   mipSize   = 0;

        for (uint32_t mip = 0; mip < surface.num_mipmaps; ++mip) {
            mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
            sgfx::updateTexture(
                texture,
                texels + offset,
                mip,
                0, mipWidth,
                0, mipHeight,
                0, 1,
                ((mipWidth + 3) / 4) * blockSize, 0
            );

            mipWidth = std::max(mipWidth >> 1, 1U);
            mipHeight = std::max(mipHeight >> 1, 1U);

            offset += mipSize;

            if (mipWidth == 4 || mipHeight == 4)
                break;
        }

        OutputDebugString(("Loaded texture: " + path + "\n").c_str());

        delete [] texels;
    } else {
        OutputDebugString(("Failed to load texture: " + path + "\n").c_str());
    }

    return texture;
}

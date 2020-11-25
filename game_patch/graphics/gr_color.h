#pragma once

#include "../rf/bmpman.h"
#include <xlog/xlog.h>

inline int GetBmFormatSize(rf::BmFormat format)
{
    switch (format) {
    case rf::BM_FORMAT_8_PALETTED:
    case rf::BM_FORMAT_8_ALPHA:
        return 1;
    case rf::BM_FORMAT_565_RGB:
    case rf::BM_FORMAT_4444_ARGB:
    case rf::BM_FORMAT_1555_ARGB:
        return 2;
    case rf::BM_FORMAT_888_RGB:
        return 3;
    case rf::BM_FORMAT_8888_ARGB:
        return 4;
    default:
        xlog::warn("Unknown format: %d", format);
        return 2;
    }
}

#ifdef DIRECT3D_VERSION

inline D3DFORMAT GetD3DFormatFromBmFormat(rf::BmFormat format)
{
    switch (format) {
    case rf::BM_FORMAT_8_PALETTED:
        // Note: this indexed format is not really A8 but because there is no corresponding D3D format A8 format
        // is returned instead (it is used only for pixel size related calculations)
    case rf::BM_FORMAT_8_ALPHA:
        return D3DFMT_A8;
    case rf::BM_FORMAT_8888_ARGB:
        return D3DFMT_A8R8G8B8;
    case rf::BM_FORMAT_888_RGB:
        return D3DFMT_R8G8B8;
    case rf::BM_FORMAT_565_RGB:
        return D3DFMT_R5G6B5;
    case rf::BM_FORMAT_1555_ARGB:
        return D3DFMT_A1R5G5B5;
    case rf::BM_FORMAT_4444_ARGB:
        return D3DFMT_A4R4G4B4;
    case rf::BM_FORMAT_DXT1:
        return D3DFMT_DXT1;
    case rf::BM_FORMAT_DXT2:
        return D3DFMT_DXT2;
    case rf::BM_FORMAT_DXT3:
        return D3DFMT_DXT3;
    case rf::BM_FORMAT_DXT4:
        return D3DFMT_DXT4;
    case rf::BM_FORMAT_DXT5:
        return D3DFMT_DXT5;
    default:
        xlog::warn("unknown BM format %d", format);
        return D3DFMT_UNKNOWN;
    }
}

inline rf::BmFormat GetBmFormatFromD3DFormat(D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
    case D3DFMT_R5G6B5:
        return rf::BM_FORMAT_565_RGB;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        return rf::BM_FORMAT_1555_ARGB;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        return rf::BM_FORMAT_4444_ARGB;
    case D3DFMT_R8G8B8:
        return rf::BM_FORMAT_888_RGB;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return rf::BM_FORMAT_8888_ARGB;
    case D3DFMT_A8:
        return rf::BM_FORMAT_8_ALPHA;
    case D3DFMT_DXT1:
        return rf::BM_FORMAT_DXT1;
    case D3DFMT_DXT2:
        return rf::BM_FORMAT_DXT2;
    case D3DFMT_DXT3:
        return rf::BM_FORMAT_DXT3;
    case D3DFMT_DXT4:
        return rf::BM_FORMAT_DXT4;
    case D3DFMT_DXT5:
        return rf::BM_FORMAT_DXT5;
    default:
        xlog::error("unknown D3D format 0x%X", d3d_fmt);
        return rf::BM_FORMAT_NONE;
    }
}

#endif

void GrColorInit();
bool ConvertSurfaceFormat(void* dst_bits_ptr, rf::BmFormat dst_fmt, const void* src_bits_ptr,
                          rf::BmFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                          const uint8_t* palette = nullptr);

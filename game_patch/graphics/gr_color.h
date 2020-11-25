#pragma once

#include "../rf/bmpman.h"
#include <xlog/xlog.h>

inline int GetPixelFormatSize(rf::BmFormat pixel_fmt)
{
    switch (pixel_fmt) {
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
        xlog::warn("unknown pixel format %d", pixel_fmt);
        return 2;
    }
}

#ifdef DIRECT3D_VERSION

inline D3DFORMAT GetD3DFormatFromPixelFormat(rf::BmFormat pixel_fmt)
{
    switch (pixel_fmt) {
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
    default:
        if (static_cast<unsigned>(pixel_fmt) >= 0x10) {
            return static_cast<D3DFORMAT>(pixel_fmt);
        }
        xlog::warn("unknown pixel format %d", pixel_fmt);
        return D3DFMT_UNKNOWN;
    }
}

inline rf::BmFormat GetPixelFormatFromD3DFormat(D3DFORMAT d3d_fmt)
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
    default:
        xlog::error("unknown D3D format 0x%X", d3d_fmt);
        return static_cast<rf::BmFormat>(d3d_fmt);
    }
}

#endif

void GrColorInit();
bool ConvertSurfacePixelFormat(void* dst_bits_ptr, rf::BmFormat dst_fmt, const void* src_bits_ptr,
                               rf::BmFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                               const uint8_t* palette = nullptr);

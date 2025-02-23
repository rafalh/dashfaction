#pragma once

#include <windows.h>
#include <d3d8.h>
#include <xlog/xlog.h>
#include "../../rf/bmpman.h"

void gr_d3d_gamma_reset();
void gr_d3d_texture_device_lost();
bool gr_d3d_is_d3d8to9();
void gr_d3d_capture_device_lost();
void gr_d3d_capture_close();

inline D3DFORMAT get_d3d_format_from_bm_format(rf::bm::Format format)
{
    switch (format) {
    case rf::bm::FORMAT_8_PALETTED:
        // Note: this indexed format is not really A8 but because there is no corresponding D3D format A8 format
        // is returned instead (it is used only for pixel size related calculations)
    case rf::bm::FORMAT_8_ALPHA:
        return D3DFMT_A8;
    case rf::bm::FORMAT_8888_ARGB:
        return D3DFMT_A8R8G8B8;
    case rf::bm::FORMAT_888_RGB:
        return D3DFMT_R8G8B8;
    case rf::bm::FORMAT_565_RGB:
        return D3DFMT_R5G6B5;
    case rf::bm::FORMAT_1555_ARGB:
        return D3DFMT_A1R5G5B5;
    case rf::bm::FORMAT_4444_ARGB:
        return D3DFMT_A4R4G4B4;
    case rf::bm::FORMAT_DXT1:
        return D3DFMT_DXT1;
    // case rf::bm::FORMAT_DXT2:
    //     return D3DFMT_DXT2;
    case rf::bm::FORMAT_DXT3:
        return D3DFMT_DXT3;
    // case rf::bm::FORMAT_DXT4:
    //     return D3DFMT_DXT4;
    case rf::bm::FORMAT_DXT5:
        return D3DFMT_DXT5;
    default:
        xlog::warn("unknown BM format: {}", static_cast<int>(format));
        return D3DFMT_UNKNOWN;
    }
}

inline rf::bm::Format get_bm_format_from_d3d_format(D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
    case D3DFMT_R5G6B5:
        return rf::bm::FORMAT_565_RGB;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        return rf::bm::FORMAT_1555_ARGB;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        return rf::bm::FORMAT_4444_ARGB;
    case D3DFMT_R8G8B8:
        return rf::bm::FORMAT_888_RGB;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return rf::bm::FORMAT_8888_ARGB;
    case D3DFMT_A8:
        return rf::bm::FORMAT_8_ALPHA;
    case D3DFMT_DXT1:
        return rf::bm::FORMAT_DXT1;
    // case D3DFMT_DXT2:
    //     return rf::bm::FORMAT_DXT2;
    case D3DFMT_DXT3:
        return rf::bm::FORMAT_DXT3;
    // case D3DFMT_DXT4:
    //     return rf::bm::FORMAT_DXT4;
    case D3DFMT_DXT5:
        return rf::bm::FORMAT_DXT5;
    default:
        xlog::error("unknown D3D format: {}", static_cast<int>(d3d_fmt));
        return rf::bm::FORMAT_NONE;
    }
}

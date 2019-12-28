#pragma once

#include "../rf.h"
#include <log/Logger.h>

inline int GetPixelFormatSize(rf::BmPixelFormat pixel_fmt)
{
    switch (pixel_fmt) {
    case rf::BMPF_MONO8:
        return 1;
    case rf::BMPF_565:
    case rf::BMPF_4444:
    case rf::BMPF_1555:
        return 2;
    case rf::BMPF_888:
        return 3;
    case rf::BMPF_8888:
        return 4;
    default:
        WARN("unknown pixel format %d", pixel_fmt);
        return 2;
    }
}

inline D3DFORMAT GetD3DFormatFromPixelFormat(rf::BmPixelFormat pixel_fmt)
{
    switch (pixel_fmt) {
    case rf::BMPF_8888:
        return D3DFMT_A8R8G8B8;
    case rf::BMPF_888:
        return D3DFMT_R8G8B8;
    case rf::BMPF_565:
        return D3DFMT_R5G6B5;
    case rf::BMPF_1555:
        return D3DFMT_A1R5G5B5;
    case rf::BMPF_4444:
        return D3DFMT_A4R4G4B4;
    default:
        if (static_cast<unsigned>(pixel_fmt) >= 0x10) {
            return static_cast<D3DFORMAT>(pixel_fmt);
        }
        ERR("unknown pixel format %d", pixel_fmt);
        return D3DFMT_UNKNOWN;
    }
}

inline rf::BmPixelFormat GetPixelFormatFromD3DFormat(D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
    case D3DFMT_R5G6B5:
        return rf::BMPF_565;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        return rf::BMPF_1555;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        return rf::BMPF_4444;
    case D3DFMT_R8G8B8:
        return rf::BMPF_888;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return rf::BMPF_8888;
    default:
        ERR("unknown D3D format 0x%X", d3d_fmt);
        return static_cast<rf::BmPixelFormat>(d3d_fmt);
    }
}

void GrColorInit();
bool ConvertBitmapFormat(uint8_t* dst_bits_ptr, rf::BmPixelFormat dst_fmt, const uint8_t* src_bits_ptr,
                         rf::BmPixelFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                         const uint8_t* palette = nullptr, bool swap_bytes = false);

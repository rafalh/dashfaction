#pragma once

inline int GetPixelFormatSize(rf::BmPixelFormat PixelFmt)
{
    switch (PixelFmt)
    {
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
        WARN("unknown pixel format %d", PixelFmt);
        return 2;
    }
}

inline D3DFORMAT GetD3DFormatFromPixelFormat(rf::BmPixelFormat PixelFmt)
{
    switch (PixelFmt)
    {
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
        ERR("unknown pixel format %d", PixelFmt);
        return D3DFMT_UNKNOWN;
    }
}

inline rf::BmPixelFormat GetPixelFormatFromD3DFormat(D3DFORMAT D3DFormat)
{
    switch (D3DFormat)
    {
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
        ERR("unknown D3D format 0x%X", D3DFormat);
        return rf::BMPF_INVALID;
    }
}

void GrColorInit();

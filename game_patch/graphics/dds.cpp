#include <windows.h>
#include <d3d8.h>
#include <ddraw.h>
#include <xlog/xlog.h>
#include "graphics_internal.h"
#include "gr_color.h"
#include "../rf/bmpman.h"
#include "../rf/graphics.h"

struct DDS_PIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwABitMask;
};

typedef struct {
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwHeight;
    DWORD           dwWidth;
    DWORD           dwPitchOrLinearSize;
    DWORD           dwDepth;
    DWORD           dwMipMapCount;
    DWORD           dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD           dwCaps;
    DWORD           dwCaps2;
    DWORD           dwCaps3;
    DWORD           dwCaps4;
    DWORD           dwReserved2;
} DDS_HEADER;

constexpr uint32_t dds_magic = 0x20534444;

D3DFORMAT GetD3DFormatFromDDSPixelFormat(DDS_PIXELFORMAT& ddspf)
{
    if (ddspf.dwFlags & DDPF_RGB) {
        switch (ddspf.dwRGBBitCount) {
            case 32:
                if (ddspf.dwABitMask)
                    return D3DFMT_A8R8G8B8;
                else
                    return D3DFMT_X8R8G8B8;
            case 24:
                return D3DFMT_R8G8B8;
            case 16:
                if (ddspf.dwABitMask == 0x8000)
                    return D3DFMT_A1R5G5B5;
                else if (ddspf.dwABitMask)
                    return D3DFMT_A4R4G4B4;
                else
                    return D3DFMT_R5G6B5;
        }
    }
    else if (ddspf.dwFlags & DDPF_FOURCC) {
        switch (ddspf.dwFourCC) {
            case MAKEFOURCC('D', 'X', 'T', '1'):
                return D3DFMT_DXT1;
            case MAKEFOURCC('D', 'X', 'T', '2'):
                return D3DFMT_DXT2;
            case MAKEFOURCC('D', 'X', 'T', '3'):
                return D3DFMT_DXT3;
            case MAKEFOURCC('D', 'X', 'T', '4'):
                return D3DFMT_DXT4;
            case MAKEFOURCC('D', 'X', 'T', '5'):
                return D3DFMT_DXT5;
        }
    }
    xlog::warn("Unsupported DDS pixel format");
    return D3DFMT_UNKNOWN;
}

rf::BmBitmapType ReadDdsHeader(rf::File& file, int *width_out, int *height_out, rf::BmPixelFormat *pixel_fmt_out,
    int *num_levels_out)
{
    auto magic = file.Read<uint32_t>();
    if (magic != dds_magic) {
        xlog::warn("Invalid magic number in DDS file: %X", magic);
        return rf::BM_INVALID;
    }
    DDS_HEADER hdr;
    file.Read(&hdr, sizeof(hdr));
    if (hdr.dwSize != sizeof(DDS_HEADER)) {
        xlog::warn("Invalid header size in DDS file: %lX", hdr.dwSize);
        return rf::BM_INVALID;
    }
    D3DFORMAT d3d_fmt = GetD3DFormatFromDDSPixelFormat(hdr.ddspf);
    if (d3d_fmt == D3DFMT_UNKNOWN) {
        return rf::BM_INVALID;
    }
    auto hr = rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat, 0,
        D3DRTYPE_TEXTURE, d3d_fmt);
    if (FAILED(hr)) {
        xlog::warn("Unsupported by video card DDS pixel format: %X", d3d_fmt);
        return rf::BM_INVALID;
    }
    xlog::trace("Using DDS format %X", d3d_fmt);

    *width_out = hdr.dwWidth;
    *height_out = hdr.dwHeight;

    // Use D3DFORMAT as BmPixelFormats - there are no conflicts because D3DFORMAT constants starts from 0x20
    *pixel_fmt_out = static_cast<rf::BmPixelFormat>(d3d_fmt);

    *num_levels_out = 1;
    if (hdr.dwFlags & DDSD_MIPMAPCOUNT) {
        *num_levels_out = hdr.dwMipMapCount;
        xlog::trace("DDS mipmaps %lu (%lux%lu)", hdr.dwMipMapCount, hdr.dwWidth, hdr.dwHeight);
    }
    return rf::BM_DDS;
}

int LockDdsBitmap(rf::BmBitmapEntry& bm_entry)
{
    rf::File file;
    std::string filename_without_ext{GetFilenameWithoutExt(bm_entry.name)};
    auto dds_filename = filename_without_ext + ".dds";

    xlog::trace("Locking DDS: %s", dds_filename.c_str());
    if (file.Open(dds_filename.c_str()) != 0) {
        xlog::error("failed to open DDS file: %s", dds_filename.c_str());
        return -1;
    }

    auto magic = file.Read<uint32_t>();
    if (magic != dds_magic) {
        xlog::error("Invalid magic number in %s: %x", dds_filename.c_str(), magic);
        return -1;
    }
    DDS_HEADER hdr;
    file.Read(&hdr, sizeof(hdr));

    if (bm_entry.orig_width != static_cast<int>(hdr.dwWidth) || bm_entry.orig_height != static_cast<int>(hdr.dwHeight)) {
        xlog::error("Bitmap file has changed before being loaded!");
        return -1;
    }

    int w = bm_entry.orig_width;
    int h = bm_entry.orig_height;
    int num_skip_levels = std::min(bm_entry.resolution_level, 2);
    auto d3d_fmt = GetD3DFormatFromPixelFormat(bm_entry.pixel_format);
    int num_skip_bytes = 0;
    int num_total_bytes = 0;

    for (int i = 0; i < num_skip_levels + bm_entry.num_levels; ++i) {
        int num_surface_bytes = GetSurfaceLengthInBytes(w, h, d3d_fmt);
        if (i < num_skip_levels) {
            num_skip_bytes += num_surface_bytes;
        }
        else {
            num_total_bytes += num_surface_bytes;
        }
        w = std::max(w / 2, 1);
        h = std::max(h / 2, 1);
    }

    // Skip levels with most details (depending on graphics settings)
    file.Seek(num_skip_bytes, rf::File::seek_cur);

    // Load actual data
    bm_entry.locked_data = rf::Malloc(num_total_bytes);
    file.Read(bm_entry.locked_data, num_total_bytes);
    if (file.HasReadFailed()) {
        xlog::error("Unexpected EOF when reading %s", dds_filename.c_str());
        return -1;
    }
    return 0;
}

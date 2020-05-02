#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <ddraw.h>
#include <cstddef>
#include <set>
#include <algorithm>
#include "../rf/graphics.h"
#include "../utils/string-utils.h"
#include "gr_color.h"
#include "graphics_internal.h"

constexpr uint32_t dds_magic = 0x20534444;

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

namespace rf
{

struct GrTextureSection
{
    IDirect3DTexture8 *d3d_texture;
    int num_vram_bytes;
    float u_scale;
    float v_scale;
    int x;
    int y;
    int width;
    int height;
};

struct GrTexture
{
    int bm_handle;
    short num_sections;
    short preserve_counter;
    short lock_count;
    uint8_t ref_count;
    bool reset;
    GrTextureSection *sections;
};

}

constexpr rf::BmBitmapType dds_bm_type = static_cast<rf::BmBitmapType>(0x10);

std::set<rf::GrTexture*> g_default_pool_tslots;

int GetSurfacePitch(int w, D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            return w * 4;
        case D3DFMT_R8G8B8:
            return w * 3;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
            return w * 2;
        case D3DFMT_DXT1:
            // 4x4 pixels per block, 64 bits per block
            return (w + 3) / 4 * 64 / 8;
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // 4x4 pixels per block, 128 bits per block
            return (w + 3) / 4 * 128 / 8;
        default:
            return -1;
    }
}

int GetSurfaceNumRows(int h, D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // 4x4 pixels per block
            return (h + 3) / 4;
        default:
            return h;
    }
}

using GrD3DSetTextureData_Type =
    int(int, const uint8_t*, const uint8_t*, int, int, rf::BmPixelFormat, rf::GrTextureSection*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> gr_d3d_set_texture_data_hook{
    0x0055BA10,
    [](int level, const uint8_t* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::BmPixelFormat pixel_fmt, rf::GrTextureSection* section, int tex_w, int tex_h, IDirect3DTexture8* texture) {

        D3DLOCKED_RECT locked_rect;
        HRESULT hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            xlog::error("LockRect failed");
            return -1;
        }
        D3DSURFACE_DESC desc;
        texture->GetLevelDesc(level, &desc);

        bool success = true;
        if (static_cast<int>(pixel_fmt) >= 0x10) {
            xlog::trace("Writing texture in custom format level %d src %p bm %dx%d tex %dx%d section %d %d",
                level, src_bits_ptr, bm_w, bm_h, tex_w, tex_h, section->x, section->y);
            auto d3d_fmt = static_cast<D3DFORMAT>(pixel_fmt);
            auto src_pitch = GetSurfacePitch(bm_w, d3d_fmt);
            auto num_src_rows = GetSurfaceNumRows(bm_h, d3d_fmt);

            auto src_ptr = reinterpret_cast<const std::byte*>(src_bits_ptr);
            auto dst_ptr = reinterpret_cast<std::byte*>(locked_rect.pBits);

            src_ptr += src_pitch * GetSurfaceNumRows(section->y, d3d_fmt);
            src_ptr += GetSurfacePitch(section->x, d3d_fmt);

            auto copy_pitch = GetSurfacePitch(section->width, d3d_fmt);

            for (int y = 0; y < num_src_rows; ++y) {
                std::memcpy(dst_ptr, src_ptr, copy_pitch);
                dst_ptr += locked_rect.Pitch;
                src_ptr += src_pitch;
            }
            xlog::trace("Writing completed");
        }
        else {
            auto tex_pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
            success = ConvertBitmapFormat(reinterpret_cast<uint8_t*>(locked_rect.pBits), tex_pixel_fmt,
                                          src_bits_ptr, pixel_fmt, bm_w, bm_h, locked_rect.Pitch,
                                          GetPixelFormatSize(pixel_fmt) * bm_w, palette);
            if (!success)
                xlog::warn("Color conversion failed (format %d -> %d)", pixel_fmt, tex_pixel_fmt);
        }

        texture->UnlockRect(level);

        if (success)
            return 0;

        return gr_d3d_set_texture_data_hook.CallTarget(level, src_bits_ptr, palette, bm_w, bm_h, pixel_fmt, section,
                                                       tex_w, tex_h, texture);
    },
};

FunHook<int(rf::BmPixelFormat, int, int, int, IDirect3DTexture8**)> gr_d3d_create_vram_texture_hook{
    0x0055B970,
    [](rf::BmPixelFormat pixel_fmt, int width, int height, int levels, IDirect3DTexture8** texture_out) {
        D3DFORMAT d3d_format;
        D3DPOOL d3d_pool = D3DPOOL_MANAGED;
        int usage = 0;
        if (pixel_fmt == render_target_pixel_format) {
            xlog::trace("Creating render target texture");
            d3d_format = rf::gr_d3d_pp.BackBufferFormat;
            d3d_pool = D3DPOOL_DEFAULT;
            usage = D3DUSAGE_RENDERTARGET;
        }
        else if (pixel_fmt == dynamic_texture_pixel_format) {
            xlog::trace("Creating dynamic texture");
            d3d_format = rf::gr_d3d_pp.BackBufferFormat;
            // Note: In Direct3D 9 D3DUSAGE_DYNAMIC can be used but it has poor performance when texture is locked
            //       without D3DLOCK_DISCARD flag so keep it disabled for now
            // d3d_pool = D3DPOOL_DEFAULT;
            // usage = D3DUSAGE_DYNAMIC;
        }
        else if (static_cast<unsigned>(pixel_fmt) >= 0x10) {
            d3d_format = static_cast<D3DFORMAT>(pixel_fmt);
        }
        else {
            return gr_d3d_create_vram_texture_hook.CallTarget(pixel_fmt, width, height, levels, texture_out);
        }

        xlog::trace("Creating texture in format %x", d3d_format);
        auto hr = rf::gr_d3d_device->CreateTexture(width, height, levels, usage, d3d_format, d3d_pool, texture_out);
        if (FAILED(hr)) {
            xlog::error("Failed to create texture %dx%d in format %x", width, height, d3d_format);
            return -1;
        }
        return 0;
    },
};

CodeInjection gr_d3d_create_vram_texture_with_mipmaps_pitch_fix{
    0x0055B820,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x5C;
        auto pixel_fmt = AddrAsRef<rf::BmPixelFormat>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto d3d_fmt = GetD3DFormatFromPixelFormat(pixel_fmt);
        int total_surface_bytes = GetSurfacePitch(w, d3d_fmt) * GetSurfaceNumRows(h, d3d_fmt);
        src_bits_ptr += total_surface_bytes;
        num_total_vram_bytes += total_surface_bytes;
    },
};

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
    return dds_bm_type;
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
        int num_surface_bytes = GetSurfacePitch(w, d3d_fmt) * GetSurfaceNumRows(h, d3d_fmt);
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

FunHook<rf::BmBitmapType(const char*, int*, int*, rf::BmPixelFormat*, int*, int*, int*, int*, int*, int*, int)>
bm_read_header_hook{
    0x0050FCB0,
    [](const char* filename, int* width_out, int* height_out, rf::BmPixelFormat *pixel_fmt_out, int *num_levels_out,
    int *num_levels_external_mips_out, int *num_frames_out, int *fps_out, int *total_bytes_m2v_out,
    int *vbm_ver_out, int a11) {

        *num_levels_external_mips_out = 1;
        *num_levels_out = 1;
        *fps_out = 0;
        *num_frames_out = 1;
        *total_bytes_m2v_out = -1;
        *vbm_ver_out = 1;

        rf::File dds_file;
        std::string filename_without_ext{GetFilenameWithoutExt(filename)};
        auto dds_filename = filename_without_ext + ".dds";
        if (dds_file.Open(dds_filename.c_str()) == 0) {
            xlog::trace("Loading %s", dds_filename.c_str());
            auto bm_type = ReadDdsHeader(dds_file, width_out, height_out, pixel_fmt_out, num_levels_out);
            if (bm_type != rf::BM_INVALID) {
                return bm_type;
            }
        }

        return bm_read_header_hook.CallTarget(filename, width_out, height_out, pixel_fmt_out, num_levels_out,
            num_levels_external_mips_out, num_frames_out, fps_out, total_bytes_m2v_out, vbm_ver_out, a11);
    },
};

FunHook<rf::BmPixelFormat(int, void**, void**)> bm_lock_hook{
    0x00510780,
    [](int bmh, void** pixels_out, void** palette_out) {
        auto& bm_entry = rf::bm_bitmaps[rf::BmHandleToIdxAnimAware(bmh)];
        if (bm_entry.bitmap_type == dds_bm_type) {
            LockDdsBitmap(bm_entry);
            *pixels_out = bm_entry.locked_data;
            *palette_out = bm_entry.locked_palette;
            return bm_entry.pixel_format;
        }
        else {
            return bm_lock_hook.CallTarget(bmh, pixels_out, palette_out);
        }
    },
};

FunHook <void(int, float, float, rf::Color*)> gr_d3d_get_bitmap_pixel_hook{
    0x0055CFA0,
    [](int bmh, float u, float v, rf::Color* out_color) {
        if (static_cast<unsigned>(rf::BmGetPixelFormat(bmh)) > 0x10) {
            // Reading pixels is not yet supported in custom texture formats
            // This is only used when shooting at a texture with alpha
            out_color->red = 255;
            out_color->green = 255;
            out_color->blue = 255;
            out_color->alpha = 255;
        }
        else {
            gr_d3d_get_bitmap_pixel_hook.CallTarget(bmh, u, v, out_color);
        }
    },
};

FunHook<int(int, rf::GrTexture&)> gr_d3d_create_texture_hook{
    0x0055CC00,
    [](int bm_handle, rf::GrTexture& tslot) {
        auto result = gr_d3d_create_texture_hook.CallTarget(bm_handle, tslot);
        if (result != 1) {
            xlog::warn("Failed to load texture %s", rf::BmGetFilename(bm_handle));
            return result;
        }
        auto pixel_fmt = rf::BmGetPixelFormat(bm_handle);
        if (pixel_fmt == render_target_pixel_format || pixel_fmt == dynamic_texture_pixel_format) {
            g_default_pool_tslots.insert(&tslot);
        }
        return result;
    },
};

FunHook<void(rf::GrTexture&)> gr_d3d_free_texture_hook{
    0x0055B640,
    [](rf::GrTexture& tslot) {
        gr_d3d_free_texture_hook.CallTarget(tslot);

        if (tslot.bm_handle >= 0) {
            xlog::info("Freeing texture");
            auto it = g_default_pool_tslots.find(&tslot);
            if (it != g_default_pool_tslots.end()) {
                g_default_pool_tslots.erase(it);
            }
        }
    },
};

// FunHook<void(int, int, int)> gr_d3d_set_state_and_texture_hook{
//     0x00550850,
//     [](int state, int bm0, int bm1) {
//         auto bm0_filename = rf::BmGetFilename(bm0);
//         if (bm0_filename && strstr(bm0_filename, "grate")) {
//             // disable alpha-blending
//             state &= ~(0x1F << 15);
//         }
//         gr_d3d_set_state_and_texture_hook.CallTarget(state, bm0, bm1);
//     },
// };

void ApplyTexturePatches()
{
    gr_d3d_set_texture_data_hook.Install();
    gr_d3d_create_vram_texture_hook.Install();
    bm_read_header_hook.Install();
    bm_lock_hook.Install();
    gr_d3d_create_vram_texture_with_mipmaps_pitch_fix.Install();
    gr_d3d_get_bitmap_pixel_hook.Install();
    gr_d3d_create_texture_hook.Install();
    gr_d3d_free_texture_hook.Install();
    //gr_d3d_set_state_and_texture_hook.Install();
}

void ReleaseAllDefaultPoolTextures()
{
    for (auto tslot : g_default_pool_tslots) {
        gr_d3d_free_texture_hook.CallTarget(*tslot);
    }
    g_default_pool_tslots.clear();
}

void DestroyTexture(int bmh)
{
    auto& gr_d3d_tcache_add_ref = AddrAsRef<void(int bmh)>(0x0055D160);
    auto& gr_d3d_tcache_remove_ref = AddrAsRef<void(int bmh)>(0x0055D190);
    gr_d3d_tcache_add_ref(bmh);
    gr_d3d_tcache_remove_ref(bmh);
}

void ChangeUserBitmapPixelFormat(int bmh, rf::BmPixelFormat pixel_fmt)
{
    DestroyTexture(bmh);
    int bm_idx = rf::BmHandleToIdxAnimAware(bmh);
    auto& bm = rf::bm_bitmaps[bm_idx];
    assert(bm.bitmap_type == rf::BM_USERBMAP);
    bm.pixel_format = pixel_fmt;
}

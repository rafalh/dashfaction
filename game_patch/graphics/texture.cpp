#include <windows.h>
#include <d3d8.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <ddraw.h>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <set>
#include <algorithm>
#include "../rf/graphics.h"
#include "../utils/string-utils.h"
#include "../main.h"
#include "gr_color.h"
#include "graphics_internal.h"

std::set<rf::GrD3DTexture*> g_default_pool_tslots;

class D3DTextureFormatSelector
{
    D3DFORMAT rgb_888_format_;
    D3DFORMAT rgba_8888_format_;
    D3DFORMAT rgb_565_format_;
    D3DFORMAT argb_1555_format_;
    D3DFORMAT argb_4444_format_;
    D3DFORMAT a_8_format_;

    template<size_t N>
    D3DFORMAT find_best_format(D3DFORMAT (&&allowed)[N])
    {
        for (auto d3d_fmt : allowed) {
            auto hr = rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat, 0,
                D3DRTYPE_TEXTURE, d3d_fmt);
            if (SUCCEEDED(hr)) {
                return d3d_fmt;
            }
        }
        return D3DFMT_UNKNOWN;
    }

public:
    void init()
    {
        if (g_game_config.res_bpp == 32 && g_game_config.true_color_textures) {
            xlog::info("Using 32-bit texture formats");
            rgb_888_format_ = find_best_format({D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5});
            rgba_8888_format_ = find_best_format({D3DFMT_A8R8G8B8, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5});
            rgb_565_format_ = find_best_format({D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_X1R5G5B5});
            argb_1555_format_ = find_best_format({D3DFMT_A1R5G5B5, D3DFMT_A8R8G8B8, D3DFMT_A4R4G4B4});
            argb_4444_format_ = find_best_format({D3DFMT_A4R4G4B4, D3DFMT_A8R8G8B8, D3DFMT_A1R5G5B5});
            a_8_format_ = find_best_format({D3DFMT_A8, D3DFMT_A8R8G8B8, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5});
        }
        else {
            xlog::info("Using 16-bit texture formats");
            rgb_888_format_ = find_best_format({D3DFMT_R5G6B5, D3DFMT_X1R5G5B5});
            rgba_8888_format_ = find_best_format({D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5});
            rgb_565_format_ = find_best_format({D3DFMT_R5G6B5, D3DFMT_X1R5G5B5});
            argb_1555_format_ = find_best_format({D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4});
            argb_4444_format_ = find_best_format({D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5});
            a_8_format_ = find_best_format({D3DFMT_A8, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5});
        }
        xlog::debug("rgb_888_format %d", rgb_888_format_);
        xlog::debug("rgba_8888_format %d", rgba_8888_format_);
        xlog::debug("rgb_565_format %d", rgb_565_format_);
        xlog::debug("argb_1555_format %d", argb_1555_format_);
        xlog::debug("argb_4444_format %d", argb_4444_format_);
        xlog::debug("a_8_format %d", a_8_format_);
    }

    D3DFORMAT select(rf::BmFormat pixel_fmt)
    {
        if (static_cast<unsigned>(pixel_fmt) >= 20) {
            return static_cast<D3DFORMAT>(pixel_fmt);
        }
        switch (pixel_fmt) {
            case rf::BM_FORMAT_A_8:
                return a_8_format_;
            case rf::BM_FORMAT_BGR_888_INDEXED:
                return rgb_888_format_;
            case rf::BM_FORMAT_RGB_888:
                return rgb_888_format_;
            case rf::BM_FORMAT_RGB_565:
                return rgb_565_format_;
            case rf::BM_FORMAT_ARGB_1555:
                return argb_1555_format_;
            case rf::BM_FORMAT_ARGB_8888:
                return rgba_8888_format_;
            case rf::BM_FORMAT_ARGB_4444:
                return argb_4444_format_;
            default:
                return D3DFMT_UNKNOWN;
        }
    }
};
D3DTextureFormatSelector g_texture_format_selector;

int GetSurfacePitch(int w, D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
            return w * 4;
        case D3DFMT_R8G8B8:
            return w * 3;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_X4R4G4B4:
            return w * 2;
        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
            return w;
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

size_t GetSurfaceLengthInBytes(int w, int h, D3DFORMAT d3d_fmt)
{
    return GetSurfacePitch(w, d3d_fmt) * GetSurfaceNumRows(h, d3d_fmt);
}

using GrD3DSetTextureData_Type =
    int(int, const uint8_t*, const uint8_t*, int, int, rf::BmFormat, rf::GrD3DTextureSection*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> gr_d3d_set_texture_data_hook{
    0x0055BA10,
    [](int level, const uint8_t* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::BmFormat pixel_fmt, rf::GrD3DTextureSection* section, int tex_w, int tex_h, IDirect3DTexture8* texture) {

        D3DSURFACE_DESC desc;
        auto hr = texture->GetLevelDesc(level, &desc);
        if (FAILED(hr)) {
            xlog::error("GetLevelDesc failed %lx", hr);
            return -1;
        }

        auto tex_pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
        if (static_cast<int>(pixel_fmt) < 0x10 && GetPixelFormatSize(tex_pixel_fmt) == 2) {
            // original code can handle only 16 bit surfaces
            return gr_d3d_set_texture_data_hook.CallTarget(level, src_bits_ptr, palette, bm_w, bm_h, pixel_fmt, section,
                                                           tex_w, tex_h, texture);
        }

        D3DLOCKED_RECT locked_rect;
        hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            xlog::error("LockRect failed");
            return -1;
        }

        bool success = true;
        if (static_cast<int>(pixel_fmt) >= 20) {
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
            auto bm_pitch = GetPixelFormatSize(pixel_fmt) * bm_w;
            success = ConvertSurfacePixelFormat(locked_rect.pBits, tex_pixel_fmt,
                                                src_bits_ptr, pixel_fmt, bm_w, bm_h, locked_rect.Pitch,
                                                bm_pitch, palette);
            if (!success)
                xlog::warn("Color conversion failed (format %d -> %d)", pixel_fmt, tex_pixel_fmt);
        }

        texture->UnlockRect(level);

        return success ? 0 : -1;
    },
};

FunHook<int(rf::BmFormat, int, int, int, IDirect3DTexture8**)> gr_d3d_create_vram_texture_hook{
    0x0055B970,
    [](rf::BmFormat pixel_fmt, int width, int height, int levels, IDirect3DTexture8** texture_out) {
        D3DFORMAT d3d_format;
        D3DPOOL d3d_pool = D3DPOOL_MANAGED;
        int usage = 0;
        if (pixel_fmt == rf::BM_FORMAT_RENDER_TARGET) {
            xlog::trace("Creating render target texture");
            d3d_format = rf::gr_d3d_pp.BackBufferFormat;
            d3d_pool = D3DPOOL_DEFAULT;
            usage = D3DUSAGE_RENDERTARGET;
        }
        else if (static_cast<unsigned>(pixel_fmt) >= 0x10) {
            d3d_format = static_cast<D3DFORMAT>(pixel_fmt);
        }
        else {
            d3d_format = g_texture_format_selector.select(pixel_fmt);
            if (d3d_format == D3DFMT_UNKNOWN) {
                xlog::error("Failed to determine texture format for pixel format %u", d3d_format);
                return -1;
            }
        }

        // TODO: In Direct3D 9 D3DPOOL_DEFAULT + D3DUSAGE_DYNAMIC can be used
        // Note: it has poor performance when texture is locked without D3DLOCK_DISCARD flag

        xlog::trace("Creating texture in format %x", d3d_format);
        auto hr = rf::gr_d3d_device->CreateTexture(width, height, levels, usage, d3d_format, d3d_pool, texture_out);
        if (FAILED(hr)) {
            xlog::error("Failed to create texture %dx%d in format %u", width, height, d3d_format);
            return -1;
        }
        return 0;
    },
};

CodeInjection gr_d3d_create_vram_texture_with_mipmaps_pitch_fix{
    0x0055B820,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x5C;
        auto pixel_fmt = AddrAsRef<rf::BmFormat>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto vram_tex_fmt = g_texture_format_selector.select(pixel_fmt);
        auto src_d3d_fmt = GetD3DFormatFromPixelFormat(pixel_fmt);
        src_bits_ptr += GetSurfaceLengthInBytes(w, h, src_d3d_fmt);
        num_total_vram_bytes += GetSurfaceLengthInBytes(w, h, vram_tex_fmt);
    },
};

FunHook<rf::BmType(const char*, int*, int*, rf::BmFormat*, int*, int*, int*, int*, int*, int*, int)>
bm_read_header_hook{
    0x0050FCB0,
    [](const char* filename, int* width_out, int* height_out, rf::BmFormat *pixel_fmt_out, int *num_levels_out,
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
            if (bm_type != rf::BM_TYPE_INVALID) {
                return bm_type;
            }
        }

        xlog::trace("Loading bitmap header for '%s'", filename);
        auto bm_type = bm_read_header_hook.CallTarget(filename, width_out, height_out, pixel_fmt_out, num_levels_out,
            num_levels_external_mips_out, num_frames_out, fps_out, total_bytes_m2v_out, vbm_ver_out, a11);
        xlog::trace("Bitmap header for '%s': type %d size %dx%d pixel_fmt %d levels %d frames %d",
            filename, bm_type, *width_out, *height_out, *pixel_fmt_out, *num_levels_out, *num_frames_out);

        // Sanity checks
        // Prevents heap corruption when width = 0 or height = 0
        if (*width_out <= 0 || *height_out <= 0 || *pixel_fmt_out == rf::BM_FORMAT_INVALID || *num_levels_out < 1 || *num_frames_out < 1) {
            bm_type = rf::BM_TYPE_INVALID;
        }

        if (bm_type == rf::BM_TYPE_INVALID) {
            xlog::warn("Failed load bitmap header for '%s'", filename);
        }

        return bm_type;
    },
};

FunHook<rf::BmFormat(int, void**, void**)> bm_lock_hook{
    0x00510780,
    [](int bmh, void** pixels_out, void** palette_out) {
        auto& bm_entry = rf::bm_bitmaps[rf::BmHandleToIdxAnimAware(bmh)];
        if (bm_entry.bitmap_type == rf::BM_TYPE_DDS) {
            LockDdsBitmap(bm_entry);
            *pixels_out = bm_entry.locked_data;
            *palette_out = bm_entry.locked_palette;
            return bm_entry.pixel_format;
        }
        else {
            auto pixel_fmt = bm_lock_hook.CallTarget(bmh, pixels_out, palette_out);
            if (pixel_fmt == rf::BM_FORMAT_INVALID) {
                *pixels_out = nullptr;
                *palette_out = nullptr;
                xlog::warn("bm_lock failed");
            }
            return pixel_fmt;
        }
    },
};

FunHook <void(int, float, float, rf::Color*)> gr_d3d_get_texel_hook{
    0x0055CFA0,
    [](int bmh, float u, float v, rf::Color* out_color) {
        if (static_cast<unsigned>(rf::BmGetFormat(bmh)) > 0x10) {
            // Reading pixels is not yet supported in custom texture formats
            // This is only used when shooting at a texture with alpha
            out_color->red = 255;
            out_color->green = 255;
            out_color->blue = 255;
            out_color->alpha = 255;
        }
        else {
            gr_d3d_get_texel_hook.CallTarget(bmh, u, v, out_color);
        }
    },
};

FunHook<int(int, rf::GrD3DTexture&)> gr_d3d_create_texture_hook{
    0x0055CC00,
    [](int bm_handle, rf::GrD3DTexture& tslot) {
        auto result = gr_d3d_create_texture_hook.CallTarget(bm_handle, tslot);
        if (result != 1) {
            xlog::warn("Failed to load texture '%s'", rf::BmGetFilename(bm_handle));
            // Note: callers of this function expects zero result on failure
            return 0;
        }
        auto pixel_fmt = rf::BmGetFormat(bm_handle);
        if (pixel_fmt == rf::BM_FORMAT_RENDER_TARGET) {
            g_default_pool_tslots.insert(&tslot);
        }
        return result;
    },
};

FunHook<void(rf::GrD3DTexture&)> gr_d3d_free_texture_hook{
    0x0055B640,
    [](rf::GrD3DTexture& tslot) {
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
    gr_d3d_get_texel_hook.Install();
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

void ChangeUserBitmapPixelFormat(int bmh, rf::BmFormat pixel_fmt, [[ maybe_unused ]] bool dynamic)
{
    DestroyTexture(bmh);
    int bm_idx = rf::BmHandleToIdxAnimAware(bmh);
    auto& bm = rf::bm_bitmaps[bm_idx];
    assert(bm.bitmap_type == rf::BM_USERBMAP);
    bm.pixel_format = pixel_fmt;
    // TODO: in DX9 use dynamic flag
}

void InitSupportedTextureFormats()
{
    g_texture_format_selector.init();
}

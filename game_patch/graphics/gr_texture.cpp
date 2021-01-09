#include <windows.h>
#include <d3d8.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <ddraw.h>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <set>
#include <unordered_set>
#include <algorithm>
#include "../rf/gr.h"
#include "../rf/gr_direct3d.h"
#include <common/utils/string-utils.h>
#include "../main/main.h"
#include "gr_color.h"
#include "graphics_internal.h"
#include "graphics.h"
#include "../bmpman/bmpman.h"

std::set<rf::GrD3DTexture*> g_default_pool_tslots;
int g_currently_creating_texture_for_bitmap = -1;

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

    D3DFORMAT select(rf::BmFormat bm_format)
    {
        switch (bm_format) {
            case rf::BM_FORMAT_8_ALPHA:
                return a_8_format_;
            case rf::BM_FORMAT_8_PALETTED:
                return rgb_888_format_;
            case rf::BM_FORMAT_888_RGB:
                return rgb_888_format_;
            case rf::BM_FORMAT_565_RGB:
                return rgb_565_format_;
            case rf::BM_FORMAT_1555_ARGB:
                return argb_1555_format_;
            case rf::BM_FORMAT_8888_ARGB:
                return rgba_8888_format_;
            case rf::BM_FORMAT_4444_ARGB:
                return argb_4444_format_;
            default:
                return get_d3d_format_from_bm_format(bm_format);
        }
    }
};
D3DTextureFormatSelector g_texture_format_selector;

using GrD3DSetTextureData_Type =
    int(int, const uint8_t*, const uint8_t*, int, int, rf::BmFormat, rf::GrD3DTextureSection*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> gr_d3d_set_texture_data_hook{
    0x0055BA10,
    [](int level, const uint8_t* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::BmFormat format, rf::GrD3DTextureSection* section, int tex_w, int tex_h, IDirect3DTexture8* texture) {
        xlog::trace("gr_d3d_set_texture_data_hook");

        D3DSURFACE_DESC desc;
        auto hr = texture->GetLevelDesc(level, &desc);
        if (FAILED(hr)) {
            xlog::error("GetLevelDesc failed %lx", hr);
            return -1;
        }

        auto tex_pixel_fmt = get_bm_format_from_d3d_format(desc.Format);
        if (!bm_is_compressed_format(tex_pixel_fmt) && bm_bytes_per_pixel(tex_pixel_fmt) == 2) {
            // original code can handle only 16 bit surfaces
            return gr_d3d_set_texture_data_hook.call_target(level, src_bits_ptr, palette, bm_w, bm_h, format, section,
                                                           tex_w, tex_h, texture);
        }

        D3DLOCKED_RECT locked_rect;
        hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            xlog::error("LockRect failed");
            return -1;
        }

        bool success = true;
        if (bm_is_compressed_format(format)) {
            xlog::trace("Writing texture in compressed format level %d src %p bm %dx%d tex %dx%d section %d %d",
                level, src_bits_ptr, bm_w, bm_h, tex_w, tex_h, section->x, section->y);
            auto src_pitch = get_surface_pitch(bm_w, format);
            auto num_src_rows = get_surface_num_rows(bm_h, format);

            auto src_ptr = reinterpret_cast<const std::byte*>(src_bits_ptr);
            auto dst_ptr = reinterpret_cast<std::byte*>(locked_rect.pBits);

            src_ptr += src_pitch * get_surface_num_rows(section->y, format);
            src_ptr += get_surface_pitch(section->x, format);

            auto copy_pitch = get_surface_pitch(section->width, format);

            for (int y = 0; y < num_src_rows; ++y) {
                std::memcpy(dst_ptr, src_ptr, copy_pitch);
                dst_ptr += locked_rect.Pitch;
                src_ptr += src_pitch;
            }
            xlog::trace("Writing completed");
        }
        else {
            auto bm_pitch = bm_bytes_per_pixel(format) * bm_w;
            success = convert_surface_format(locked_rect.pBits, tex_pixel_fmt,
                                                src_bits_ptr, format, bm_w, bm_h, locked_rect.Pitch,
                                                bm_pitch, palette);
            if (!success)
                xlog::warn("Color conversion failed (format %d -> %d)", format, tex_pixel_fmt);
        }

        texture->UnlockRect(level);

        return success ? 0 : -1;
    },
};

FunHook<int(rf::BmFormat, int, int, int, IDirect3DTexture8**)> gr_d3d_create_vram_texture_hook{
    0x0055B970,
    [](rf::BmFormat format, int width, int height, int levels, IDirect3DTexture8** texture_out) {
        xlog::trace("gr_d3d_create_vram_texture_hook");
        D3DFORMAT d3d_format;
        D3DPOOL d3d_pool = D3DPOOL_MANAGED;
        int usage = 0;
        if (format == rf::BM_FORMAT_RENDER_TARGET) {
            xlog::trace("Creating render target texture");
            d3d_format = rf::gr_d3d_pp.BackBufferFormat;
            d3d_pool = D3DPOOL_DEFAULT;
            usage = D3DUSAGE_RENDERTARGET;
        }
        else {
            d3d_format = g_texture_format_selector.select(format);
            if (d3d_format == D3DFMT_UNKNOWN) {
                xlog::error("Failed to determine texture format for pixel format %u", d3d_format);
                return -1;
            }
            assert(g_currently_creating_texture_for_bitmap != -1);
            if (bm_is_dynamic(g_currently_creating_texture_for_bitmap) && gr_d3d_is_d3d8to9()) {
                d3d_pool = D3DPOOL_DEFAULT;
                usage = D3DUSAGE_DYNAMIC;
                xlog::info("Creating dynamic texture %dx%d", width, height);
            }
        }

        xlog::trace("Creating texture bm_format 0x%X d3d_format 0x%X", format, d3d_format);
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
        auto bm_format = addr_as_ref<rf::BmFormat>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto vram_tex_fmt = g_texture_format_selector.select(bm_format);
        src_bits_ptr += get_surface_length_in_bytes(w, h, bm_format);
        num_total_vram_bytes += get_surface_length_in_bytes(w, h, get_bm_format_from_d3d_format(vram_tex_fmt));
    },
};

FunHook<void(int, float, float, rf::Color*)> gr_d3d_get_texel_hook{
    0x0055CFA0,
    [](int bm_handle, float u, float v, rf::Color* out_color) {
        // This function is only used when shooting at a texture with alpha
        // Note: original function has out of bounds error - it reads color for 16bpp and 32bpp (two addresses)
        // before actually checking the format
        rf::GrLockInfo lock;
        if (rf::gr_lock(bm_handle, 0, &lock, rf::GR_LOCK_READ_ONLY)) {
            // Make sure u and v are in [0, 1] range
            // Assume wrap addressing mode
            u = std::fmod(u, 1.0f);
            v = std::fmod(v, 1.0f);
            if (u < 0.0f) {
                u += 1.0f;
            }
            if (v < 0.0f) {
                v += 1.0f;
            }
            int x = static_cast<int>(u * lock.w + 0.5f);
            int y = static_cast<int>(v * lock.h + 0.5f);
            *out_color = bm_get_pixel(lock.data, lock.format, lock.stride_in_bytes, x, y);


            rf::gr_unlock(&lock);
        }
        else {
            out_color->set(255, 255, 255, 255);
        }
    },
};

extern FunHook<int(int, rf::GrD3DTexture&)> gr_d3d_create_texture_hook;

int gr_d3d_create_texture(int bm_handle, rf::GrD3DTexture& tslot) {
    g_currently_creating_texture_for_bitmap = bm_handle;
    auto result = gr_d3d_create_texture_hook.call_target(bm_handle, tslot);
    g_currently_creating_texture_for_bitmap = -1;

    if (result != 1) {
        xlog::warn("Failed to load texture '%s'", rf::bm_get_filename(bm_handle));
        // Note: callers of this function expects zero result on failure
        return 0;
    }
    auto format = rf::bm_get_format(bm_handle);
    if (format == rf::BM_FORMAT_RENDER_TARGET || (bm_is_dynamic(bm_handle) && gr_d3d_is_d3d8to9())) {
        g_default_pool_tslots.insert(&tslot);
    }
    return result;
}

FunHook<int(int, rf::GrD3DTexture&)> gr_d3d_create_texture_hook{0x0055CC00, gr_d3d_create_texture};

FunHook<void(rf::GrD3DTexture&)> gr_d3d_free_texture_hook{
    0x0055B640,
    [](rf::GrD3DTexture& tslot) {
        gr_d3d_free_texture_hook.call_target(tslot);

        if (tslot.bm_handle >= 0) {
            xlog::trace("Freeing texture");
            auto it = g_default_pool_tslots.find(&tslot);
            if (it != g_default_pool_tslots.end()) {
                g_default_pool_tslots.erase(it);
            }
        }
    },
};

bool gr_d3d_lock(int bm_handle, int section, rf::GrLockInfo *lock) {
    xlog::trace("gr_d3d_lock");
    auto& tslot = rf::gr_d3d_textures[rf::bm_handle_to_index_anim_aware(bm_handle)];
    if (tslot.num_sections < 1 || tslot.bm_handle != bm_handle) {
        auto ret = gr_d3d_create_texture(bm_handle, tslot);
        if (ret != 1) {
            return false;
        }
    }
    D3DLOCKED_RECT locked_rect;
    auto d3d_texture = tslot.sections[section].d3d_texture;
    DWORD lock_flags = 0;
    if (lock->mode == rf::GR_LOCK_READ_ONLY) {
        lock_flags = D3DLOCK_READONLY;
    }
    else if (lock->mode == rf::GR_LOCK_WRITE_ONLY && gr_d3d_is_d3d8to9()) {
        lock_flags = D3DLOCK_DISCARD;
    }
    auto hr = d3d_texture->LockRect(0, &locked_rect, nullptr, lock_flags);
    if (FAILED(hr)) {
        xlog::warn("gr_d3d_lock: IDirect3DTexture8::LockRect failed with result 0x%lx", hr);
        return false;
    }

    tslot.lock_count++;
    D3DSURFACE_DESC desc;
    hr = d3d_texture->GetLevelDesc(0, &desc);
    if (FAILED(hr)) {
        xlog::warn("gr_d3d_lock: IDirect3DTexture8::GetLevelDesc failed with result %lx", hr);
        return false;
    }

    lock->data = reinterpret_cast<rf::ubyte*>(locked_rect.pBits);
    lock->stride_in_bytes = locked_rect.Pitch;
    lock->w = desc.Width;
    lock->h = desc.Height;
    lock->section = section;
    lock->bm_handle = bm_handle;
    lock->format = get_bm_format_from_d3d_format(desc.Format);
    return true;
}

static FunHook<bool(int, int, rf::GrLockInfo *)> gr_d3d_lock_hook{0x0055CE00, gr_d3d_lock};

// FunHook<void(rf::GrMode, int, int)> gr_d3d_set_state_and_texture_hook{
//     0x00550850,
//     [](rf::GrMode mode, int bm0, int bm1) {
//         auto bm0_filename = rf::bm_get_filename(bm0);
//         if (bm0_filename && strstr(bm0_filename, "grate")) {
//             // disable alpha-blending
//             mode.set_alpha_blend(rf::ALPHA_BLEND_NONE);
//         }
//         gr_d3d_set_state_and_texture_hook.call_target(mode, bm0, bm1);
//     },
// };

bool gr_is_format_supported(rf::BmFormat format)
{
    if (rf::gr_screen.mode != rf::GR_DIRECT3D) {
        return false;
    }

    auto d3d_fmt = get_d3d_format_from_bm_format(format);
    if (d3d_fmt == D3DFMT_UNKNOWN) {
        return false;
    }

    auto hr = rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat, 0,
        D3DRTYPE_TEXTURE, d3d_fmt);
    if (FAILED(hr)) {
        return false;
    }
    return true;
}

void apply_texture_patches()
{
    gr_d3d_set_texture_data_hook.install();
    gr_d3d_create_vram_texture_hook.install();
    gr_d3d_create_vram_texture_with_mipmaps_pitch_fix.install();
    gr_d3d_get_texel_hook.install();
    gr_d3d_create_texture_hook.install();
    gr_d3d_free_texture_hook.install();
    //gr_d3d_set_state_and_texture_hook.install();
    gr_d3d_lock_hook.install();

    // True Color textures (is it used?)
    if (g_game_config.res_bpp == 32 && g_game_config.true_color_textures) {
        // Available texture formats (tested for compatibility)
        write_mem<int>(0x005A7DFC, D3DFMT_X8R8G8B8); // old: D3DFMT_R5G6B5
        write_mem<int>(0x005A7E00, D3DFMT_A8R8G8B8); // old: D3DFMT_X1R5G5B5
        write_mem<int>(0x005A7E04, D3DFMT_A8R8G8B8); // old: D3DFMT_A1R5G5B5, lightmaps
        write_mem<int>(0x005A7E08, D3DFMT_A8R8G8B8); // old: D3DFMT_A4R4G4B4
        write_mem<int>(0x005A7E0C, D3DFMT_A4R4G4B4); // old: D3DFMT_A8R3G3B2
    }
}

void gr_delete_all_default_pool_textures()
{
    for (auto tslot : g_default_pool_tslots) {
        gr_d3d_free_texture_hook.call_target(*tslot);
    }
    g_default_pool_tslots.clear();
}

void gr_delete_texture(int bm_handle)
{
    if (rf::gr_screen.mode == rf::GR_DIRECT3D) {
        auto bm_index = rf::bm_handle_to_index_anim_aware(bm_handle);
        auto& tslot = rf::gr_d3d_textures[bm_index];
        rf::gr_d3d_free_texture(tslot);
    }
}

void init_supported_texture_formats()
{
    g_texture_format_selector.init();
}

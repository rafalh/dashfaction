#include <windows.h>
#include <d3d8.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <common/utils/string-utils.h>
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_direct3d.h"
#include "../main/main.h"
#include "../bmpman/bmpman.h"
#include "gr_d3d_internal.h"
#include "gr_internal.h"
#include "gr.h"

std::set<rf::gr::d3d::Texture*> g_default_pool_tslots;
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
            auto hr = rf::gr::d3d::d3d->CheckDeviceFormat(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, rf::gr::d3d::pp.BackBufferFormat, 0,
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

    D3DFORMAT select(rf::bm::Format bm_format)
    {
        switch (bm_format) {
            case rf::bm::FORMAT_8_ALPHA:
                return a_8_format_;
            case rf::bm::FORMAT_8_PALETTED:
            case rf::bm::FORMAT_888_RGB:
                return rgb_888_format_;
            case rf::bm::FORMAT_565_RGB:
                return rgb_565_format_;
            case rf::bm::FORMAT_1555_ARGB:
                return argb_1555_format_;
            case rf::bm::FORMAT_8888_ARGB:
                return rgba_8888_format_;
            case rf::bm::FORMAT_4444_ARGB:
                return argb_4444_format_;
            default:
                return get_d3d_format_from_bm_format(bm_format);
        }
    }
};
D3DTextureFormatSelector g_texture_format_selector;

using GrD3DSetTextureData_Type =
    int(int, const std::byte*, const uint8_t*, int, int, rf::bm::Format, rf::gr::d3d::TextureSection*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> gr_d3d_set_texture_data_hook{
    0x0055BA10,
    [](int level, const std::byte* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::bm::Format format, rf::gr::d3d::TextureSection* section, int tex_w, int tex_h, IDirect3DTexture8* texture) {
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
        hr = texture->LockRect(level, &locked_rect, nullptr, 0);
        if (FAILED(hr)) {
            xlog::error("LockRect failed");
            return -1;
        }

        bool success = true;
        if (bm_is_compressed_format(format)) {
            xlog::trace("Writing texture in compressed format level %d src %p bm %dx%d tex %dx%d section %d %d",
                level, src_bits_ptr, bm_w, bm_h, tex_w, tex_h, section->x, section->y);
            auto src_pitch = bm_calculate_pitch(bm_w, format);
            auto num_src_rows = bm_calculate_rows(bm_h, format);

            const auto* src_ptr = src_bits_ptr;
            auto* dst_ptr = static_cast<std::byte*>(locked_rect.pBits);

            src_ptr += src_pitch * bm_calculate_rows(section->y, format);
            src_ptr += bm_calculate_pitch(section->x, format);

            auto copy_pitch = bm_calculate_pitch(section->width, format);

            for (int y = 0; y < num_src_rows; ++y) {
                std::memcpy(dst_ptr, src_ptr, copy_pitch);
                dst_ptr += locked_rect.Pitch;
                src_ptr += src_pitch;
            }
            xlog::trace("Writing completed");
        }
        else {
            auto bm_pitch = bm_bytes_per_pixel(format) * bm_w;
            success = bm_convert_format(locked_rect.pBits, tex_pixel_fmt,
                                                src_bits_ptr, format, bm_w, bm_h, locked_rect.Pitch,
                                                bm_pitch, palette);
            if (!success)
                xlog::warn("Color conversion failed (format %d -> %d)", format, tex_pixel_fmt);
        }

        texture->UnlockRect(level);

        return success ? 0 : -1;
    },
};

FunHook<int(rf::bm::Format, int, int, int, IDirect3DTexture8**)> gr_d3d_create_vram_texture_hook{
    0x0055B970,
    [](rf::bm::Format format, int width, int height, int levels, IDirect3DTexture8** texture_out) {
        xlog::trace("gr_d3d_create_vram_texture_hook");
        D3DFORMAT d3d_format;
        D3DPOOL d3d_pool = D3DPOOL_MANAGED;
        int usage = 0;
        if (format == rf::bm::FORMAT_RENDER_TARGET) {
            xlog::trace("Creating render target texture");
            d3d_format = rf::gr::d3d::pp.BackBufferFormat;
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
        auto hr = rf::gr::d3d::device->CreateTexture(width, height, levels, usage, d3d_format, d3d_pool, texture_out);
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
        auto bm_format = addr_as_ref<rf::bm::Format>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto vram_tex_fmt = g_texture_format_selector.select(bm_format);
        src_bits_ptr += bm_calculate_total_bytes(w, h, bm_format);
        num_total_vram_bytes += bm_calculate_total_bytes(w, h, get_bm_format_from_d3d_format(vram_tex_fmt));
    },
};

FunHook<void(int, float, float, rf::Color*)> gr_d3d_get_texel_hook{
    0x0055CFA0,
    [](int bm_handle, float u, float v, rf::Color* out_color) {
        // This function is only used when shooting at a texture with alpha
        // Note: original function has out of bounds error - it reads color for 16bpp and 32bpp (two addresses)
        // before actually checking the format
        rf::gr::LockInfo lock;
        if (rf::gr::lock(bm_handle, 0, &lock, rf::gr::LOCK_READ_ONLY)) {
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
            int x = std::lround(u * lock.w);
            int y = std::lround(v * lock.h);
            *out_color = bm_get_pixel(lock.data, lock.format, lock.stride_in_bytes, x, y);


            rf::gr::unlock(&lock);
        }
        else {
            out_color->set(255, 255, 255, 255);
        }
    },
};

extern FunHook<int(int, rf::gr::d3d::Texture&)> gr_d3d_create_texture_hook;

int gr_d3d_create_texture(int bm_handle, rf::gr::d3d::Texture& tslot) {
    g_currently_creating_texture_for_bitmap = bm_handle;
    auto result = gr_d3d_create_texture_hook.call_target(bm_handle, tslot);
    g_currently_creating_texture_for_bitmap = -1;

    if (result != 1) {
        xlog::warn("Failed to load texture '%s'", rf::bm::get_filename(bm_handle));
        // Note: callers of this function expects zero result on failure
        return 0;
    }
    auto format = rf::bm::get_format(bm_handle);
    if (format == rf::bm::FORMAT_RENDER_TARGET || (bm_is_dynamic(bm_handle) && gr_d3d_is_d3d8to9())) {
        g_default_pool_tslots.insert(&tslot);
    }
    return result;
}

FunHook<int(int, rf::gr::d3d::Texture&)> gr_d3d_create_texture_hook{0x0055CC00, gr_d3d_create_texture};

FunHook<void(rf::gr::d3d::Texture&)> gr_d3d_free_texture_hook{
    0x0055B640,
    [](rf::gr::d3d::Texture& tslot) {
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

bool gr_d3d_lock(int bm_handle, int section, rf::gr::LockInfo *lock) {
    xlog::trace("gr_d3d_lock");
    auto& tslot = rf::gr::d3d::textures[rf::bm::get_cache_slot(bm_handle)];
    if (tslot.num_sections < 1 || tslot.bm_handle != bm_handle) {
        auto ret = gr_d3d_create_texture(bm_handle, tslot);
        if (ret != 1) {
            return false;
        }
    }
    D3DLOCKED_RECT locked_rect;
    IDirect3DTexture8* d3d_texture = tslot.sections[section].d3d_texture;
    DWORD lock_flags = 0;
    if (lock->mode == rf::gr::LOCK_READ_ONLY) {
        lock_flags = D3DLOCK_READONLY;
    }
    else if (lock->mode == rf::gr::LOCK_WRITE_ONLY && gr_d3d_is_d3d8to9()) {
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

    lock->data = static_cast<rf::ubyte*>(locked_rect.pBits);
    lock->stride_in_bytes = locked_rect.Pitch;
    lock->w = desc.Width;
    lock->h = desc.Height;
    lock->section = section;
    lock->bm_handle = bm_handle;
    lock->format = get_bm_format_from_d3d_format(desc.Format);
    return true;
}

static FunHook<bool(int, int, rf::gr::LockInfo *)> gr_d3d_lock_hook{0x0055CE00, gr_d3d_lock};

// FunHook<void(rf::gr::Mode, int, int)> gr_d3d_set_state_and_texture_hook{
//     0x00550850,
//     [](rf::gr::Mode mode, int bm0, int bm1) {
//         auto bm0_filename = rf::bm::get_filename(bm0);
//         if (bm0_filename && strstr(bm0_filename, "grate")) {
//             // disable alpha-blending
//             mode.set_alpha_blend(rf::ALPHA_BLEND_NONE);
//         }
//         gr_d3d_set_state_and_texture_hook.call_target(mode, bm0, bm1);
//     },
// };

bool gr_d3d_is_texture_format_supported(rf::bm::Format format)
{
    if (rf::gr::screen.mode != rf::gr::DIRECT3D) {
        return false;
    }

    D3DFORMAT d3d_fmt = get_d3d_format_from_bm_format(format);
    if (d3d_fmt == D3DFMT_UNKNOWN) {
        return false;
    }

    HRESULT hr = rf::gr::d3d::d3d->CheckDeviceFormat(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL,
        rf::gr::d3d::pp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, d3d_fmt);
    return SUCCEEDED(hr);
}

void gr_d3d_texture_apply_patch()
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

void gr_d3d_texture_device_lost()
{
    for (auto* tslot : g_default_pool_tslots) {
        gr_d3d_free_texture_hook.call_target(*tslot);
    }
    g_default_pool_tslots.clear();
}

void gr_d3d_delete_texture(int bm_handle)
{
    auto bm_index = rf::bm::get_cache_slot(bm_handle);
    auto& tslot = rf::gr::d3d::textures[bm_index];
    rf::gr::d3d::free_texture(tslot);
}

void gr_d3d_texture_init()
{
    g_texture_format_selector.init();
}

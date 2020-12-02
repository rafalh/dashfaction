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
                return GetD3DFormatFromBmFormat(bm_format);
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

        D3DSURFACE_DESC desc;
        auto hr = texture->GetLevelDesc(level, &desc);
        if (FAILED(hr)) {
            xlog::error("GetLevelDesc failed %lx", hr);
            return -1;
        }

        auto tex_pixel_fmt = GetBmFormatFromD3DFormat(desc.Format);
        if (!BmIsCompressedFormat(tex_pixel_fmt) && GetBmFormatSize(tex_pixel_fmt) == 2) {
            // original code can handle only 16 bit surfaces
            return gr_d3d_set_texture_data_hook.CallTarget(level, src_bits_ptr, palette, bm_w, bm_h, format, section,
                                                           tex_w, tex_h, texture);
        }

        D3DLOCKED_RECT locked_rect;
        hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            xlog::error("LockRect failed");
            return -1;
        }

        bool success = true;
        if (BmIsCompressedFormat(format)) {
            xlog::trace("Writing texture in compressed format level %d src %p bm %dx%d tex %dx%d section %d %d",
                level, src_bits_ptr, bm_w, bm_h, tex_w, tex_h, section->x, section->y);
            auto src_pitch = GetSurfacePitch(bm_w, format);
            auto num_src_rows = GetSurfaceNumRows(bm_h, format);

            auto src_ptr = reinterpret_cast<const std::byte*>(src_bits_ptr);
            auto dst_ptr = reinterpret_cast<std::byte*>(locked_rect.pBits);

            src_ptr += src_pitch * GetSurfaceNumRows(section->y, format);
            src_ptr += GetSurfacePitch(section->x, format);

            auto copy_pitch = GetSurfacePitch(section->width, format);

            for (int y = 0; y < num_src_rows; ++y) {
                std::memcpy(dst_ptr, src_ptr, copy_pitch);
                dst_ptr += locked_rect.Pitch;
                src_ptr += src_pitch;
            }
            xlog::trace("Writing completed");
        }
        else {
            auto tex_pixel_fmt = GetBmFormatFromD3DFormat(desc.Format);
            auto bm_pitch = GetBmFormatSize(format) * bm_w;
            success = ConvertSurfaceFormat(locked_rect.pBits, tex_pixel_fmt,
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
        }

        // TODO: In Direct3D 9 D3DPOOL_DEFAULT + D3DUSAGE_DYNAMIC can be used
        // Note: it has poor performance when texture is locked without D3DLOCK_DISCARD flag

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
        auto bm_format = AddrAsRef<rf::BmFormat>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto vram_tex_fmt = g_texture_format_selector.select(bm_format);
        src_bits_ptr += GetSurfaceLengthInBytes(w, h, bm_format);
        num_total_vram_bytes += GetSurfaceLengthInBytes(w, h, GetBmFormatFromD3DFormat(vram_tex_fmt));
    },
};

int GetBytesPerCompressedBlock(rf::BmFormat format)
{
    switch (format) {
        case rf::BM_FORMAT_DXT1:
            return 8;
        case rf::BM_FORMAT_DXT2:
        case rf::BM_FORMAT_DXT3:
        case rf::BM_FORMAT_DXT4:
        case rf::BM_FORMAT_DXT5:
            return 16;
        default:
            assert(false);
            return 0;
    }
}

FunHook <void(int, float, float, rf::Color*)> gr_d3d_get_texel_hook{
    0x0055CFA0,
    [](int bm_handle, float u, float v, rf::Color* out_color) {
        if (BmIsCompressedFormat(rf::BmGetFormat(bm_handle))) {
            // This function is only used when shooting at a texture with alpha
            rf::GrLockInfo lock;
            if (rf::GrLock(bm_handle, 0, &lock, rf::GR_LOCK_READ_ONLY)) {
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
                constexpr int block_w = 4;
                constexpr int block_h = 4;
                auto bytes_per_block = GetBytesPerCompressedBlock(lock.format);
                auto block = lock.data + (y / block_h) * lock.stride_in_bytes + x / block_w * bytes_per_block;
                *out_color = DecodeBlockCompressedPixel(block, lock.format, x % block_w, y % block_h);
                rf::GrUnlock(&lock);
            }
            else {
                out_color->Set(255, 255, 255, 255);
            }
        }
        else {
            gr_d3d_get_texel_hook.CallTarget(bm_handle, u, v, out_color);
        }
    },
};

extern FunHook<int(int, rf::GrD3DTexture&)> gr_d3d_create_texture_hook;

int gr_d3d_create_texture(int bm_handle, rf::GrD3DTexture& tslot) {
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
}

FunHook<int(int, rf::GrD3DTexture&)> gr_d3d_create_texture_hook{0x0055CC00, gr_d3d_create_texture};

FunHook<void(rf::GrD3DTexture&)> gr_d3d_free_texture_hook{
    0x0055B640,
    [](rf::GrD3DTexture& tslot) {
        gr_d3d_free_texture_hook.CallTarget(tslot);

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
    auto& tslot = rf::gr_d3d_textures[rf::BmHandleToIdxAnimAware(bm_handle)];
    if (tslot.num_sections < 1 || tslot.bm_handle != bm_handle) {
        auto ret = gr_d3d_create_texture(bm_handle, tslot);
        if (ret != 1) {
            return false;
        }
    }
    D3DLOCKED_RECT locked_rect;
    auto d3d_texture = tslot.sections[section].d3d_texture;
    DWORD lock_flags = lock->mode == rf::GR_LOCK_READ_ONLY ? D3DLOCK_READONLY : 0;
    if (FAILED(d3d_texture->LockRect(0, &locked_rect, nullptr, lock_flags))) {
        return false;
    }

    tslot.lock_count++;
    D3DSURFACE_DESC desc;
    d3d_texture->GetLevelDesc(0, &desc);

    lock->data = reinterpret_cast<rf::ubyte*>(locked_rect.pBits);
    lock->stride_in_bytes = locked_rect.Pitch;
    lock->w = desc.Width;
    lock->h = desc.Height;
    lock->section = section;
    lock->bm_handle = bm_handle;
    lock->format = GetBmFormatFromD3DFormat(desc.Format);
    return true;
}

static FunHook<bool(int, int, rf::GrLockInfo *)> gr_d3d_lock_hook{0x0055CE00, gr_d3d_lock};

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

bool GrIsFormatSupported(rf::BmFormat format)
{
    if (rf::gr_screen.mode != rf::GR_DIRECT3D) {
        return false;
    }

    auto d3d_fmt = GetD3DFormatFromBmFormat(format);
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

void ApplyTexturePatches()
{
    gr_d3d_set_texture_data_hook.Install();
    gr_d3d_create_vram_texture_hook.Install();
    gr_d3d_create_vram_texture_with_mipmaps_pitch_fix.Install();
    gr_d3d_get_texel_hook.Install();
    gr_d3d_create_texture_hook.Install();
    gr_d3d_free_texture_hook.Install();
    //gr_d3d_set_state_and_texture_hook.Install();
    gr_d3d_lock_hook.Install();
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
    assert(bm.bm_type == rf::BM_TYPE_USER);
    bm.format = pixel_fmt;
    // TODO: in DX9 use dynamic flag
}

void InitSupportedTextureFormats()
{
    g_texture_format_selector.init();
}

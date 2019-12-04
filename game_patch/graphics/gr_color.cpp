#include "gr_color.h"
#include "../main.h"
#include "../rf.h"
#include "../stdafx.h"
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <algorithm>

inline void ConvertPixel_RGB8_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr)
{
    *(dst_ptr++) = *(src_ptr++);
    *(dst_ptr++) = *(src_ptr++);
    *(dst_ptr++) = *(src_ptr++);
    *(dst_ptr++) = 255;
}

inline void ConvertPixel_BGR8_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr)
{
    uint8_t r = *(src_ptr++), g = *(src_ptr++), b = *(src_ptr++);
    *(dst_ptr++) = b;
    *(dst_ptr++) = g;
    *(dst_ptr++) = r;
    *(dst_ptr++) = 255;
}

inline void ConvertPixel_RGBA4_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr)
{
    *(dst_ptr++) = (*(src_ptr)&0x0F) * 17;
    *(dst_ptr++) = ((*(src_ptr++) & 0xF0) >> 4) * 17;
    *(dst_ptr++) = (*(src_ptr)&0x0F) * 17;
    *(dst_ptr++) = ((*(src_ptr++) & 0xF0) >> 4) * 17;
}

inline void ConvertPixel_ARGB1555_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr)
{
    uint16_t src_word = *reinterpret_cast<const uint16_t*>(src_ptr);
    src_ptr += 2;
    *(dst_ptr++) = ((src_word & (0x1F << 0)) >> 0) * 255 / 31;
    *(dst_ptr++) = ((src_word & (0x1F << 5)) >> 5) * 255 / 31;
    *(dst_ptr++) = ((src_word & (0x1F << 10)) >> 10) * 255 / 31;
    *(dst_ptr++) = (src_word & 0x8000) ? 255 : 0;
}

inline void ConvertPixel_RGB565_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr)
{
    uint16_t src_word = *reinterpret_cast<const uint16_t*>(src_ptr);
    src_ptr += 2;
    *(dst_ptr++) = ((src_word & (0x1F << 0)) >> 0) * 255 / 31;
    *(dst_ptr++) = ((src_word & (0x3F << 5)) >> 5) * 255 / 63;
    *(dst_ptr++) = ((src_word & (0x1F << 11)) >> 11) * 255 / 31;
    *(dst_ptr++) = 255;
}

inline void ConvertPixel_MONO8_To_RGBA8(uint8_t*& dst_ptr, const uint8_t*& src_ptr, const uint8_t* palette)
{
    const auto& clr = &palette[3 * *(src_ptr++)];
    *(dst_ptr++) = clr[0]; // B
    *(dst_ptr++) = clr[1]; // G
    *(dst_ptr++) = clr[2]; // R
    *(dst_ptr++) = 255; // A
}

bool ConvertPixelFormat(uint8_t*& dst_ptr, rf::BmPixelFormat dst_fmt, const uint8_t*& src_ptr,
                        rf::BmPixelFormat src_fmt, const uint8_t* palette)
{
    if (dst_fmt == src_fmt) {
        int pixel_size = GetPixelFormatSize(src_fmt);
        memcpy(dst_ptr, src_ptr, pixel_size);
        dst_ptr += pixel_size;
        src_ptr += pixel_size;
        return true;
    }
    if (dst_fmt != rf::BMPF_8888) {
        ERR("unsupported dest pixel format %d (ConvertPixelFormat)", dst_fmt);
        return false;
    }
    switch (src_fmt) {
    case rf::BMPF_888:
        ConvertPixel_RGB8_To_RGBA8(dst_ptr, src_ptr);
        return true;
    case rf::BMPF_4444:
        ConvertPixel_RGBA4_To_RGBA8(dst_ptr, src_ptr);
        return true;
    case rf::BMPF_565:
        ConvertPixel_RGB565_To_RGBA8(dst_ptr, src_ptr);
        return true;
    case rf::BMPF_1555:
        ConvertPixel_ARGB1555_To_RGBA8(dst_ptr, src_ptr);
        return true;
    case rf::BMPF_MONO8:
        if (!palette) {
            return false;
        }
        ConvertPixel_MONO8_To_RGBA8(dst_ptr, src_ptr, palette);
        return true;
    default:
        ERR("unsupported src pixel format %d", src_fmt);
        return false;
    }
}

bool ConvertBitmapFormat(uint8_t* dst_bits_ptr, rf::BmPixelFormat dst_fmt, const uint8_t* src_bits_ptr,
                         rf::BmPixelFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                         const uint8_t* palette = nullptr, bool swap_bytes = false)
{
    if (dst_fmt == src_fmt) {
        for (int y = 0; y < height; ++y) {
            memcpy(dst_bits_ptr, src_bits_ptr, std::min(src_pitch, dst_pitch));
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    }
    if (dst_fmt != rf::BMPF_8888)
        return false;
    switch (src_fmt) {
    case rf::BMPF_888:
        for (int y = 0; y < height; ++y) {
            uint8_t* dst_ptr = dst_bits_ptr;
            const uint8_t* src_ptr = src_bits_ptr;
            for (int x = 0; x < width; ++x) {
                if (swap_bytes)
                    ConvertPixel_BGR8_To_RGBA8(dst_ptr, src_ptr);
                else
                    ConvertPixel_RGB8_To_RGBA8(dst_ptr, src_ptr);
            }
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    case rf::BMPF_4444:
        for (int y = 0; y < height; ++y) {
            uint8_t* dst_ptr = dst_bits_ptr;
            const uint8_t* src_ptr = src_bits_ptr;
            for (int x = 0; x < width; ++x) ConvertPixel_RGBA4_To_RGBA8(dst_ptr, src_ptr);
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    case rf::BMPF_1555:
        for (int y = 0; y < height; ++y) {
            uint8_t* dst_ptr = dst_bits_ptr;
            const uint8_t* src_ptr = src_bits_ptr;
            for (int x = 0; x < width; ++x) ConvertPixel_ARGB1555_To_RGBA8(dst_ptr, src_ptr);
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    case rf::BMPF_565:
        for (int y = 0; y < height; ++y) {
            uint8_t* dst_ptr = dst_bits_ptr;
            const uint8_t* src_ptr = src_bits_ptr;
            for (int x = 0; x < width; ++x) ConvertPixel_RGB565_To_RGBA8(dst_ptr, src_ptr);
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    case rf::BMPF_MONO8:
        if (!palette) {
            return false;
        }
        for (int y = 0; y < height; ++y) {
            uint8_t* dst_ptr = dst_bits_ptr;
            const uint8_t* src_ptr = src_bits_ptr;
            for (int x = 0; x < width; ++x) ConvertPixel_MONO8_To_RGBA8(dst_ptr, src_ptr, palette);
            dst_bits_ptr += dst_pitch;
            src_bits_ptr += src_pitch;
        }
        return true;
    default:
        ERR("unsupported src format %d", src_fmt);
        return false;
    }
}

using GrD3DSetTextureData_Type =
    int(int, const uint8_t*, const uint8_t*, int, int, rf::BmPixelFormat, void*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> GrD3DSetTextureData_hook{
    0x0055BA10,
    [](int level, const uint8_t* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::BmPixelFormat pixel_fmt, void* a7, int tex_w, int tex_h, IDirect3DTexture8* texture) {

        D3DLOCKED_RECT locked_rect;
        HRESULT hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            ERR("LockRect failed");
            return -1;
        }
        D3DSURFACE_DESC desc;
        texture->GetLevelDesc(level, &desc);
        auto tex_pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
        bool success = ConvertBitmapFormat(reinterpret_cast<uint8_t*>(locked_rect.pBits), tex_pixel_fmt,
                                            src_bits_ptr, pixel_fmt, bm_w, bm_h, locked_rect.Pitch,
                                            GetPixelFormatSize(pixel_fmt) * bm_w, palette);
        texture->UnlockRect(level);

        if (success)
            return 0;

        WARN("Color conversion failed (format %d -> %d)", pixel_fmt, tex_pixel_fmt);
        return GrD3DSetTextureData_hook.CallTarget(level, src_bits_ptr, palette, bm_w, bm_h, pixel_fmt, a7, tex_w,
                                                    tex_h, texture);
    },
};

CodeInjection RflLoadLightmaps_color_conv_patch{
    0x004ED3E9,
    [](auto& regs) {
        // Always skip original code
        regs.eip = 0x004ED4FA;

        auto lightmap = reinterpret_cast<rf::RflLightmap*>(regs.ebx);

        rf::GrLockData lock_data;
        if (!rf::GrLock(lightmap->bm_handle, 0, &lock_data, 2))
            return;

    #if 1 // cap minimal color channel value as RF does
        for (int i = 0; i < lightmap->w * lightmap->h * 3; ++i)
            lightmap->buf[i] = std::max(lightmap->buf[i], (uint8_t)(4 << 3)); // 32
    #endif

        bool success = ConvertBitmapFormat(lock_data.bits, lock_data.pixel_format, lightmap->buf, rf::BMPF_888, lightmap->w,
                                           lightmap->h, lock_data.pitch, 3 * lightmap->w, nullptr, true);
        if (!success)
            ERR("ConvertBitmapFormat failed for lightmap");

        rf::GrUnlock(&lock_data);
    },
};

CodeInjection GeoModGenerateTexture_color_conv_patch{
    0x004F2F23,
    [](auto& regs) {
        // Always skip original code
        regs.eip = 0x004F3023;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::RflLightmap& lightmap = *StructFieldRef<rf::RflLightmap*>(face_light_info, 12);
        rf::GrLockData lock_data;
        if (!rf::GrLock(lightmap.bm_handle, 0, &lock_data, 1)) {
            return;
        }

        int offset_y = StructFieldRef<int>(face_light_info, 20);
        int offset_x = StructFieldRef<int>(face_light_info, 16);
        int src_width = lightmap.w;
        int dst_pixel_size = GetPixelFormatSize(lock_data.pixel_format);
        uint8_t* src_data = lightmap.buf + 3 * (offset_x + offset_y * src_width);
        uint8_t* dst_data = &lock_data.bits[dst_pixel_size * offset_x + offset_y * lock_data.pitch];
        int height = StructFieldRef<int>(face_light_info, 28);
        int src_pitch = 3 * src_width;
        bool success = ConvertBitmapFormat(dst_data, lock_data.pixel_format, src_data, rf::BMPF_888, src_width, height,
                                           lock_data.pitch, src_pitch);
        if (!success)
            ERR("ConvertBitmapFormat failed for geomod (fmt %d)", lock_data.pixel_format);
        rf::GrUnlock(&lock_data);
    },
};

CodeInjection GeoModGenerateLightmap_color_conv_patch{
    0x004E487B,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E4993;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::RflLightmap& lightmap = *StructFieldRef<rf::RflLightmap*>(face_light_info, 12);
        rf::GrLockData lock_data;
        if (!rf::GrLock(lightmap.bm_handle, 0, &lock_data, 1)) {
            return;
        }

        int offset_y = StructFieldRef<int>(face_light_info, 20);
        int src_width = lightmap.w;
        int offset_x = StructFieldRef<int>(face_light_info, 16);
        uint8_t* src_data_begin = lightmap.buf;
        int src_offset = 3 * (offset_x + src_width * StructFieldRef<int>(face_light_info, 20)); // src offset
        uint8_t* src_data = src_offset + src_data_begin;
        int height = StructFieldRef<int>(face_light_info, 28);
        int dst_pixel_size = GetPixelFormatSize(lock_data.pixel_format);
        uint8_t* dst_row_ptr = &lock_data.bits[dst_pixel_size * offset_x + offset_y * lock_data.pitch];
        int src_pitch = 3 * src_width;
        bool success = ConvertBitmapFormat(dst_row_ptr, lock_data.pixel_format, src_data, rf::BMPF_888, src_width,
                                           height, lock_data.pitch, src_pitch);
        if (!success)
            ERR("ConvertBitmapFormat failed for geomod2 (fmt %d)", lock_data.pixel_format);
        rf::GrUnlock(&lock_data);
    },
};

CodeInjection WaterGenerateTexture_color_conv_patch{
    0x004E68D1,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E6B68;

        uintptr_t waveform_info = static_cast<uintptr_t>(regs.esi);
        int src_bm_handle = *reinterpret_cast<int*>(waveform_info + 36);
        rf::GrLockData src_lock_data, dst_lock_data;
        if (!rf::GrLock(src_bm_handle, 0, &src_lock_data, 0)) {
            return;
        }
        int dst_bm_handle = *reinterpret_cast<int*>(waveform_info + 24);
        if (!rf::GrLock(dst_bm_handle, 0, &dst_lock_data, 2)) {
            rf::GrUnlock(&src_lock_data);
            return;
        }

        auto& byte_1370f90 = AddrAsRef<uint8_t[256]>(0x1370F90);
        auto& byte_1371b14 = AddrAsRef<uint8_t[256]>(0x1371B14);
        auto& byte_1371090 = AddrAsRef<uint8_t[512]>(0x1371090);

        uint8_t* dst_row_ptr = dst_lock_data.bits;
        int src_pixel_size = GetPixelFormatSize(src_lock_data.pixel_format);

        for (int y = 0; y < dst_lock_data.height; ++y) {
            int t1 = byte_1370f90[y];
            int t2 = byte_1371b14[y];
            uint8_t* off_arr = &byte_1371090[-t1];
            uint8_t* dst_ptr = dst_row_ptr;
            for (int x = 0; x < dst_lock_data.width; ++x) {
                int src_x = t1;
                int src_y = t2 + off_arr[t1];
                int src_x_limited = src_x & (dst_lock_data.width - 1);
                int src_y_limited = src_y & (dst_lock_data.height - 1);
                const uint8_t* src_ptr = src_lock_data.bits + src_x_limited * src_pixel_size + src_y_limited * src_lock_data.pitch;
                // Note: GrLock never returns indexed bitmap
                ConvertPixelFormat(dst_ptr, dst_lock_data.pixel_format, src_ptr, src_lock_data.pixel_format, nullptr);
                ++t1;
            }
            dst_row_ptr += dst_lock_data.pitch;
        }

        rf::GrUnlock(&src_lock_data);
        rf::GrUnlock(&dst_lock_data);
    }
};

CodeInjection GetAmbientColorFromLightmaps_color_conv_patch{
    0x004E5CE3,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E5D57;

        int bm_handle = regs.eax;
        int x = regs.edi;
        int y = regs.ebx;
        auto& color = *reinterpret_cast<uint32_t*>(regs.esp + 0x34 - 0x28);

        rf::GrLockData lock_data;
        if (rf::GrLock(bm_handle, 0, &lock_data, 0)) {
            auto src_bytes_per_pixel = GetPixelFormatSize(lock_data.pixel_format);
            const uint8_t* src_ptr = lock_data.bits + y * lock_data.pitch + x * src_bytes_per_pixel;
            auto dst_ptr = reinterpret_cast<uint8_t*>(&color);
            // Note: GrLock never returns indexed bitmap
            ConvertPixelFormat(dst_ptr, rf::BMPF_8888, src_ptr, lock_data.pixel_format, nullptr);
            rf::GrUnlock(&lock_data);
        }
    },
};

FunHook<unsigned()> BinkInitDeviceInfo_hook{
    0x005210C0,
    []() {
        unsigned bink_flags = BinkInitDeviceInfo_hook.CallTarget();

        if (g_game_config.true_color_textures && g_game_config.res_bpp == 32) {
            static auto& bink_bm_pixel_fmt = AddrAsRef<uint32_t>(0x018871C0);
            bink_bm_pixel_fmt = rf::BMPF_888;
            bink_flags = 3;
        }

        return bink_flags;
    },
};

CodeInjection MonitorRenderNoise_patch{
    0x004123AD,
    [](auto& regs) {
        // Note: default noise generation algohritm is not used because it's not uniform enough in high resolution
        static int noise_buf;
        if (regs.edx % 15 == 0)
            noise_buf = std::rand();
        bool white = noise_buf & 1;
        noise_buf >>= 1;

        auto& lock = *reinterpret_cast<rf::GrLockData*>(regs.esp + 0x2C - 0x20);
        if (GetPixelFormatSize(lock.pixel_format) == 4) {
            *reinterpret_cast<int32_t*>(regs.esi) = white ? 0 : -1;
            regs.esi += 4;
        }
        else {
            *reinterpret_cast<int16_t*>(regs.esi) = white ? 0 : -1;
            regs.esi += 2;
        }
        ++regs.edx;
        regs.eip = 0x004123DA;
    },
};

CodeInjection MonitorRenderOffState_patch{
    0x00412430,
    [](auto& regs) {
        auto& lock_data = *reinterpret_cast<rf::GrLockData*>(regs.esp + 0x34 - 0x20);
        regs.ecx = lock_data.height * lock_data.pitch;
        regs.eip = 0x0041243F;
    },
};

void GrColorInit()
{
    // True Color textures
    if (g_game_config.res_bpp == 32 && g_game_config.true_color_textures) {
        // Available texture formats (tested for compatibility)
        WriteMem<u32>(0x005A7DFC, D3DFMT_X8R8G8B8); // old: D3DFMT_R5G6B5
        WriteMem<u32>(0x005A7E00, D3DFMT_A8R8G8B8); // old: D3DFMT_X1R5G5B5
        WriteMem<u32>(0x005A7E04, D3DFMT_A8R8G8B8); // old: D3DFMT_A1R5G5B5, lightmaps
        WriteMem<u32>(0x005A7E08, D3DFMT_A8R8G8B8); // old: D3DFMT_A4R4G4B4
        WriteMem<u32>(0x005A7E0C, D3DFMT_A4R4G4B4); // old: D3DFMT_A8R3G3B2

        GrD3DSetTextureData_hook.Install();
        BinkInitDeviceInfo_hook.Install();

        // lightmaps
        RflLoadLightmaps_color_conv_patch.Install();
        // geomod
        GeoModGenerateTexture_color_conv_patch.Install();
        GeoModGenerateLightmap_color_conv_patch.Install();
        // water
        AsmWriter(0x004E68B0, 0x004E68B6).nop();
        WaterGenerateTexture_color_conv_patch.Install();
        // ambient color
        GetAmbientColorFromLightmaps_color_conv_patch.Install();
        // monitor noise
        MonitorRenderNoise_patch.Install();
        // monitor off state
        MonitorRenderOffState_patch.Install();
    }
}

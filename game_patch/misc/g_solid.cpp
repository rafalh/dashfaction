#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../rf/geometry.h"
#include "../rf/graphics.h"
#include "../graphics/gr_color.h"
#include "../graphics/graphics.h"

constexpr auto reference_fps = 30.0f;
constexpr auto reference_framerate = 1.0f / reference_fps;

FunHook<int(rf::GSolid*, rf::GRoom*)> geo_cache_prepare_room_hook{
    0x004F0C00,
    [](rf::GSolid* solid, rf::GRoom* room) {
        int ret = geo_cache_prepare_room_hook.call_target(solid, room);
        std::byte** pp_room_geom = (std::byte**)(reinterpret_cast<std::byte*>(room) + 4);
        std::byte* room_geom = *pp_room_geom;
        if (ret == 0 && room_geom) {
            uint32_t room_vert_num = *reinterpret_cast<uint32_t*>(room_geom + 4);
            if (room_vert_num > 8000) {
                static int once = 0;
                if (!(once++))
                    xlog::warn("Not rendering room with %u vertices!", room_vert_num);
                *pp_room_geom = nullptr;
                return -1;
            }
        }
        return ret;
    },
};

CodeInjection GSurface_calculate_lightmap_color_conv_patch{
    0x004F2F23,
    [](auto& regs) {
        // Always skip original code
        regs.eip = 0x004F3023;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::GLightmap& lightmap = *struct_field_ref<rf::GLightmap*>(face_light_info, 12);
        rf::GrLockInfo lock;
        if (!rf::gr_lock(lightmap.bm_handle, 0, &lock, rf::GR_LOCK_WRITE_ONLY)) {
            return;
        }

        int offset_y = struct_field_ref<int>(face_light_info, 20);
        int offset_x = struct_field_ref<int>(face_light_info, 16);
        int src_width = lightmap.w;
        int dst_pixel_size = bm_bytes_per_pixel(lock.format);
        uint8_t* src_data = lightmap.buf + 3 * (offset_x + offset_y * src_width);
        uint8_t* dst_data = &lock.data[dst_pixel_size * offset_x + offset_y * lock.stride_in_bytes];
        int height = struct_field_ref<int>(face_light_info, 28);
        int src_pitch = 3 * src_width;
        bool success = convert_surface_format(dst_data, lock.format, src_data, rf::BM_FORMAT_888_BGR,
            src_width, height, lock.stride_in_bytes, src_pitch);
        if (!success)
            xlog::error("convert_surface_format failed for geomod (fmt %d)", lock.format);
        rf::gr_unlock(&lock);
    },
};

CodeInjection GSurface_alloc_lightmap_color_conv_patch{
    0x004E487B,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E4993;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::GLightmap& lightmap = *struct_field_ref<rf::GLightmap*>(face_light_info, 12);
        rf::GrLockInfo lock;
        if (!rf::gr_lock(lightmap.bm_handle, 0, &lock, rf::GR_LOCK_WRITE_ONLY)) {
            return;
        }

        int offset_y = struct_field_ref<int>(face_light_info, 20);
        int src_width = lightmap.w;
        int offset_x = struct_field_ref<int>(face_light_info, 16);
        uint8_t* src_data_begin = lightmap.buf;
        int src_offset = 3 * (offset_x + src_width * struct_field_ref<int>(face_light_info, 20)); // src offset
        uint8_t* src_data = src_offset + src_data_begin;
        int height = struct_field_ref<int>(face_light_info, 28);
        int dst_pixel_size = bm_bytes_per_pixel(lock.format);
        uint8_t* dst_row_ptr = &lock.data[dst_pixel_size * offset_x + offset_y * lock.stride_in_bytes];
        int src_pitch = 3 * src_width;
        bool success = convert_surface_format(dst_row_ptr, lock.format, src_data, rf::BM_FORMAT_888_BGR,
                                                 src_width, height, lock.stride_in_bytes, src_pitch);
        if (!success)
            xlog::error("ConvertBitmapFormat failed for geomod2 (fmt %d)", lock.format);
        rf::gr_unlock(&lock);
    },
};

CodeInjection GSolid_get_ambient_color_from_lightmap_patch{
    0x004E5CE3,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E5D57;

        int x = regs.edi;
        int y = regs.ebx;
        auto& lm = *reinterpret_cast<rf::GLightmap*>(regs.esi);
        auto& color = *reinterpret_cast<rf::Color*>(regs.esp + 0x34 - 0x28);

        // Optimization: instead of locking the lightmap texture get color data from lightmap pixels stored in RAM
        const uint8_t* src_ptr = lm.buf + (y * lm.w + x) * 3;
        color.set(src_ptr[0], src_ptr[1], src_ptr[2], 255);
    },
};

CodeInjection g_proctex_update_water_patch{
    0x004E68D1,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E6B68;

        uintptr_t waveform_info = static_cast<uintptr_t>(regs.esi);
        int src_bm_handle = *reinterpret_cast<int*>(waveform_info + 36);
        rf::GrLockInfo src_lock, dst_lock;
        if (!rf::gr_lock(src_bm_handle, 0, &src_lock, rf::GR_LOCK_READ_ONLY)) {
            return;
        }
        int dst_bm_handle = *reinterpret_cast<int*>(waveform_info + 24);
        bm_set_dynamic(dst_bm_handle, true);
        if (!rf::gr_lock(dst_bm_handle, 0, &dst_lock, rf::GR_LOCK_WRITE_ONLY)) {
            rf::gr_unlock(&src_lock);
            return;
        }

        gr_copy_water_bitmap(src_lock, dst_lock);

        rf::gr_unlock(&src_lock);
        rf::gr_unlock(&dst_lock);
    }
};

CodeInjection g_proctex_create_bm_create_injection{
    0x004E66A2,
    [](auto& regs) {
        bm_set_dynamic(regs.eax, true);
    },
};

CodeInjection face_scroll_fix{
    0x004EE1D6,
    [](auto& regs) {
        auto geometry = reinterpret_cast<void*>(regs.ebp);
        auto& scroll_data_vec = struct_field_ref<rf::VArray<void*>>(geometry, 0x2F4);
        auto GTextureMover_setup_faces = reinterpret_cast<void(__thiscall*)(void* self, void* geometry)>(0x004E60C0);
        for (int i = 0; i < scroll_data_vec.size(); ++i) {
            GTextureMover_setup_faces(scroll_data_vec[i], geometry);
        }
    },
};

CodeInjection g_proctex_update_water_speed_fix{
    0x004E68A0,
    [](auto& regs) {
        rf::Vector3& result = *reinterpret_cast<rf::Vector3*>(regs.esi + 0x2C);
        result.x += 12.8f * (rf::frametime) / reference_framerate;
        result.y += 4.2666669f * (rf::frametime) / reference_framerate;
        result.z += 3.878788f * (rf::frametime) / reference_framerate;
    },
};

void g_solid_do_patch()
{
    // Buffer overflows in solid_read
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important bm_load copies texture name to 32 bytes long buffers
    write_mem<i8>(0x004ED612 + 1, 32);
    write_mem<i8>(0x004ED66E + 1, 32);
    write_mem<i8>(0x004ED72E + 1, 32);
    write_mem<i8>(0x004EDB02 + 1, 32);

    // Fix crash in geometry rendering
    geo_cache_prepare_room_hook.install();

    // 32-bit color format - geomod
    GSurface_calculate_lightmap_color_conv_patch.install();
    GSurface_alloc_lightmap_color_conv_patch.install();
    // 32-bit color format - ambient color
    GSolid_get_ambient_color_from_lightmap_patch.install();
    // water
    AsmWriter(0x004E68B0, 0x004E68B6).nop();
    g_proctex_update_water_patch.install();

    // Fix face scroll in levels after version 0xB4
    face_scroll_fix.install();

    // Fix water waves animation on high FPS
    AsmWriter(0x004E68A0, 0x004E68A9).nop();
    AsmWriter(0x004E68B6, 0x004E68D1).nop();
    g_proctex_update_water_speed_fix.install();

    // Set dynamic flag on proctex texture
    g_proctex_create_bm_create_injection.install();
}

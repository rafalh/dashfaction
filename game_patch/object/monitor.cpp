#include <cstring>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include <common/utils/list-utils.h>
#include "../rf/clutter.h"
#include "../rf/gr/gr.h"
#include "../rf/bmpman.h"
#include "../rf/player/player.h"
#include "../main/main.h"
#include "../graphics/gr.h"
#include "../bmpman/bmpman.h"

FunHook<char(int, int, int, int, char)> monitor_create_hook{
    0x00412470,
    [](int clutter_handle, int always_minus_1, int w, int h, char always_1) {
        if (g_game_config.high_monitor_res) {
            constexpr int factor = 2;
            w *= factor;
            h *= factor;
        }
        return monitor_create_hook.call_target(clutter_handle, always_minus_1, w, h, always_1);
    },
};

void monitor_refresh_all()
{
    auto mon_list = DoublyLinkedList{rf::monitor_list};
    for (auto& mon : mon_list) {
        // force rerender of this monitor in case it is not updated every frame
        // this is needed because DF uses default pool texture
        mon.flags &= ~rf::MF_BM_RENDERED;
    }
}

CallHook<int(rf::bm::Format, int, int)> bm_create_user_bitmap_monitor_hook{
    0x00412547,
    []([[ maybe_unused ]] rf::bm::Format pixel_fmt, int w, int h) {
        return bm_create_user_bitmap_monitor_hook.call_target(rf::bm::FORMAT_RENDER_TARGET, w, h);
    },
};

void ensure_monitor_bitmap_is_dynamic(rf::Monitor& mon)
{
    if (rf::bm::get_format(mon.user_bitmap) == rf::bm::FORMAT_RENDER_TARGET) {
        xlog::trace("Changing pixel format for monitor bitmap");
        bm_change_format(mon.user_bitmap, rf::bm::FORMAT_888_RGB);
        bm_set_dynamic(mon.user_bitmap, true);
    }
}

#if 1
inline bool generate_static_noise()
{
    static int noise_buf;
    static int counter = 0;
    if (counter == 0)
        noise_buf = std::rand();
    bool white = noise_buf & 1;
    noise_buf >>= 1;
    // assume rand() returns 15-bit long random number
    counter = (counter + 1) % 15;
    return white;
}

FunHook<void(rf::Monitor&)> monitor_update_static_hook{
    0x00412370,
    [](rf::Monitor& mon) {
        // No longer use render target texture
        ensure_monitor_bitmap_is_dynamic(mon);
        // make sure alpha component in 32-bit formats is set to 255
        rf::ubyte black[4] = {0, 0, 0, 255};
        rf::ubyte white[4] = {255, 255, 255, 255};
        // Use custom noise generation algohritm because the default one is not uniform enough in high resolution
        rf::gr::LockInfo lock;
        if (rf::gr::lock(mon.user_bitmap, 0, &lock, rf::gr::LOCK_WRITE_ONLY)) {
            auto pixel_size = bm_bytes_per_pixel(lock.format);
            for (int y = 0; y < lock.h; ++y) {
                auto* ptr = lock.data + y * lock.stride_in_bytes;
                for (int x = 0; x < lock.w; ++x) {
                    bool is_white = generate_static_noise();
                    // support 32-bit textures
                    std::memcpy(ptr, is_white ? white : black, pixel_size);
                    ptr += pixel_size;
                }
            }
            rf::gr::unlock(&lock);
        }
        mon.flags |= rf::MF_BM_RENDERED;
    },
};
#else
void replace_monitor_screen_bitmap(rf::Monitor& mon, int hbm)
{
    auto clutter = rf::obj_from_handle(mon.clutter_handle);
    auto vmesh = clutter->vmesh;
    int num_materials;
    rf::MeshMaterial *materials;
    rf::vmesh_get_materials_array(vmesh, &num_materials, &materials);
    for (int i = 0; i < num_materials; ++i) {
        auto& mat = materials[i];
        if (string_equals_ignore_case(mat.name, "screen.tga")) {
            mat.texture = hbm;
            *mat.mat_field_20 = 0.0f;
        }
    }
}

FunHook<void(rf::Monitor&)> monitor_update_static_hook{
    0x00412370,
    [](rf::Monitor& mon) {
        // monitor is no longer displaying view from camera so its texture usage must be changed
        // from render target to dynamic texture
        //ensure_monitor_bitmap_is_dynamic(mon);
        if (!(mon.flags & rf::MF_BM_RENDERED)) {
            // very good idea but needs more work on UVs...
            auto hbm = rf::bm::load("gls_noise01.tga", -1, true);
            replace_monitor_screen_bitmap(mon, hbm);
            mon.flags |= rf::MF_BM_RENDERED;
        }
        //monitor_update_static_hook.call_target(mon);
    },
};
#endif

CodeInjection monitor_update_from_camera_begin_render_to_texture{
    0x00412860,
    [](auto& regs) {
        rf::Monitor* mon = regs.edi;
        gr_set_render_target(mon->user_bitmap);
    },
};

FunHook<void(rf::Monitor&)> monitor_update_off_hook{
    0x00412410,
    [](rf::Monitor& mon) {
        if (mon.flags & rf::MF_BM_RENDERED) {
            return;
        }
        // monitor is no longer displaying view from camera so its texture usage must be changed
        // from render target to dynamic texture
        ensure_monitor_bitmap_is_dynamic(mon);
        // Add support for 32-bit textures
        rf::gr::LockInfo lock;
        if (rf::gr::lock(mon.user_bitmap, 0, &lock, rf::gr::LOCK_WRITE_ONLY)) {
            std::memset(lock.data, 0, lock.h * lock.stride_in_bytes);
            rf::gr::unlock(&lock);
        }
    },
};

CodeInjection render_corpse_in_monitor_patch{
    0x00412905,
    []() {
        rf::player_render_held_corpse(rf::local_player);
    },
};

void monitor_do_patch()
{
    // High monitors/mirrors resolution
    monitor_create_hook.install();

    // Make sure bitmaps used together with gr_capture_back_buffer have the same format as backbuffer
    bm_create_user_bitmap_monitor_hook.install();
    monitor_update_off_hook.install();
    monitor_update_static_hook.install();

    // Use render to texture approach for monitors
    monitor_update_from_camera_begin_render_to_texture.install();

    // Render held corpse in monitor
    render_corpse_in_monitor_patch.install();
}

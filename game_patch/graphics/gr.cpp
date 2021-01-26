#include <algorithm>
#include <common/error/d3d-error.h>
#include <common/utils/string-utils.h>
#include <common/utils/list-utils.h>
#include <common/utils/os-utils.h>
#include <common/config/BuildConfig.h>
#include <patch_common/ComPtr.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include "gr.h"
#include "gr_internal.h"
#include "../os/console.h"
#include "../hud/hud.h"
#include "../main/main.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/player.h"
#include "../rf/multi.h"
#include "../rf/hud.h"
#include "../rf/gameseq.h"
#include "../rf/os/os.h"
#include "../rf/item.h"
#include "../rf/clutter.h"

constexpr int min_fov = 75;
constexpr int max_fov = 120;

CodeInjection setup_stretched_window_patch{
    0x0050C464,
    [](auto& regs) {
        if (g_game_config.wnd_mode == GameConfig::STRETCHED) {
            // Make sure stretched window is always full screen
            auto cx = GetSystemMetrics(SM_CXSCREEN);
            auto cy = GetSystemMetrics(SM_CYSCREEN);
            SetWindowLongA(rf::main_wnd, GWL_STYLE, WS_POPUP | WS_SYSMENU);
            SetWindowLongA(rf::main_wnd, GWL_EXSTYLE, 0);
            SetWindowPos(rf::main_wnd, HWND_NOTOPMOST, 0, 0, cx, cy, SWP_SHOWWINDOW);
            rf::gr_screen.aspect = static_cast<float>(cx) / static_cast<float>(cy) * 0.75f;
            regs.eip = 0x0050C551;
        }
    },
};

float gr_scale_horz_fov(float horizontal_fov = 90.0f)
{
    if (g_game_config.horz_fov >= min_fov && g_game_config.horz_fov <= max_fov) {
        // Use user provided factor
        // Note: 90 is the default FOV for RF
        return horizontal_fov * g_game_config.horz_fov / 90.0f;
    }
    else {
        // Use Hor+ FOV scaling method to improve user experience for wide screens
        // Assume provided FOV makes sense on a 4:3 screen
        float s = static_cast<float>(rf::gr_screen.max_w) / rf::gr_screen.max_h * 0.75f;
        constexpr float pi = 3.141592f;
        float h_fov_rad = horizontal_fov / 180.0f * pi;
        float x = std::tan(h_fov_rad / 2.0f);
        float y = x * s;
        h_fov_rad = 2.0f * std::atan(y);
        horizontal_fov = h_fov_rad / pi * 180.0f;
        // Clamp the value to avoid artifacts when the view is very stretched
        horizontal_fov = std::min<float>(horizontal_fov, max_fov);
        return horizontal_fov;
    }
}

FunHook<void(rf::Matrix3&, rf::Vector3&, float, bool, bool)> gr_setup_3d_hook{
    0x00517EB0,
    [](rf::Matrix3& viewer_orient, rf::Vector3& viewer_pos, float horizontal_fov, bool zbuffer_flag, bool z_scale) {
        rf::gr_screen.aspect = 1.0f;
        horizontal_fov = gr_scale_horz_fov(horizontal_fov);
        gr_setup_3d_hook.call_target(viewer_orient, viewer_pos, horizontal_fov, zbuffer_flag, z_scale);
    },
};

ConsoleCommand2 fov_cmd{
    "fov",
    [](std::optional<int> fov_opt) {
        if (fov_opt) {
            if (fov_opt.value() <= 0) {
                g_game_config.horz_fov = 0;
            }
            else {
                g_game_config.horz_fov = std::clamp(fov_opt.value(), min_fov, max_fov);
            }
            g_game_config.save();
        }
        rf::console_printf("Horizontal FOV: %.0f", gr_scale_horz_fov());
    },
    "Sets horizontal FOV (field of view) in degrees. Use 0 to enable automatic FOV scaling.",
    "fov [degrees]",
};

CallHook<void(int, rf::Vertex**, int, rf::GrMode)> gr_rect_gr_tmapper_hook{
    0x0050DD69,
    [](int nv, rf::Vertex** verts, int flags, rf::GrMode mode) {
        for (int i = 0; i < nv; ++i) {
            verts[i]->sx -= 0.5f;
            verts[i]->sy -= 0.5f;
        }
        gr_rect_gr_tmapper_hook.call_target(nv, verts, flags, mode);
    },
};

FunHook<void()> do_damage_screen_flash_hook{
    0x004A7520,
    []() {
        if (g_game_config.damage_screen_flash) {
            do_damage_screen_flash_hook.call_target();
        }
    },
};

ConsoleCommand2 damage_screen_flash_cmd{
    "damage_screen_flash",
    []() {
        g_game_config.damage_screen_flash = !g_game_config.damage_screen_flash;
        g_game_config.save();
        rf::console_printf("Damage screen flash effect is %s", g_game_config.damage_screen_flash ? "enabled" : "disabled");
    },
    "Toggle damage screen flash effect",
};

void gr_light_use_static(bool use_static)
{
    // Enable some experimental flag that causes static lights to be included in computations
    auto& experimental_alloc_and_lighting = addr_as_ref<bool>(0x00879AF8);
    auto& gr_light_state = addr_as_ref<int>(0x00C96874);
    experimental_alloc_and_lighting = use_static;
    // Increment light cache key to trigger cache invalidation
    gr_light_state++;
}

CodeInjection display_full_screen_image_alpha_support_patch{
    0x00432CAF,
    [](auto& regs) {
        static rf::GrMode mode{
            rf::TEXTURE_SOURCE_WRAP,
            rf::COLOR_SOURCE_TEXTURE,
            rf::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
            rf::ALPHA_BLEND_ALPHA,
            rf::ZBUFFER_TYPE_NONE,
            rf::FOG_ALLOWED
        };
        regs.edx = mode;
    },
};

void gr_delete_texture(int bm_handle)
{
    if (rf::gr_screen.mode == rf::GR_DIRECT3D) {
        gr_d3d_delete_texture(bm_handle);
    }
}

bool gr_is_texture_format_supported(rf::BmFormat format)
{
    if (rf::gr_screen.mode == rf::GR_DIRECT3D) {
        bool gr_d3d_is_texture_format_supported(rf::BmFormat);
        return gr_d3d_is_texture_format_supported(format);
    }
    return false;
}

void gr_apply_patch()
{
    if (g_game_config.wnd_mode != GameConfig::FULLSCREEN) {
        // Enable windowed mode
        write_mem<u32>(0x004B29A5 + 6, 0xC8);
        setup_stretched_window_patch.install();
    }

    // Fix FOV for widescreen
    gr_setup_3d_hook.install();

#if 1
    // Fix rendering of right and bottom edges of viewport in gameplay_render_frame (part 1)
    write_mem<u8>(0x00431D9F, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x00431F6B, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004328CF, asm_opcodes::jmp_rel_short);
    AsmWriter(0x0043298F).jmp(0x004329DC);

    // Left and top viewport edge fix for MSAA (RF does similar thing in gr_d3d_bitmap)
    gr_rect_gr_tmapper_hook.install();
#endif

    // Fonts
    gr_font_apply_patch();

    // D3D generic patches
    gr_d3d_apply_patch();

    // D3D texture handling
    gr_d3d_texture_apply_patch();

    // Back-buffer capture or render to texture related code
    gr_d3d_capture_apply_patch();

    // Gamma related code
    gr_d3d_gamma_apply_patch();

    // Bink Video patch
    bink_apply_patch();

    // Do not flush drawing buffers in gr_set_color
    write_mem<u8>(0x0050CFEB, asm_opcodes::jmp_rel_short);

    // Fix details and liquids rendering in railgun scanner. They were unnecessary modulated with very dark green color,
    // which seems to be a left-over from an old implementationof railgun scanner green overlay (currently it is
    // handled by drawing a green rectangle after all the geometry is drawn)
    write_mem<u8>(0x004D33F3, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004D410D, asm_opcodes::jmp_rel_short);

    // Support disabling of damage screen flash effect
    do_damage_screen_flash_hook.install();

    // Use gr_clear instead of gr_rect for faster drawing of the fog background
    AsmWriter(0x00431F99).call(0x0050CDF0);

    // Support textures with alpha channel in Display_Fullscreen_Image event
    display_full_screen_image_alpha_support_patch.install();

    // Commands
    fov_cmd.register_cmd();
    damage_screen_flash_cmd.register_cmd();
}

void gr_render_fps_counter()
{
    if (g_game_config.fps_counter && !rf::hud_disabled) {
        auto text = string_format("FPS: %.1f", rf::current_fps);
        rf::gr_set_color(0, 255, 0, 255);
        int x = rf::gr_screen_width() - (g_game_config.big_hud ? 165 : 90);
        int y = 10;
        if (rf::gameseq_in_gameplay()) {
            y = g_game_config.big_hud ? 110 : 60;
            if (hud_weapons_is_double_ammo()) {
                y += g_game_config.big_hud ? 80 : 40;
            }
        }

        int font_id = hud_get_default_font();
        rf::gr_string(x, y, text.c_str(), font_id);
    }

    void frametime_render();
    frametime_render();
}

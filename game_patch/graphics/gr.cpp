#include <algorithm>
#include <common/error/d3d-error.h>
#include <common/utils/list-utils.h>
#include <common/utils/os-utils.h>
#include <common/config/BuildConfig.h>
#include <common/ComPtr.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../os/console.h"
#include "../main/main.h"
#include "../multi/multi.h"
#include "../rf/gr/gr.h"
#include "../rf/level.h"
#include "../rf/geometry.h"
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../rf/os/os.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "gr.h"
#include "gr_internal.h"
#include "legacy/gr_d3d.h"

namespace df::gr::d3d11
{
    bool set_render_target(int bm_handle);
    void update_window_mode();
    void bitmap_float(int bitmap_handle, float x, float y, float w, float h, float sx, float sy, float sw, float sh, bool flip_x, bool flip_y, rf::gr::Mode mode);
}

float gr_lod_dist_scale = 1.0f;

CodeInjection gr_init_stretched_window_injection{
    0x0050C464,
    [](auto& regs) {
        if (g_game_config.wnd_mode == GameConfig::STRETCHED) {
            // Make sure stretched window is always full screen
            auto cx = GetSystemMetrics(SM_CXSCREEN);
            auto cy = GetSystemMetrics(SM_CYSCREEN);
            SetWindowLongA(rf::main_wnd, GWL_STYLE, WS_POPUP | WS_SYSMENU);
            SetWindowLongA(rf::main_wnd, GWL_EXSTYLE, 0);
            SetWindowPos(rf::main_wnd, HWND_NOTOPMOST, 0, 0, cx, cy, SWP_SHOWWINDOW);
            rf::gr::screen.aspect = static_cast<float>(cx) / static_cast<float>(cy) * 0.75f;
            regs.eip = 0x0050C551;
        }
    },
};

CodeInjection gr_init_injection{
    0x0050C556,
    []() {
        // Make sure pixel aspect ratio is set to 1 so the frame is not stretched
        rf::gr::screen.aspect = 1.0f;
    },
};

float gr_scale_fov_hor_plus(float horizontal_fov)
{
    // Use Hor+ FOV scaling method to improve user experience for wide screens
    // Assume provided FOV makes sense on a 4:3 screen
    float s = static_cast<float>(rf::gr::screen.max_w) / rf::gr::screen.max_h * 0.75f;
    constexpr float pi = 3.141592f;
    float h_fov_rad = horizontal_fov / 180.0f * pi;
    float x = std::tan(h_fov_rad / 2.0f);
    float y = x * s;
    h_fov_rad = 2.0f * std::atan(y);
    horizontal_fov = h_fov_rad / pi * 180.0f;
    // Clamp the value to avoid artifacts when the view is very stretched
    horizontal_fov = std::min<float>(horizontal_fov, GameConfig::max_fov);
    return horizontal_fov;
}

float gr_scale_world_fov(float horizontal_fov = 90.0f)
{
    if (g_game_config.horz_fov > 0.0f) {
        // Use user provided factor
        // Note: 90 is the default FOV for RF
        horizontal_fov *= g_game_config.horz_fov / 90.0f;
    }
    else {
        horizontal_fov = gr_scale_fov_hor_plus(horizontal_fov);
    }

    const auto& server_info_opt = get_df_server_info();
    if (server_info_opt && server_info_opt.value().max_fov) {
        horizontal_fov = std::min(horizontal_fov, server_info_opt.value().max_fov.value());
    }
    return horizontal_fov;
}

CodeInjection gameplay_render_frame_fov_injection{
    0x00431BA1,
    []() {
        // Scale world FOV
        auto& rf_fov = addr_as_ref<float>(0x0059613C);
        rf_fov = gr_scale_world_fov(rf_fov);
    },
};

CallHook<void(rf::Matrix3&, rf::Vector3&, float, bool, bool)> gr_setup_3d_railgun_hook{
    {
        0x00431CE9, // railgun
        0x004ADDB4,
    },
    [](rf::Matrix3& viewer_orient, rf::Vector3& viewer_pos, float horizontal_fov, bool zbuffer_flag, bool z_scale) {
        horizontal_fov = gr_scale_fov_hor_plus(horizontal_fov);
        gr_setup_3d_railgun_hook.call_target(viewer_orient, viewer_pos, horizontal_fov, zbuffer_flag, z_scale);
    },
};

ConsoleCommand2 fov_cmd{
    "fov",
    [](std::optional<float> fov_opt) {
        if (fov_opt) {
            if (fov_opt.value() <= 0.0f) {
                g_game_config.horz_fov = 0.0f;
            }
            else {
                g_game_config.horz_fov = fov_opt.value();
            }
            g_game_config.save();
        }
        rf::console::print("Horizontal FOV: {:.2f}", gr_scale_world_fov());

        const auto& server_info_opt = get_df_server_info();
        if (server_info_opt && server_info_opt.value().max_fov) {
            rf::console::print("Server FOV limit: {:.2f}", server_info_opt.value().max_fov.value());
        }
    },
    "Sets horizontal FOV (field of view) in degrees. Use 0 to enable automatic FOV scaling.",
    "fov [degrees]",
};

ConsoleCommand2 gamma_cmd{
    "gamma",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            rf::gr::set_gamma(value_opt.value());
        }
        rf::console::print("Gamma: {:.2f}", rf::gr::gamma);
    },
    "Sets gamma.",
    "gamma [value]",
};

void evaluate_lightmaps_only()
{
    bool server_side_restrict_lightmaps_only =
        rf::is_multi && !rf::is_server && get_df_server_info() && !get_df_server_info()->allow_lmap;

    if (server_side_restrict_lightmaps_only) {
        if (g_game_config.try_lightmaps_only) {
            rf::console::print("This server does not allow you to use lightmap only mode!");
            rf::gr::show_lightmaps = false;
            rf::g_cache_clear();
        }
    }
    else {
        if (rf::gr::show_lightmaps != g_game_config.try_lightmaps_only) {
            rf::gr::show_lightmaps = g_game_config.try_lightmaps_only;
            rf::g_cache_clear();
        }
    }
}

ConsoleCommand2 lightmaps_only_cmd{
    "lightmaps_only",
    []() {
        g_game_config.try_lightmaps_only = !g_game_config.try_lightmaps_only;
        g_game_config.save();

        evaluate_lightmaps_only();

        rf::console::print("Lightmap only mode is {}", g_game_config.try_lightmaps_only ?
            "enabled. In multiplayer, this will only apply if the server allows it." : "disabled.");
    },
    "Render only lightmaps for level geometry (no textures). In multiplayer, this is only available if the server allows it.",
};

FunHook<float(const rf::Vector3&)> gr_get_apparent_distance_from_camera_hook{
    0x005182F0,
    [](const rf::Vector3& pos) {
        return gr_get_apparent_distance_from_camera_hook.call_target(pos) / gr_lod_dist_scale;
    },
};

bool gr_is_texture_format_supported(rf::bm::Format format)
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        if (g_game_config.renderer == GameConfig::Renderer::d3d11) {
            return true;
        }
        else {
            return gr_d3d_is_texture_format_supported(format);
        }
    }
    return false;
}

bool gr_set_render_target(int bm_handle)
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        if (g_game_config.renderer == GameConfig::Renderer::d3d11) {
            return df::gr::d3d11::set_render_target(bm_handle);
        }
        else {
            return gr_d3d_set_render_target(bm_handle);
        }
    }
    return false;
}

void gr_bitmap_scaled_float(int bitmap_handle, float x, float y, float w, float h,
                            float sx, float sy, float sw, float sh, bool flip_x, bool flip_y, rf::gr::Mode mode)
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        if (g_game_config.renderer == GameConfig::Renderer::d3d11) {
            df::gr::d3d11::bitmap_float(bitmap_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
        }
        else {
            gr_d3d_bitmap_float(bitmap_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
        }
    }
}

void gr_set_window_mode(rf::gr::WindowMode window_mode)
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        rf::gr::screen.window_mode = window_mode;
        if (g_game_config.renderer == GameConfig::Renderer::d3d11) {
            df::gr::d3d11::update_window_mode();
        }
        else {
            gr_d3d_update_window_mode();
        }
    }
}

void gr_update_texture_filtering()
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        if (g_game_config.renderer != GameConfig::Renderer::d3d11) {
            gr_d3d_update_texture_filtering();
        }
    }
}

ConsoleCommand2 fullscreen_cmd{
    "fullscreen",
    []() {
        gr_set_window_mode(rf::gr::FULLSCREEN);
    },
};

ConsoleCommand2 windowed_cmd{
    "windowed",
    []() {
        gr_set_window_mode(rf::gr::WINDOWED);
    },
};

ConsoleCommand2 nearest_texture_filtering_cmd{
    "nearest_texture_filtering",
    []() {
        g_game_config.nearest_texture_filtering = !g_game_config.nearest_texture_filtering;
        g_game_config.save();
        gr_update_texture_filtering();
        rf::console::print("Nearest texture filtering is {}", g_game_config.nearest_texture_filtering ? "enabled" : "disabled");
    },
    "Toggle nearest texture filtering",
};

ConsoleCommand2 lod_distance_scale_cmd{
    "lod_distance_scale",
    [](std::optional<float> scale_opt) {
        if (scale_opt.has_value()) {
            gr_lod_dist_scale = scale_opt.value();
        }
        rf::console::print("LOD distance scale: {:.2f}", gr_lod_dist_scale);
    },
    "Sets LOD distance scale factor",
};

void gr_apply_patch()
{
    if (g_game_config.wnd_mode != GameConfig::FULLSCREEN) {
        // Enable windowed mode
        write_mem<u32>(0x004B29A5 + 6, 0xC8);
        gr_init_stretched_window_injection.install();
    }

    // Fix FOV for widescreen
    gr_init_injection.install();
    gameplay_render_frame_fov_injection.install();
    gr_setup_3d_railgun_hook.install();

    // Fix rendering of right and bottom edges of viewport in gameplay_render_frame (part 1)
    write_mem<u8>(0x00431D9F, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x00431F6B, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004328CF, asm_opcodes::jmp_rel_short);
    AsmWriter(0x0043298F).jmp(0x004329DC);

    // Fonts
    gr_font_apply_patch();

    // Lights
    gr_light_apply_patch();

    if (g_game_config.renderer == GameConfig::Renderer::d3d11) {
        void gr_d3d11_apply_patch();
        gr_d3d11_apply_patch();
    }
    else {
        // D3D generic patches
        gr_d3d_apply_patch();

        // D3D texture handling
        gr_d3d_texture_apply_patch();

        // Back-buffer capture or render to texture related code
        gr_d3d_capture_apply_patch();

        // Gamma related code
        gr_d3d_gamma_apply_patch();
    }

    // Bink Video patch
    bink_apply_patch();

    // Do not flush drawing buffers in gr_set_color
    write_mem<u8>(0x0050CFEB, asm_opcodes::jmp_rel_short);

    // Fix details and liquids rendering in railgun scanner. They were unnecessary modulated with very dark green color,
    // which seems to be a left-over from an old implementationof railgun scanner green overlay (currently it is
    // handled by drawing a green rectangle after all the geometry is drawn)
    write_mem<u8>(0x004D33F3, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004D410D, asm_opcodes::jmp_rel_short);

    // Use gr_clear instead of gr_rect for faster drawing of the fog background
    AsmWriter(0x00431F99).call(0x0050CDF0);

    // Fix possible divisions by zero
    // Fixes performance issues caused by gr::sceen::fog_far_scaled being initialized to inf
    rf::gr::matrix_scale.set(1.0f, 1.0f, 1.0f);
    rf::gr::one_over_matrix_scale_z = 1.0f;

    // Increase mesh details
    gr_get_apparent_distance_from_camera_hook.install();
    if (g_game_config.disable_lod_models) {
        // Do not scale LOD distance for character in multi (gr_d3d_render_lod_vif)
        AsmWriter{0x0052FAED, 0x0052FAFB}.nop();
        // Change default LOD scale
        gr_lod_dist_scale = 10.0f;
    }

    // Fix gr_rect_border not drawing left border
    AsmWriter{0x0050DF2D}.push(asm_regs::ebp).push(asm_regs::ebx);

    // Commands
    fov_cmd.register_cmd();
    gamma_cmd.register_cmd();
    lightmaps_only_cmd.register_cmd();
    fullscreen_cmd.register_cmd();
    windowed_cmd.register_cmd();
    nearest_texture_filtering_cmd.register_cmd();
    lod_distance_scale_cmd.register_cmd();
}

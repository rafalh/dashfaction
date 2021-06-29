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
#include "../main/main.h"
#include "../multi/multi.h"
#include "../rf/gr/gr.h"
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../rf/os/os.h"
#include "../rf/item.h"
#include "../rf/clutter.h"

#define USE_D3D11 1

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
        rf::console::printf("Horizontal FOV: %.2f", gr_scale_world_fov());

        const auto& server_info_opt = get_df_server_info();
        if (server_info_opt && server_info_opt.value().max_fov) {
            rf::console::printf("Server FOV limit: %.2f", server_info_opt.value().max_fov.value());
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
        rf::console::printf("Gamma: %.2f", rf::gr::gamma);
    },
    "Sets gamma.",
    "gamma [value]",
};

CallHook<void(int, rf::gr::Vertex**, int, rf::gr::Mode)> gr_rect_gr_tmapper_hook{
    0x0050DD69,
    [](int nv, rf::gr::Vertex** verts, int flags, rf::gr::Mode mode) {
        for (int i = 0; i < nv; ++i) {
            verts[i]->sx -= 0.5f;
            verts[i]->sy -= 0.5f;
        }
        gr_rect_gr_tmapper_hook.call_target(nv, verts, flags, mode);
    },
};

void gr_delete_texture(int bm_handle)
{
#if USE_D3D11
    void gr_d3d11_delete_texture(int bm_handle);
    gr_d3d11_delete_texture(bm_handle);
#else
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        gr_d3d_delete_texture(bm_handle);
    }
#endif
}

bool gr_is_texture_format_supported(rf::bm::Format format)
{
#if USE_D3D11
    (void) format;
    return true;
#else
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
        bool gr_d3d_is_texture_format_supported(rf::bm::Format);
        return gr_d3d_is_texture_format_supported(format);
    }
    return false;
#endif
}

bool gr_set_render_target(int bm_handle)
{
    if (rf::gr::screen.mode == rf::gr::DIRECT3D) {
#if USE_D3D11
    bool gr_d3d11_set_render_target(int bm_handle);
    return gr_d3d11_set_render_target(bm_handle);
#else
    bool gr_d3d_set_render_target(int bm_handle);
    return gr_d3d_set_render_target(bm_handle);
#endif
    }
}

void gr_bitmap_scaled_float(int bitmap_handle, float x, float y, float w, float h,
                            float sx, float sy, float sw, float sh, bool flip_x, bool flip_y, rf::gr::Mode mode)
{
#if !USE_D3D11
    auto& gr_d3d_get_num_texture_sections = addr_as_ref<int(int bitmap_handle)>(0x0055CA60);
    if (rf::gr::screen.mode == rf::gr::DIRECT3D && gr_d3d_get_num_texture_sections(bitmap_handle) != 1) {
        // If bitmap is sectioned fall back to the old implementation...
        rf::gr::bitmap_scaled(bitmap_handle,
            static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h),
            static_cast<int>(sx), static_cast<int>(sy), static_cast<int>(sw), static_cast<int>(sh),
            flip_x, flip_y, mode);
        return;
    }
#endif

    rf::gr::set_texture(bitmap_handle, -1);
    int bm_w, bm_h;
    rf::bm::get_dimensions(bitmap_handle, &bm_w, &bm_h);

    // For some reason original implementation do not allow UVs > 1
    sw = std::min(sw, bm_w - sx);
    sh = std::min(sh, bm_h - sy);
    if (sw <= 0.0f || sh <= 0.0f) {
        return;
    }

    rf::gr::Vertex verts[4];
    rf::gr::Vertex* verts_ptrs[4] = {&verts[0], &verts[1], &verts[2], &verts[3]};
    float sx_left = rf::gr::screen.offset_x + x - 0.5f;
    float sx_right = rf::gr::screen.offset_x + x + w - 0.5f;
    float sy_top = rf::gr::screen.offset_y + y - 0.5f;
    float sy_bottom = rf::gr::screen.offset_y + y + h - 0.5f;
    float u_left = sx / bm_w * (flip_x ? -1.0f : 1.0f);
    float u_right = (sx + sw) / bm_w * (flip_x ? -1.0f : 1.0f);
    float v_top = sy / bm_h * (flip_y ? -1.0f : 1.0f);
    float v_bottom = (sy + sh) / bm_h * (flip_y ? -1.0f : 1.0f);
    verts[0].sx = sx_left;
    verts[0].sy = sy_top;
    verts[0].sw = 1.0f;
    verts[0].u1 = u_left;
    verts[0].v1 = v_top;
    verts[1].sx = sx_right;
    verts[1].sy = sy_top;
    verts[1].sw = 1.0f;
    verts[1].u1 = u_right;
    verts[1].v1 = v_top;
    verts[2].sx = sx_right;
    verts[2].sy = sy_bottom;
    verts[2].sw = 1.0f;
    verts[2].u1 = u_right;
    verts[2].v1 = v_bottom;
    verts[3].sx = sx_left;
    verts[3].sy = sy_bottom;
    verts[3].sw = 1.0f;
    verts[3].u1 = u_left;
    verts[3].v1 = v_bottom;
    rf::gr::tmapper(std::size(verts_ptrs), verts_ptrs, rf::gr::TMAP_FLAG_TEXTURED, mode);
}

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

    // Lights
    gr_light_apply_patch();

#if USE_D3D11
    void gr_d3d11_apply_patch();
    gr_d3d11_apply_patch();
#else
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
#endif

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

    // Commands
    fov_cmd.register_cmd();
    gamma_cmd.register_cmd();
}

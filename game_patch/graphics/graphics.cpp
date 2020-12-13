#include <windows.h>
#include <d3d8.h>
#include "graphics.h"
#include "graphics_internal.h"
#include "gr_color.h"
#include "../console/console.h"
#include "../in_game_ui/hud.h"
#include "../main.h"
#include "../rf/graphics.h"
#include "../rf/gr_direct3d.h"
#include "../rf/player.h"
#include "../rf/multi.h"
#include "../rf/hud.h"
#include "../rf/gameseq.h"
#include "../rf/os.h"
#include <common/error/d3d-error.h>
#include <common/utils/string-utils.h>
#include <common/utils/list-utils.h>
#include <common/utils/os-utils.h>
#include "../rf/item.h"
#include "../rf/clutter.h"
#include <common/config/BuildConfig.h>
#include <patch_common/ComPtr.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <algorithm>

static float g_gr_clipped_geom_offset_x = -0.5;
static float g_gr_clipped_geom_offset_y = -0.5;

static void set_texture_min_mag_filter_in_code(D3DTEXTUREFILTERTYPE filter_type0, D3DTEXTUREFILTERTYPE filter_type1)
{
    // clang-format off
    uintptr_t addresses_tex0[] = {
        0x0054F2E8, 0x0054F2FC, // TEXTURE_SOURCE_WRAP
        0x0054F438, 0x0054F44C, // TEXTURE_SOURCE_CLAMP
        0x0054F62D, 0x0054F641, // TEXTURE_SOURCE_MT_WRAP 0
        0x0054F800, 0x0054F814, // TEXTURE_SOURCE_MT_CLAMP 0
        0x0054FA1A, 0x0054FA2E, // TEXTURE_SOURCE_MT_WRAP_M2X 0
        0x0054FBFE, 0x0054FC12, // TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP 0
        0x0054FD90, 0x0054FDA4, // TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP 0
        0x0054FF74, 0x0054FF88, // TEXTURE_SOURCE_MT_WRAP_TRILIN 0
        0x005500C6, 0x005500DA, // TEXTURE_SOURCE_MT_CLAMP_TRILIN 0
    };
    uintptr_t addresses_tex1[] = {
        0x0054F709, 0x0054F71D, // TEXTURE_SOURCE_MT_WRAP 1
        0x0054F909, 0x0054F91D, // TEXTURE_SOURCE_MT_CLAMP 1
        0x0054FB22, 0x0054FB36, // TEXTURE_SOURCE_MT_WRAP_M2X 1
        0x0054FD06, 0x0054FD1A, // TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP 1
        0x0054FE98, 0x0054FEAC, // TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP 1
        0x0055007C, 0x00550090, // TEXTURE_SOURCE_MT_WRAP_TRILIN 1
        0x005501A2, 0x005501B6, // TEXTURE_SOURCE_MT_CLAMP_TRILIN 1
    };
    // clang-format on
    for (auto address : addresses_tex0) {
        write_mem<u8>(address + 1, filter_type0);
    }
    for (auto address : addresses_tex1) {
        write_mem<u8>(address + 1, filter_type1);
    }
}

CodeInjection gr_d3d_setup_3d_widescreen_fix{
    0x005473AD,
    []() {
        constexpr float ref_aspect_ratio = 4.0f / 3.0f;
        constexpr float max_wide_aspect_ratio = 21.0f / 9.0f; // biggest aspect ratio currently in market

        // g_gr_screen.aspect == ScrW / ScrH * 0.75 (1.0 for 4:3 monitors, 1.2 for 16:10) - looks like Pixel Aspect
        // Ratio We use here MaxWidth and MaxHeight to calculate proper FOV for windowed mode

        float viewport_aspect_ratio = static_cast<float>(rf::gr_screen.clip_width) / rf::gr_screen.clip_height;
        float aspect_ratio = static_cast<float>(rf::gr_screen.max_width) / rf::gr_screen.max_height;
        float scale_x = 1.0f;
        // this is how RF does compute scale_y and it is needed for working scanner
        float scale_y = ref_aspect_ratio * viewport_aspect_ratio / aspect_ratio;

        if (aspect_ratio <= max_wide_aspect_ratio) // never make X scale too high in windowed mode
            scale_x *= ref_aspect_ratio / aspect_ratio;
        else {
            scale_x *= ref_aspect_ratio / max_wide_aspect_ratio;
            scale_y *= aspect_ratio / max_wide_aspect_ratio;
        }

        rf::gr_scale_vec.x *= scale_x; // Note: original division by aspect ratio is noped
        rf::gr_scale_vec.y *= scale_y;

        g_gr_clipped_geom_offset_x = rf::gr_screen.offset_x - 0.5f;
        g_gr_clipped_geom_offset_y = rf::gr_screen.offset_y - 0.5f;
        auto& gr_viewport_center_x = addr_as_ref<float>(0x01818B54);
        auto& gr_viewport_center_y = addr_as_ref<float>(0x01818B5C);
        gr_viewport_center_x -= 0.5f; // viewport center x
        gr_viewport_center_y -= 0.5f; // viewport center y
    },
};

CodeInjection gr_d3d_init_error_patch{
    0x00545CBD,
    [](auto& regs) {
        auto hr = static_cast<HRESULT>(regs.eax);
        xlog::error("D3D CreateDevice failed (hr 0x%lX - %s)", hr, get_d3d_error_str(hr));

        auto text = string_format("Failed to create Direct3D device object - error 0x%lX (%s).\n"
                                 "A critical error has occurred and the program cannot continue.\n"
                                 "Press OK to exit the program",
                                 hr, get_d3d_error_str(hr));

        hr = rf::gr_d3d->CheckDeviceType(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat,
            rf::gr_d3d_pp.BackBufferFormat, rf::gr_d3d_pp.Windowed);
        if (FAILED(hr)) {
            xlog::error("CheckDeviceType for format %d failed: %lX", rf::gr_d3d_pp.BackBufferFormat, hr);
        }

        hr = rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat,
            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, rf::gr_d3d_pp.AutoDepthStencilFormat);
        if (FAILED(hr)) {
            xlog::error("CheckDeviceFormat for depth-stencil format %d failed: %lX", rf::gr_d3d_pp.AutoDepthStencilFormat, hr);
        }

        if (!(rf::gr_d3d_device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
            xlog::error("No T&L hardware support!");
        }

        ShowWindow(rf::main_wnd, SW_HIDE);
        MessageBoxA(nullptr, text.c_str(), "Error!", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TASKMODAL);
        ExitProcess(-1);
    },
};

CodeInjection gr_zbuffer_clear_fix_rect{
    0x00550A19,
    [](auto& regs) {
        D3DRECT* rect = regs.edx;
        rect->x2++;
        rect->y2++;
    },
};

D3DFORMAT determine_depth_buffer_format(D3DFORMAT adapter_format)
{
    D3DFORMAT formats_to_check[] = {D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D32, D3DFMT_D16, D3DFMT_D15S1};
    for (auto depth_fmt : formats_to_check) {

        if (SUCCEEDED(rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, adapter_format, D3DUSAGE_DEPTHSTENCIL,
                                                    D3DRTYPE_SURFACE, depth_fmt)) &&
            SUCCEEDED(rf::gr_d3d->CheckDepthStencilMatch(rf::gr_adapter_idx, D3DDEVTYPE_HAL, adapter_format, adapter_format,
                                                         depth_fmt))) {
            xlog::info("Selected D3D depth format: %u", depth_fmt);
            return depth_fmt;
        }
    }
    xlog::warn("CheckDeviceFormat failed for all depth formats!");
    return D3DFMT_D16;
}

CodeInjection update_pp_hook{
    0x00545BC7,
    []() {
        if (gr_d3d_is_d3d8to9()) {
            xlog::info("d3d8to9 detected");
        }

        auto& format = addr_as_ref<D3DFORMAT>(0x005A135C);
        xlog::info("D3D Format: %u", format);

        xlog::info("D3D DevCaps: %lX", rf::gr_d3d_device_caps.DevCaps);
        xlog::info("Max texture size: %ldx%ld", rf::gr_d3d_device_caps.MaxTextureWidth, rf::gr_d3d_device_caps.MaxTextureHeight);

        if (g_game_config.msaa && format > 0) {
            // Make sure selected MSAA mode is available
            auto multi_sample_type = static_cast<D3DMULTISAMPLE_TYPE>(g_game_config.msaa.value());
            HRESULT hr = rf::gr_d3d->CheckDeviceMultiSampleType(rf::gr_adapter_idx, D3DDEVTYPE_HAL, format,
                                                                rf::gr_d3d_pp.Windowed, multi_sample_type);
            if (SUCCEEDED(hr)) {
                xlog::info("Enabling Anti-Aliasing (%ux MSAA)...", g_game_config.msaa.value());
                rf::gr_d3d_pp.MultiSampleType = multi_sample_type;
            }
            else {
                xlog::warn("MSAA not supported (0x%lx)...", hr);
                g_game_config.msaa = D3DMULTISAMPLE_NONE;
            }
        }

        // remove D3DPRESENTFLAG_LOCKABLE_BACKBUFFER flag
        rf::gr_d3d_pp.Flags = 0;
#if D3D_LOCKABLE_BACKBUFFER
        // Note: if MSAA is used backbuffer cannot be lockable
        if (g_game_config.msaa == D3DMULTISAMPLE_NONE)
            rf::gr_d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#endif

        // Use glorad detail map on all cards (multi-texturing support is required)
        auto& use_glorad_detail_map = addr_as_ref<bool>(0x01CFCBCC);
        use_glorad_detail_map = true;

        // Override depth format to avoid card specific hackfixes that makes it different on Nvidia and AMD
        rf::gr_d3d_pp.AutoDepthStencilFormat = determine_depth_buffer_format(format);

        init_supported_texture_formats();
    },
};

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

#if D3D_HW_VERTEX_PROCESSING

CodeInjection d3d_behavior_flags_patch{
    0x00545BE0,
    [](auto& regs) {
        auto& behavior_flags = *static_cast<u32*>(regs.esp);
        // Use hardware vertex processing instead of software processing if supported
        if (rf::gr_d3d_device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
            xlog::info("Enabling T&L in hardware");
            behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }
        else {
            xlog::info("T&L in hardware is not supported");
        }
    },
};

CodeInjection d3d_vertex_buffer_usage_patch{
    0x005450E2,
    [](auto& regs) {
        auto& usage = *static_cast<u32*>(regs.esp);
        if (rf::gr_d3d_device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
            usage &= ~D3DUSAGE_SOFTWAREPROCESSING;
        }
    },
};

CodeInjection d3d_index_buffer_usage_patch{
    0x0054511C,
    [](auto& regs) {
        auto& usage = *static_cast<u32*>(regs.esp);
        if (rf::gr_d3d_device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
            usage &= ~D3DUSAGE_SOFTWAREPROCESSING;
        }
    },
};

#endif // D3D_HW_VERTEX_PROCESSING

CallHook<void(int, rf::GrVertex**, int, int)> gr_rect_gr_tmapper_hook{
    0x0050DD69,
    [](int num, rf::GrVertex** pp_vertices, int flags, int mat) {
        for (int i = 0; i < num; ++i) {
            pp_vertices[i]->spos.x -= 0.5f;
            pp_vertices[i]->spos.y -= 0.5f;
        }
        gr_rect_gr_tmapper_hook.call_target(num, pp_vertices, flags, mat);
    },
};

void setup_texture_filtering()
{
    if (g_game_config.nearest_texture_filtering) {
        // use linear filtering for lightmaps because otherwise it looks bad
        set_texture_min_mag_filter_in_code(D3DTEXF_POINT, D3DTEXF_LINEAR);
    }
    else if (rf::gr_d3d_device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering && !rf::is_dedicated_server) {
        // Anisotropic texture filtering
        set_texture_min_mag_filter_in_code(D3DTEXF_ANISOTROPIC, D3DTEXF_ANISOTROPIC);
    } else {
        set_texture_min_mag_filter_in_code(D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }
}

DWORD setup_max_anisotropy()
{
    DWORD anisotropy_level = std::min(rf::gr_d3d_device_caps.MaxAnisotropy, 16ul);
    rf::gr_d3d_device->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, anisotropy_level);
    rf::gr_d3d_device->SetTextureStageState(1, D3DTSS_MAXANISOTROPY, anisotropy_level);
    return anisotropy_level;
}

CallHook<void()> gr_d3d_init_buffers_gr_d3d_flip_hook{
    0x00545045,
    []() {
        gr_d3d_init_buffers_gr_d3d_flip_hook.call_target();

        // Apply state change after reset
        // Note: we dont have to set min/mag filtering because its set when selecting material

        rf::gr_d3d_device->SetRenderState(D3DRS_CULLMODE, 1);
        rf::gr_d3d_device->SetRenderState(D3DRS_SHADEMODE, 2);
        rf::gr_d3d_device->SetRenderState(D3DRS_SPECULARENABLE, 0);
        rf::gr_d3d_device->SetRenderState(D3DRS_AMBIENT, 0xFF545454);
        rf::gr_d3d_device->SetRenderState(D3DRS_CLIPPING, 0);

        if (rf::local_player)
            rf::gr_set_texture_mip_filter(rf::local_player->settings.filtering_level == 0);

        if (rf::gr_d3d_device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering)
            setup_max_anisotropy();
    },
};

bool g_reset_device_req = false;

CodeInjection switch_d3d_mode_patch{
    0x0054500D,
    [](auto& regs) {
        if (g_reset_device_req) {
            g_reset_device_req = false;
            regs.eip = 0x00545017;
        }
    },
};

ConsoleCommand2 fullscreen_cmd{
    "fullscreen",
    []() {
        rf::gr_d3d_pp.Windowed = false;
        rf::gr_d3d_pp.FullScreen_PresentationInterval =
            g_game_config.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        g_reset_device_req = true;
    },
};

ConsoleCommand2 windowed_cmd{
    "windowed",
    []() {
        rf::gr_d3d_pp.Windowed = true;
        rf::gr_d3d_pp.FullScreen_PresentationInterval = 0;
        g_reset_device_req = true;
    },
};

ConsoleCommand2 antialiasing_cmd{
    "antialiasing",
    []() {
        if (!g_game_config.msaa)
            rf::console_printf("Anti-aliasing is not supported");
        else {
            DWORD enabled = 0;
            rf::gr_d3d_device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &enabled);
            enabled = !enabled;
            rf::gr_d3d_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, enabled);
            rf::console_printf("Anti-aliasing is %s", enabled ? "enabled" : "disabled");
        }
    },
    "Toggles anti-aliasing",
};

// frame profiling
#ifdef DEBUG

static bool g_profile_frame_req = false;
static bool g_profile_frame = false;
static int g_num_draw_calls = 0;

CodeInjection gr_d3d_set_mode_profile_patch{
    0x0054F19C,
    [](auto& regs) {
        if (g_profile_frame) {
            auto state_flags = addr_as_ref<rf::GrMode>(regs.esp + 0x10 + 0x4);
            const char* desc = "";
            if (state_flags == rf::gr_string_mode)
                desc = " (text)";
            else if (state_flags == rf::gr_rect_mode)
                desc = " (rect)";
            else if (state_flags == rf::gr_line_mode)
                desc = " (line)";
            else if (state_flags == rf::gr_bitmap_clamp_mode)
                desc = " (bitmap)";
            xlog::info("GrD3DSetMaterialFlags 0x%X%s", state_flags.value, desc);
        }
    },
};

CodeInjection gr_d3d_set_texture_profile_patch{
    0x0055CB6A,
    [](auto& regs) {
        if (g_profile_frame) {
            unsigned bm_handle = regs.edi;
            int stage_id = (regs.ebx - 0x01E65308) / 0x18;
            xlog::info("GrD3DSetTexture %d 0x%X %s", stage_id, bm_handle, rf::bm_get_filename(bm_handle));
        }
    },
};

CodeInjection D3D_DrawIndexedPrimitive_profile_patch{
    0,
    [](auto& regs) {
        if (g_profile_frame) {
            // Note: this is passed by stack because d3d uses stdcall
            auto prim_type = addr_as_ref<int>(regs.esp + 8);
            auto min_index = addr_as_ref<unsigned>(regs.esp + 12);
            auto num_vertices = addr_as_ref<unsigned>(regs.esp + 16);
            auto start_index = addr_as_ref<unsigned>(regs.esp + 20);
            auto prim_count = addr_as_ref<unsigned>(regs.esp + 24);
            xlog::info("DrawIndexedPrimitive %d %d %u %u %u", prim_type, min_index, num_vertices, start_index, prim_count);
            ++g_num_draw_calls;
        }
    },
};

FunHook<void()> gr_flip_profile_patch{
    0x0050CE20,
    []() {
        gr_flip_profile_patch.call_target();

        if (g_profile_frame) {
            xlog::info("Total draw calls: %d", g_num_draw_calls);
        }
        g_profile_frame = g_profile_frame_req;
        g_profile_frame_req = false;
        g_num_draw_calls = 0;
    },
};

ConsoleCommand2 profile_frame_cmd{
    "profile_frame",
    []() {
        g_profile_frame_req = true;

        static bool patches_installed = false;
        if (!patches_installed) {
            gr_d3d_set_mode_profile_patch.install();
            gr_d3d_set_texture_profile_patch.install();
            gr_flip_profile_patch.install();
            auto d3d_dev_vtbl = *reinterpret_cast<uintptr_t**>(rf::gr_d3d_device);
            D3D_DrawIndexedPrimitive_profile_patch.set_addr(d3d_dev_vtbl[0x11C / 4]); // DrawIndexedPrimitive
            D3D_DrawIndexedPrimitive_profile_patch.install();
            patches_installed = true;
        }
    },
};

#endif // DEBUG

CodeInjection after_gr_init_hook{
    0x0050C5CD,
    []() {
        setup_texture_filtering();
        if (rf::gr_d3d_device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering && !rf::is_dedicated_server) {
            DWORD anisotropy_level = setup_max_anisotropy();
            xlog::info("Anisotropic Filtering enabled (level: %lu)", anisotropy_level);
        }

        // Change font for Time Left text
        static int time_left_font = rf::gr_load_font("rfpc-large.vf");
        if (time_left_font >= 0) {
            write_mem<i8>(0x00477157 + 1, time_left_font);
            write_mem<i8>(0x0047715F + 2, 21);
            write_mem<i32>(0x00477168 + 1, 154);
        }

        // Fix performance issues caused by this field being initialized to inf
        rf::gr_screen.fog_far_scaled = 255.0f / rf::gr_screen.fog_far;
    },
};

CodeInjection load_tga_alloc_fail_fix{
    0x0051095D,
    [](auto& regs) {
        if (regs.eax == 0) {
            regs.esp += 4;
            unsigned bpp = regs.esi;
            auto num_total_pixels = addr_as_ref<size_t>(regs.ebp + 0x30);
            auto num_bytes = num_total_pixels * bpp;
            xlog::warn("Failed to allocate buffer for a bitmap: %d bytes!", num_bytes);
            regs.eip = 0x00510944;
        }
    },
};

FunHook<int(int, void*)> gr_d3d_create_vram_texture_with_mipmaps_hook{
    0x0055B700,
    [](int bm_handle, void* tex) {
        int result = gr_d3d_create_vram_texture_with_mipmaps_hook.call_target(bm_handle, tex);
        if (result != 1) {
            auto BmUnlock = addr_as_ref<void(int)>(0x00511700);
            BmUnlock(bm_handle);
        }
        return result;
    },
};

CodeInjection gr_d3d_create_texture_fail_hook{
    0x0055B9FD,
    [](auto& regs) {
        auto hr = static_cast<HRESULT>(regs.eax);
        xlog::warn("Failed to alloc texture - HRESULT 0x%lX %s", hr, get_d3d_error_str(hr));
    },
};

CodeInjection GrLoadFontInternal_fix_texture_ref{
    0x0051F429,
    [](auto& regs) {
        auto gr_tcache_add_ref = addr_as_ref<void(int bm_handle)>(0x0050E850);
        gr_tcache_add_ref(regs.eax);
    },
};

ConsoleCommand2 max_fps_cmd{
    "maxfps",
    [](std::optional<int> limit_opt) {
        if (limit_opt) {
#ifdef NDEBUG
            int new_limit = std::clamp<int>(limit_opt.value(), MIN_FPS_LIMIT, MAX_FPS_LIMIT);
#else
            int new_limit = limit_opt.value();
#endif
            g_game_config.max_fps = new_limit;
            g_game_config.save();
            rf::framerate_min = 1.0f / new_limit;
        }
        else
            rf::console_printf("Maximal FPS: %.1f", 1.0f / rf::framerate_min);
    },
    "Sets maximal FPS",
    "maxfps <limit>",
};

CodeInjection gr_d3d_lock_crash_fix{
    0x0055CE55,
    [](auto& regs) {
        if (regs.eax == 0) {
            regs.esp += 8;
            regs.eip = 0x0055CF23;
        }
    },
};

CodeInjection gr_d3d_bitmap_patch_1{
    0x00550CD6,
    [](auto& regs) {
        if (!rf::gr_d3d_buffers_locked || rf::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00550CEC;
        }
    },
};

CodeInjection gr_d3d_bitmap_patch_2{
    0x0055105F,
    [](auto& regs) {
        if (!rf::gr_d3d_buffers_locked || rf::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x0055107B;
        }
    },
};

CodeInjection gr_d3d_line_patch_1{
    0x0055147D,
    [](auto& regs) {
        bool flush_needed = !rf::gr_d3d_buffers_locked
                         || rf::gr_d3d_primitive_type != D3DPT_LINELIST
                         || rf::gr_d3d_max_hw_vertex + 2 > 6000
                         || rf::gr_d3d_max_hw_index + rf::gr_d3d_num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x00551482;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 rf::gr_d3d_buffers_locked, rf::gr_d3d_primitive_type, rf::gr_d3d_max_hw_vertex,
                 rf::gr_d3d_max_hw_index + rf::gr_d3d_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_patch_2{
    0x005515B2,
    []() {
        rf::gr_d3d_num_vertices += 2;
    },
};

CodeInjection gr_d3d_line_vertex_internal_patch_1{
    0x005516FE,
    [](auto& regs) {
        bool flush_needed = !rf::gr_d3d_buffers_locked
                         || rf::gr_d3d_primitive_type != D3DPT_LINELIST
                         || rf::gr_d3d_max_hw_vertex + 2 > 6000
                         || rf::gr_d3d_max_hw_index + rf::gr_d3d_num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x00551703;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 rf::gr_d3d_buffers_locked, rf::gr_d3d_primitive_type, rf::gr_d3d_max_hw_vertex,
                 rf::gr_d3d_max_hw_index + rf::gr_d3d_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_vertex_internal_patch_2{
    0x005518E8,
    []() {
        rf::gr_d3d_num_vertices += 2;
    },
};

CodeInjection gr_d3d_tmapper_patch{
    0x0055193B,
    [](auto& regs) {
        if (!rf::gr_d3d_buffers_locked || rf::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551958;
        }
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_1{
    0x00551EF5,
    [](auto& regs) {
        if (!rf::gr_d3d_buffers_locked || rf::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551F13;
        }
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_2{
    0x00551CB9,
    [](auto& regs) {
        if (!rf::gr_d3d_buffers_locked || rf::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551CD6;
        }
    },
};

FunHook<void()> DoDamageScreenFlash_hook{
    0x004A7520,
    []() {
        if (g_game_config.damage_screen_flash) {
            DoDamageScreenFlash_hook.call_target();
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

FunHook<void(int*, void*, int, int, int, rf::GrMode, int)> gr_d3d_queue_triangles_hook{
    0x005614B0,
    [](int *list_idx, void *vertices, int num_vertices, int bm0, int bm1, rf::GrMode state, int pass_id) {
        // Fix glass_house level having faces that use MT state but don't have any lightmap
        if (bm1 == -1) {
            WARN_ONCE("Prevented rendering of an unlit face with multi-texturing render state");
            constexpr int texture_source_mask = 0x1F;
            int tex_src = state.value & texture_source_mask;
            if (tex_src == rf::TEXTURE_SOURCE_MT_WRAP || tex_src == rf::TEXTURE_SOURCE_MT_WRAP_M2X) {
                tex_src = rf::TEXTURE_SOURCE_WRAP;
            } else if (tex_src == rf::TEXTURE_SOURCE_MT_CLAMP) {
                tex_src = rf::TEXTURE_SOURCE_CLAMP;
            }
            state.value = state.value & ~texture_source_mask;
            state.value |= tex_src;
        }

        gr_d3d_queue_triangles_hook.call_target(list_idx, vertices, num_vertices, bm0, bm1, state, pass_id);
    },
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

void obj_mesh_lighting_alloc_one(rf::Object *objp)
{
    // Note: ObjDeleteMesh frees mesh_lighting_data
    assert(objp->mesh_lighting_data == nullptr);
    auto size = rf::vmesh_calc_lighting_data_size(objp->vmesh);
    objp->mesh_lighting_data = rf::Malloc(size);
}

void obj_mesh_lighting_update_one(rf::Object *objp)
{
    gr_light_use_static(true);
    rf::vmesh_update_lighting_data(objp->vmesh, objp->room, objp->pos, objp->orient, objp->mesh_lighting_data);
    gr_light_use_static(false);
}

FunHook<void()> obj_mesh_lighting_update_hook{
    0x0048B0E0,
    []() {
        xlog::trace("update_mesh_lighting_hook");
        // Init transform for lighting calculation
        auto& gr_view_matrix = addr_as_ref<rf::Matrix3>(0x018186C8);
        auto& gr_view_pos = addr_as_ref<rf::Vector3>(0x01818690);
        auto& light_matrix = addr_as_ref<rf::Matrix3>(0x01818A38);
        auto& light_base = addr_as_ref<rf::Vector3>(0x01818A28);
        gr_view_matrix.make_identity();
        gr_view_pos.zero();
        light_matrix.make_identity();
        light_base.zero();

        if (g_game_config.mesh_static_lighting) {
            // Enable static lights
            gr_light_use_static(true);
            // Calculate lighting for meshes now
            obj_mesh_lighting_update_hook.call_target();
            // Switch back to dynamic lights
            gr_light_use_static(false);
        }
        else {
            obj_mesh_lighting_update_hook.call_target();
        }
    },
};

FunHook<void()> obj_mesh_lighting_alloc_hook{
    0x0048B1D0,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            if (item.vmesh && !(item.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(item.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(item.vmesh);
                item.mesh_lighting_data = rf::Malloc(size);
            }
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            if (clutter.vmesh && !(clutter.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(clutter.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(clutter.vmesh);
                clutter.mesh_lighting_data = rf::Malloc(size);
            }
        }
    },
};

FunHook<void()> obj_mesh_lighting_free_hook{
    0x0048B370,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            rf::Free(item.mesh_lighting_data);
            item.mesh_lighting_data = nullptr;
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            rf::Free(clutter.mesh_lighting_data);
            clutter.mesh_lighting_data = nullptr;
        }
    },
};

FunHook<void(rf::Object*)> obj_delete_mesh_hook{
    0x00489FC0,
    [](rf::Object* objp) {
        obj_delete_mesh_hook.call_target(objp);
        if (objp->mesh_lighting_data) {
            rf::Free(objp->mesh_lighting_data);
            objp->mesh_lighting_data = nullptr;
        }
    },
};

void recalc_mesh_static_lighting()
{
    AddrCaller{0x0048B370}.c_call(); // CleanupMeshLighting
    AddrCaller{0x0048B1D0}.c_call(); // AllocBuffersForMeshLighting
    AddrCaller{0x0048B0E0}.c_call(); // UpdateMeshLighting
}

ConsoleCommand2 mesh_static_lighting_cmd{
    "mesh_static_lighting",
    []() {
        g_game_config.mesh_static_lighting = !g_game_config.mesh_static_lighting;
        g_game_config.save();
        recalc_mesh_static_lighting();
        rf::console_printf("Mesh static lighting is %s", g_game_config.mesh_static_lighting ? "enabled" : "disabled");
    },
    "Toggle mesh static lighting calculation",
};

ConsoleCommand2 nearest_texture_filtering_cmd{
    "nearest_texture_filtering",
    []() {
        g_game_config.nearest_texture_filtering = !g_game_config.nearest_texture_filtering;
        g_game_config.save();
        setup_texture_filtering();
        rf::console_printf("Nearest texture filtering is %s", g_game_config.nearest_texture_filtering ? "enabled" : "disabled");
    },
    "Toggle nearest texture filtering",
};

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
        regs.edx = mode.value;
    },
};

CodeInjection gr_d3d_init_load_library_injection{
    0x005459AE,
    [](auto& regs) {
        auto d3d8to9_path = get_module_dir(g_hmodule) + "\\d3d8to9.dll";
        xlog::info("Loading d3d8to9.dll: %s", d3d8to9_path.c_str());
        auto d3d8to9_module = LoadLibraryA(d3d8to9_path.c_str());
        if (d3d8to9_module) {
            regs.eax = reinterpret_cast<int32_t>(d3d8to9_module);
            regs.eip = 0x005459B9;
        }
    },
};

void apply_texture_patches();
void apply_font_patches();
void graphics_capture_init();
void graphics_capture_after_game_init();
void init_gamma();

void graphics_init()
{
    // Fix for "At least 8 MB of available video memory"
    write_mem<u8>(0x005460CD, asm_opcodes::jae_rel_short);

    // Set initial FPS limit
    write_mem<float>(0x005094CA, 1.0f / g_game_config.max_fps);

    if (g_game_config.wnd_mode != GameConfig::FULLSCREEN) {
        // Enable windowed mode
        write_mem<u32>(0x004B29A5 + 6, 0xC8);
        setup_stretched_window_patch.install();
    }

    // Disable keyboard hooks (they were supposed to block alt-tab; they does not work in modern OSes anyway)
    write_mem<u8>(0x00524C98, asm_opcodes::jmp_rel_short);

#if D3D_SWAP_DISCARD
    // Use Discard Swap Mode
    write_mem<u32>(0x00545B4D + 6, D3DSWAPEFFECT_DISCARD); // full screen
    write_mem<u32>(0x00545AE3 + 6, D3DSWAPEFFECT_DISCARD); // windowed
#endif

    // Update D3D present paremeters and misc flags
    update_pp_hook.install();

    // Use hardware vertex processing if supported
#if D3D_HW_VERTEX_PROCESSING
    d3d_behavior_flags_patch.install();
    d3d_vertex_buffer_usage_patch.install();
    d3d_index_buffer_usage_patch.install();
#endif

    // nVidia issue fix (make sure D3Dsc is enabled)
    write_mem<u8>(0x00546154, 1);

    // Properly restore state after device reset
    gr_d3d_init_buffers_gr_d3d_flip_hook.install();

    // Fix FOV for widescreen
    AsmWriter(0x00547354, 0x00547358).nop();
    gr_d3d_setup_3d_widescreen_fix.install();
    write_mem<float>(0x0058A29C, 0.0003f); // factor related to near plane, default is 0.000588f

    // Don't use LOD models
    if (g_game_config.disable_lod_models) {
        // write_mem<u8>(0x00421A40, asm_opcodes::jmp_rel_short);
        write_mem<u8>(0x0052FACC, asm_opcodes::jmp_rel_short);
    }

    // Better error message in case of device creation error
    gr_d3d_init_error_patch.install();

    // Optimization - remove unused back buffer locking/unlocking in GrSwapBuffers
    AsmWriter(0x0054504A).jmp(0x0054508B);

#if 1
    // Fix rendering of right and bottom edges of viewport
    write_mem<u8>(0x00431D9F, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x00431F6B, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004328CF, asm_opcodes::jmp_rel_short);
    AsmWriter(0x0043298F).jmp(0x004329DC);
    gr_zbuffer_clear_fix_rect.install();

    // Left and top viewport edge fix for MSAA (RF does similar thing in GrDrawTextureD3D)
    write_mem<u8>(0x005478C6, asm_opcodes::fadd);
    write_mem<u8>(0x005478D7, asm_opcodes::fadd);
    write_mem_ptr(0x005478C6 + 2, &g_gr_clipped_geom_offset_x);
    write_mem_ptr(0x005478D7 + 2, &g_gr_clipped_geom_offset_y);
    gr_rect_gr_tmapper_hook.install();
#endif

    if (g_game_config.high_scanner_res) {
        // Improved Railgun Scanner resolution
        constexpr int8_t scanner_resolution = 120;        // default is 64, max is 127 (signed byte)
        write_mem<u8>(0x004325E6 + 1, scanner_resolution); // RenderInGame
        write_mem<u8>(0x004325E8 + 1, scanner_resolution);
        write_mem<u8>(0x004A34BB + 1, scanner_resolution); // PlayerCreate
        write_mem<u8>(0x004A34BD + 1, scanner_resolution);
        write_mem<u8>(0x004ADD70 + 1, scanner_resolution); // PlayerRenderRailgunScannerViewToTexture
        write_mem<u8>(0x004ADD72 + 1, scanner_resolution);
        write_mem<u8>(0x004AE0B7 + 1, scanner_resolution);
        write_mem<u8>(0x004AE0B9 + 1, scanner_resolution);
        write_mem<u8>(0x004AF0B0 + 1, scanner_resolution); // PlayerRenderScannerView
        write_mem<u8>(0x004AF0B4 + 1, scanner_resolution * 3 / 4);
        write_mem<u8>(0x004AF0B6 + 1, scanner_resolution);
        write_mem<u8>(0x004AF7B0 + 1, scanner_resolution);
        write_mem<u8>(0x004AF7B2 + 1, scanner_resolution);
        write_mem<u8>(0x004AF7CF + 1, scanner_resolution);
        write_mem<u8>(0x004AF7D1 + 1, scanner_resolution);
        write_mem<u8>(0x004AF818 + 1, scanner_resolution);
        write_mem<u8>(0x004AF81A + 1, scanner_resolution);
        write_mem<u8>(0x004AF820 + 1, scanner_resolution);
        write_mem<u8>(0x004AF822 + 1, scanner_resolution);
        write_mem<u8>(0x004AF855 + 1, scanner_resolution);
        write_mem<u8>(0x004AF860 + 1, scanner_resolution * 3 / 4);
        write_mem<u8>(0x004AF862 + 1, scanner_resolution);
    }

    // Always transfer entire framebuffer to entire window in Present call
    write_mem<u8>(0x00544FE6, asm_opcodes::jmp_rel_short);

    // Init True Color improvements
    gr_color_init();

    // Patch texture handling
    apply_texture_patches();

    // Apply bmpman patches
    bm_apply_patches();

    // Fonts
    apply_font_patches();

    // Back-buffer capture or render to texture related code
    graphics_capture_init();

    // Gamma related code
    init_gamma();

    // Enable mip-mapping for textures bigger than 256x256
    AsmWriter(0x0050FEDA, 0x0050FEE9).nop();
    write_mem<u8>(0x0055B739, asm_opcodes::jmp_rel_short);

    // Fix decal fade out
    write_mem<u8>(0x00560994 + 1, 3);

    // fullscreen/windowed commands
    switch_d3d_mode_patch.install();
    fullscreen_cmd.register_cmd();
    windowed_cmd.register_cmd();

    // Other commands
    antialiasing_cmd.register_cmd();
    nearest_texture_filtering_cmd.register_cmd();

    // Do not flush drawing buffers during gr_set_color_rgba call
    write_mem<u8>(0x0050CFEB, asm_opcodes::jmp_rel_short);

    // Flush instead of preparing D3D drawing buffers in gr_d3d_set_state, gr_d3d_set_state_and_texture, gr_d3d_tcache_set
    // Note: By default RF calls gr_d3d_tcache_set for many textures during level load process. It causes D3D buffers
    // to be locked and unlocked multiple times with discard flag. For some driver implementations (e.g. DXVK) such
    // behaviour leads to allocation of new vertex/index buffers during each lock and in the end it can cause end of
    // memory error.
    AsmWriter(0x0054F1A9).call(rf::gr_d3d_flush_buffers);
    AsmWriter(0x0055088A).call(rf::gr_d3d_flush_buffers);
    AsmWriter(0x0055CAE9).call(rf::gr_d3d_flush_buffers);
    AsmWriter(0x0055CB77).call(rf::gr_d3d_flush_buffers);
    AsmWriter(0x0055CB58).call(rf::gr_d3d_flush_buffers);

    // Do not check if buffers are locked before preparing them in gr_d3d_tmapper
    AsmWriter(0x0055191D, 0x00551925).nop();

    // Call gr_d3d_prepare_buffers when buffers are not locked or primitive type is not matching
    // This is needed after removal of gr_d3d_prepare_buffers calls from
    // gr_d3d_set_state, gr_d3d_set_state_and_texture and gr_d3d_tcache_set
    gr_d3d_bitmap_patch_1.install();
    gr_d3d_bitmap_patch_2.install();
    gr_d3d_line_patch_1.install();
    gr_d3d_line_patch_2.install();
    gr_d3d_line_vertex_internal_patch_1.install();
    gr_d3d_line_vertex_internal_patch_2.install();
    gr_d3d_tmapper_patch.install();
    gr_d3d_draw_geometry_face_patch_1.install();
    gr_d3d_draw_geometry_face_patch_2.install();

    // Do not flush D3D buffers explicity in gr_d3d_line_vertex_internal and gr_d3d_line
    // Buffers will be flushed if needed by gr_d3d_prepare_buffers
    AsmWriter(0x00551474).nop(5);
    AsmWriter(0x005516F5).nop(5);

#ifdef DEBUG
    profile_frame_cmd.register_cmd();
#endif

    // Render rocket launcher scanner image every frame
    // addr_as_ref<bool>(0x5A1020) = 0;

    after_gr_init_hook.install();

    // Fix crash when loading very big TGA files
    load_tga_alloc_fail_fix.install();

    // Fix memory leak if texture cannot be created
    gr_d3d_create_vram_texture_with_mipmaps_hook.install();
    gr_d3d_create_texture_fail_hook.install();

    // Fix font texture leak
    // Original code sets bitmap handle in all fonts to -1 on level unload. On next font usage the font bitmap is reloaded.
    // Note: font bitmaps are dynamic (USERBMAP) so they cannot be found by name unlike normal bitmaps.
    AsmWriter(0x0050E1A8).ret();
    GrLoadFontInternal_fix_texture_ref.install();

    // maxfps command
    max_fps_cmd.register_cmd();

    // Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails)
    gr_d3d_lock_crash_fix.install();

    // Fix undefined behavior in D3D state handling: alpha operations cannot be disabled when color operations are enabled
    write_mem<u8>(0x0054F785 + 1, D3DTOP_SELECTARG2);
    write_mem<u8>(0x0054FF18 + 1, D3DTOP_SELECTARG2);

    // Fix details and liquids rendering in railgun scanner. They were unnecessary modulated with very dark green color,
    // which seems to be a left-over from an old implementationof railgun scanner green overlay (currently it is
    // handled by drawing a green rectangle after all the geometry is drawn)
    write_mem<u8>(0x004D33F3, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x004D410D, asm_opcodes::jmp_rel_short);

    // Support disabling of damage screen flash effect
    DoDamageScreenFlash_hook.install();
    damage_screen_flash_cmd.register_cmd();

    // Fix glass_house level having faces that use multi-textured state but don't have any lightmap (stage 1 texture
    // from previous drawing operation was used)
    gr_d3d_queue_triangles_hook.install();

    // Fix/improve items and clutters static lighting calculation: fix matrices being zero and use static lights
    obj_mesh_lighting_update_hook.install();
    obj_mesh_lighting_alloc_hook.install();
    obj_mesh_lighting_free_hook.install();
    obj_delete_mesh_hook.install();
    mesh_static_lighting_cmd.register_cmd();

    // Fix invalid vertex offset in mesh lighting calculation
    write_mem<i8>(0x005042F0 + 2, sizeof(rf::Vector3));

    // Make d3d_zm variable not dependent on fov to fix wall-peeking
    AsmWriter(0x0054715E).nop(2);

    // Use gr_clear instead of gr_rect for faster drawing of the fog background
    AsmWriter(0x00431F99).call(0x0050CDF0);

    // Support textures with alpha channel in Display_Fullscreen_Image event
    display_full_screen_image_alpha_support_patch.install();

    // Fix flamethrower "stroboscopic effect" on high FPS
    // gr_3d_bitmap_angle interprets parameters differently than gr_3d_bitmap_stretched_square expects - zero angle does not use
    // diamond shape and size is not divided by 2. Instead of calling it when p0 to p1 distance is small get rid of this
    // special case. gr_3d_bitmap_stretched_square should handle it properly even if distance is 0 because it
    // uses Vector3::normalize_safe() API.
    write_mem<u8>(0x00558E61, asm_opcodes::jmp_rel_short);

    // Use d3d8to9 instead of d3d8
    gr_d3d_init_load_library_injection.install();
}

void graphics_after_game_init()
{
    graphics_capture_after_game_init();

}

void graphics_draw_fps_counter()
{
    if (g_game_config.fps_counter && !rf::is_hud_hidden) {
        auto text = string_format("FPS: %.1f", rf::current_fps);
        rf::gr_set_color_rgba(0, 255, 0, 255);
        int x = rf::gr_screen_width() - (g_game_config.big_hud ? 165 : 90);
        int y = 10;
        if (rf::gameseq_in_gameplay()) {
            y = g_game_config.big_hud ? 110 : 60;
            if (is_double_ammo_hud()) {
                y += g_game_config.big_hud ? 80 : 40;
            }
        }

        int font_id = hud_get_default_font();
        rf::gr_string(x, y, text.c_str(), font_id);
    }
}

bool gr_d3d_is_d3d8to9()
{
    if (rf::gr_screen.mode != rf::GR_DIRECT3D) {
        return false;
    }
    static bool is_d3d9 = false;
    static bool is_d3d9_inited = false;
    if (!is_d3d9_inited) {
        static const GUID IID_IDirect3D9 = {0x81BDCBCA, 0x64D4, 0x426D, {0xAE, 0x8D, 0xAD, 0x01, 0x47, 0xF4, 0x27, 0x5C}};
        ComPtr<IUnknown> d3d9;
        is_d3d9 = SUCCEEDED(rf::gr_d3d->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&d3d9)));
        is_d3d9_inited = true;
    }
    return is_d3d9;
}

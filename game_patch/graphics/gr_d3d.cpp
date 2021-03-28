#include <windows.h>
#include <d3d8.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ComPtr.h>
#include <common/error/d3d-error.h>
#include <common/config/BuildConfig.h>
#include <xlog/xlog.h>
#include <common/utils/os-utils.h>
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/gr/gr_direct3d.h"
#include "../rf/os/os.h"
#include "../rf/multi.h"
#include "../rf/player.h"
#include "../main/main.h"
#include "../os/console.h"
#include "gr.h"
#include "gr_internal.h"

static float g_gr_clipped_geom_offset_x = -0.5;
static float g_gr_clipped_geom_offset_y = -0.5;
static float gr_lod_dist_scale = 1.0f;

void gr_d3d_capture_device_lost();
void gr_d3d_capture_close();

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

FunHook<void(rf::Matrix3&, rf::Vector3&, float, bool, bool)> gr_d3d_setup_3d_hook{
    0x00547150,
    [](rf::Matrix3& viewer_orient, rf::Vector3& viewer_pos, float horizontal_fov, bool zbuffer_flag, bool z_scale) {
        gr_d3d_setup_3d_hook.call_target(viewer_orient, viewer_pos, horizontal_fov, zbuffer_flag, z_scale);
        // Half pixel offset
        g_gr_clipped_geom_offset_x = rf::gr::screen.offset_x - 0.5f;
        g_gr_clipped_geom_offset_y = rf::gr::screen.offset_y - 0.5f;
        auto& gr_viewport_center_x = addr_as_ref<float>(0x01818B54);
        auto& gr_viewport_center_y = addr_as_ref<float>(0x01818B5C);
        gr_viewport_center_x -= 0.5f; // viewport center x
        gr_viewport_center_y -= 0.5f; // viewport center y
        if (z_scale) {
            constexpr float pi = 3.141592f;
            rf::gr::d3d::zm = 1 / 1700.0f;
            // zm is proportional to near clipping plane
            // Make sure zm is not increased when fov is getting smaller than 90 to fix wall peeking with scope weapons
            // Make sure zm is decreased when fov is increased to fix clipping issues on a very high fov
            float hfov_div = std::tan(horizontal_fov / 2.0f / 180.0f * pi);
            if (hfov_div > 1.0f) {
                rf::gr::d3d::zm /= hfov_div;
            }
            float clip_aspect = static_cast<float>(rf::gr::screen.clip_width) / rf::gr::screen.clip_height;
            float vfov_div = hfov_div / clip_aspect;
            if (vfov_div > 1.0f) {
                rf::gr::d3d::zm /= vfov_div;
            }
        }
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

        hr = rf::gr::d3d::d3d->CheckDeviceType(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, rf::gr::d3d::pp.BackBufferFormat,
            rf::gr::d3d::pp.BackBufferFormat, rf::gr::d3d::pp.Windowed);
        if (FAILED(hr)) {
            xlog::error("CheckDeviceType for format %d failed: %lX", rf::gr::d3d::pp.BackBufferFormat, hr);
        }

        hr = rf::gr::d3d::d3d->CheckDeviceFormat(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, rf::gr::d3d::pp.BackBufferFormat,
            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, rf::gr::d3d::pp.AutoDepthStencilFormat);
        if (FAILED(hr)) {
            xlog::error("CheckDeviceFormat for depth-stencil format %d failed: %lX", rf::gr::d3d::pp.AutoDepthStencilFormat, hr);
        }

        if (!(rf::gr::d3d::device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
            xlog::error("No T&L hardware support!");
        }

        xlog::info("Supported adapter modes:");
        unsigned num_modes = rf::gr::d3d::d3d->GetAdapterModeCount(rf::gr::d3d::adapter_idx);
        D3DDISPLAYMODE mode;
        for (unsigned i = 0; i < num_modes; ++i) {
            rf::gr::d3d::d3d->EnumAdapterModes(rf::gr::d3d::adapter_idx, i, &mode);
            xlog::info("- %ux%u (format %u refresh rate %u)", mode.Width, mode.Height, mode.Format, mode.RefreshRate);
        }

        ShowWindow(rf::main_wnd, SW_HIDE);
        MessageBoxA(nullptr, text.c_str(), "Error!", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TASKMODAL);
        ExitProcess(-1);
    },
};

CodeInjection gr_d3d_zbuffer_clear_fix_rect{
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

        if (SUCCEEDED(rf::gr::d3d::d3d->CheckDeviceFormat(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, adapter_format, D3DUSAGE_DEPTHSTENCIL,
                                                    D3DRTYPE_SURFACE, depth_fmt)) &&
            SUCCEEDED(rf::gr::d3d::d3d->CheckDepthStencilMatch(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, adapter_format, adapter_format,
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

        xlog::info("Back Buffer format: %u", rf::gr::d3d::pp.BackBufferFormat);
        xlog::info("Back Buffer dimensions: %ux%u", rf::gr::d3d::pp.BackBufferWidth, rf::gr::d3d::pp.BackBufferHeight);
        xlog::info("D3D Device Caps: %lX", rf::gr::d3d::device_caps.DevCaps);
        xlog::info("D3D Raster Caps: %lX", rf::gr::d3d::device_caps.RasterCaps);
        xlog::info("Max texture size: %ldx%ld", rf::gr::d3d::device_caps.MaxTextureWidth, rf::gr::d3d::device_caps.MaxTextureHeight);

        if (g_game_config.msaa) {
            // Make sure selected MSAA mode is available
            auto multi_sample_type = static_cast<D3DMULTISAMPLE_TYPE>(g_game_config.msaa.value());
            HRESULT hr = rf::gr::d3d::d3d->CheckDeviceMultiSampleType(rf::gr::d3d::adapter_idx, D3DDEVTYPE_HAL, rf::gr::d3d::pp.BackBufferFormat,
                                                                rf::gr::d3d::pp.Windowed, multi_sample_type);
            if (SUCCEEDED(hr)) {
                xlog::info("Enabling Anti-Aliasing (%ux MSAA)...", g_game_config.msaa.value());
                rf::gr::d3d::pp.MultiSampleType = multi_sample_type;
            }
            else {
                xlog::warn("MSAA not supported (0x%lx)...", hr);
                g_game_config.msaa = D3DMULTISAMPLE_NONE;
            }
        }

        // remove D3DPRESENTFLAG_LOCKABLE_BACKBUFFER flag
        rf::gr::d3d::pp.Flags = 0;
#if D3D_LOCKABLE_BACKBUFFER
        // Note: if MSAA is used backbuffer cannot be lockable
        if (g_game_config.msaa == D3DMULTISAMPLE_NONE)
            rf::gr::d3d::pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#endif

        // Use glorad detail map on all cards (multi-texturing support is required)
        auto& use_glorad_detail_map = addr_as_ref<bool>(0x01CFCBCC);
        use_glorad_detail_map = true;

        // Override depth format to avoid card specific hackfixes that makes it different on Nvidia and AMD
        rf::gr::d3d::pp.AutoDepthStencilFormat = determine_depth_buffer_format(rf::gr::d3d::pp.BackBufferFormat);

        gr_d3d_texture_init();
    },
};

#if D3D_HW_VERTEX_PROCESSING

CodeInjection d3d_behavior_flags_patch{
    0x00545BE0,
    [](auto& regs) {
        auto& behavior_flags = *static_cast<u32*>(regs.esp);
        // Use hardware vertex processing instead of software processing if supported
        if (rf::gr::d3d::device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
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
        if (rf::gr::d3d::device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
            usage &= ~D3DUSAGE_SOFTWAREPROCESSING;
        }
    },
};

CodeInjection d3d_index_buffer_usage_patch{
    0x0054511C,
    [](auto& regs) {
        auto& usage = *static_cast<u32*>(regs.esp);
        if (rf::gr::d3d::device_caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
            usage &= ~D3DUSAGE_SOFTWAREPROCESSING;
        }
    },
};

#endif // D3D_HW_VERTEX_PROCESSING

void setup_texture_filtering()
{
    if (g_game_config.nearest_texture_filtering) {
        // use linear filtering for lightmaps because otherwise it looks bad
        set_texture_min_mag_filter_in_code(D3DTEXF_POINT, D3DTEXF_LINEAR);
    }
    else if (rf::gr::d3d::device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering && !rf::is_dedicated_server) {
        // Anisotropic texture filtering
        set_texture_min_mag_filter_in_code(D3DTEXF_ANISOTROPIC, D3DTEXF_ANISOTROPIC);
    } else {
        set_texture_min_mag_filter_in_code(D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }
}

DWORD setup_max_anisotropy()
{
    DWORD anisotropy_level = std::min(rf::gr::d3d::device_caps.MaxAnisotropy, 16ul);
    rf::gr::d3d::device->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, anisotropy_level);
    rf::gr::d3d::device->SetTextureStageState(1, D3DTSS_MAXANISOTROPY, anisotropy_level);
    return anisotropy_level;
}

CallHook<void()> gr_d3d_init_buffers_gr_d3d_flip_hook{
    0x00545045,
    []() {
        gr_d3d_init_buffers_gr_d3d_flip_hook.call_target();

        // Apply state change after reset
        // Note: we dont have to set min/mag filtering because its set when selecting material

        rf::gr::d3d::device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        rf::gr::d3d::device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        rf::gr::d3d::device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        rf::gr::d3d::device->SetRenderState(D3DRS_AMBIENT, 0xFF545454);
        rf::gr::d3d::device->SetRenderState(D3DRS_CLIPPING, FALSE);

        if (rf::local_player)
            rf::gr::set_texture_mip_filter(!rf::local_player->settings.bilinear_filtering);

        if (rf::gr::d3d::device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering)
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
        rf::gr::d3d::pp.Windowed = false;
        rf::gr::d3d::pp.FullScreen_PresentationInterval =
            g_game_config.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        g_reset_device_req = true;
    },
};

ConsoleCommand2 windowed_cmd{
    "windowed",
    []() {
        rf::gr::d3d::pp.Windowed = true;
        rf::gr::d3d::pp.FullScreen_PresentationInterval = 0;
        g_reset_device_req = true;
    },
};

ConsoleCommand2 antialiasing_cmd{
    "antialiasing",
    []() {
        if (!g_game_config.msaa)
            rf::console::printf("Anti-aliasing is not supported");
        else {
            DWORD enabled = 0;
            rf::gr::d3d::device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &enabled);
            enabled = !enabled;
            rf::gr::d3d::device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, enabled);
            rf::console::printf("Anti-aliasing is %s", enabled ? "enabled" : "disabled");
        }
    },
    "Toggles anti-aliasing",
};

// frame profiling
#ifdef DEBUG

static bool g_profile_frame_req = false;
static bool g_profile_frame = false;
static int g_num_draw_calls = 0;

CodeInjection gr_d3d_set_state_profile_patch{
    0x0054F19C,
    [](auto& regs) {
        if (g_profile_frame) {
            auto mode = addr_as_ref<rf::gr::Mode>(regs.esp + 0x10 + 0x4);
            const char* desc = "";
            if (mode == rf::gr::string_mode)
                desc = " (text)";
            else if (mode == rf::gr::rect_mode)
                desc = " (rect)";
            else if (mode == rf::gr::line_mode)
                desc = " (line)";
            else if (mode == rf::gr::bitmap_clamp_mode)
                desc = " (bitmap)";
            xlog::info("gr_d3d_set_state 0x%X%s", static_cast<int>(mode), desc);
        }
    },
};

CodeInjection gr_d3d_set_texture_profile_patch{
    0x0055CB6A,
    [](auto& regs) {
        if (g_profile_frame) {
            unsigned bm_handle = regs.edi;
            int stage_id = (regs.ebx - 0x01E65308) / 0x18;
            xlog::info("gr_d3d_set_texture %d 0x%X %s", stage_id, bm_handle, rf::bm::get_filename(bm_handle));
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

FunHook<void()> gr_d3d_flip_profile_patch{
    0x00544FC0,
    []() {
        gr_d3d_flip_profile_patch.call_target();

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
            gr_d3d_set_state_profile_patch.install();
            gr_d3d_set_texture_profile_patch.install();
            gr_d3d_flip_profile_patch.install();
            auto d3d_dev_vtbl = *reinterpret_cast<uintptr_t**>(rf::gr::d3d::device);
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
        if (rf::gr::d3d::device_caps.MaxAnisotropy > 0 && g_game_config.anisotropic_filtering && !rf::is_dedicated_server) {
            DWORD anisotropy_level = setup_max_anisotropy();
            xlog::info("Anisotropic Filtering enabled (level: %lu)", anisotropy_level);
        }

        // Change font for Time Left text
        static int time_left_font = rf::gr::load_font("rfpc-large.vf");
        if (time_left_font >= 0) {
            write_mem<i8>(0x00477157 + 1, time_left_font);
            write_mem<i8>(0x0047715F + 2, 21);
            write_mem<i32>(0x00477168 + 1, 154);
        }

        // Fix performance issues caused by this field being initialized to inf
        rf::gr::screen.fog_far_scaled = 255.0f / rf::gr::screen.fog_far;
    },
};

FunHook<int(int, void*)> gr_d3d_create_vram_texture_with_mipmaps_hook{
    0x0055B700,
    [](int bm_handle, void* tex) {
        int result = gr_d3d_create_vram_texture_with_mipmaps_hook.call_target(bm_handle, tex);
        if (result != 1) {
            rf::bm::unlock(bm_handle);
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
        if (!rf::gr::d3d::buffers_locked || rf::gr::d3d::primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00550CEC;
        }
    },
};

CodeInjection gr_d3d_bitmap_patch_2{
    0x0055105F,
    [](auto& regs) {
        if (!rf::gr::d3d::buffers_locked || rf::gr::d3d::primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x0055107B;
        }
    },
};

CodeInjection gr_d3d_line_patch_1{
    0x0055147D,
    [](auto& regs) {
        bool flush_needed = !rf::gr::d3d::buffers_locked
                         || rf::gr::d3d::primitive_type != D3DPT_LINELIST
                         || rf::gr::d3d::max_hw_vertex + 2 > 6000
                         || rf::gr::d3d::max_hw_index + rf::gr::d3d::num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x00551482;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 rf::gr::d3d::buffers_locked, rf::gr::d3d::primitive_type, rf::gr::d3d::max_hw_vertex,
                 rf::gr::d3d::max_hw_index + rf::gr::d3d::num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_patch_2{
    0x005515B2,
    []() {
        rf::gr::d3d::num_vertices += 2;
    },
};

CodeInjection gr_d3d_line_vertex_internal_patch_1{
    0x005516FE,
    [](auto& regs) {
        bool flush_needed = !rf::gr::d3d::buffers_locked
                         || rf::gr::d3d::primitive_type != D3DPT_LINELIST
                         || rf::gr::d3d::max_hw_vertex + 2 > 6000
                         || rf::gr::d3d::max_hw_index + rf::gr::d3d::num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x00551703;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 rf::gr::d3d::buffers_locked, rf::gr::d3d::primitive_type, rf::gr::d3d::max_hw_vertex,
                 rf::gr::d3d::max_hw_index + rf::gr::d3d::num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_vertex_internal_patch_2{
    0x005518E8,
    []() {
        rf::gr::d3d::num_vertices += 2;
    },
};

CodeInjection gr_d3d_tmapper_patch{
    0x0055193B,
    [](auto& regs) {
        if (!rf::gr::d3d::buffers_locked || rf::gr::d3d::primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551958;
        }
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_1{
    0x00551EF5,
    [](auto& regs) {
        if (!rf::gr::d3d::buffers_locked || rf::gr::d3d::primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551F13;
        }
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_2{
    0x00551CB9,
    [](auto& regs) {
        if (!rf::gr::d3d::buffers_locked || rf::gr::d3d::primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x00551CD6;
        }
    },
};

FunHook<void(int*, void*, int, int, int, rf::gr::Mode, int)> gr_d3d_queue_triangles_hook{
    0x005614B0,
    [](int *list_idx, void *vertices, int num_vertices, int bm0, int bm1, rf::gr::Mode mode, int pass_id) {
        // Fix glass_house level having faces that use MT state but don't have any lightmap
        if (bm1 == -1) {
            INFO_ONCE("Prevented rendering of an unlit face with multi-texturing render state");
            rf::gr::TextureSource ts = mode.get_texture_source();
            if (ts == rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0 || ts == rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X) {
                ts = rf::gr::TEXTURE_SOURCE_WRAP;
            } else if (ts == rf::gr::TEXTURE_SOURCE_CLAMP_1_CLAMP_0) {
                ts = rf::gr::TEXTURE_SOURCE_CLAMP;
            }
            mode.set_texture_source(ts);
        }

        gr_d3d_queue_triangles_hook.call_target(list_idx, vertices, num_vertices, bm0, bm1, mode, pass_id);
    },
};

ConsoleCommand2 nearest_texture_filtering_cmd{
    "nearest_texture_filtering",
    []() {
        g_game_config.nearest_texture_filtering = !g_game_config.nearest_texture_filtering;
        g_game_config.save();
        setup_texture_filtering();
        rf::console::printf("Nearest texture filtering is %s", g_game_config.nearest_texture_filtering ? "enabled" : "disabled");
    },
    "Toggle nearest texture filtering",
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

CodeInjection gr_d3d_device_lost_injection{
    0x00545042,
    []() {
        void monitor_refresh_all();

        xlog::trace("D3D device lost");
        gr_d3d_capture_device_lost();
        gr_d3d_texture_device_lost();
        monitor_refresh_all();
    },
};

CodeInjection gr_d3d_close_injection{
    0x0054527A,
    []() {
        gr_d3d_capture_close();
        gr_d3d_gamma_reset();
    },
};

FunHook<float(const rf::Vector3&)> gr_get_apparent_distance_from_camera_hook{
    0x005182F0,
    [](const rf::Vector3& pos) {
        return gr_get_apparent_distance_from_camera_hook.call_target(pos) / gr_lod_dist_scale;
    },
};

ConsoleCommand2 lod_distance_scale_cmd{
    "lod_distance_scale",
    [](std::optional<float> scale_opt) {
        if (scale_opt.has_value()) {
            gr_lod_dist_scale = scale_opt.value();
        }
        rf::console::printf("LOD distance scale: %.2f", gr_lod_dist_scale);
    },
    "Sets LOD distance scale factor",
};

ConsoleCommand2 pow2_tex_cmd{
    "pow2_tex",
    []() {
        rf::gr::d3d::p2t = !rf::gr::d3d::p2t;
        rf::console::printf("Power of 2 textures: %s", rf::gr::d3d::p2t ? "on" : "off");
    },
    "Forces usage of power of two textures. It may fix UV mappings in old levels. Onyl levels loaded after usage of this command are affected.",
};

bool gr_d3d_is_d3d8to9()
{
    if (rf::gr::screen.mode != rf::gr::DIRECT3D) {
        return false;
    }
    static bool is_d3d9 = false;
    static bool is_d3d9_inited = false;
    if (!is_d3d9_inited) {
        static const GUID IID_IDirect3D9 = {0x81BDCBCA, 0x64D4, 0x426D, {0xAE, 0x8D, 0xAD, 0x01, 0x47, 0xF4, 0x27, 0x5C}};
        ComPtr<IUnknown> d3d9;
        is_d3d9 = SUCCEEDED(rf::gr::d3d::d3d->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&d3d9)));
        is_d3d9_inited = true;
    }
    return is_d3d9;
}

void gr_d3d_apply_patch()
{
    // Fix for "At least 8 MB of available video memory"
    write_mem<u8>(0x005460CD, asm_opcodes::jae_rel_short);

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

    // nVidia issue fix - make sure D3Dsc (D3D software clipping) is enabled
    write_mem<u8>(0x00546154, 1);

    // Properly restore state after device reset
    gr_d3d_init_buffers_gr_d3d_flip_hook.install();

    // Increase mesh details
    gr_get_apparent_distance_from_camera_hook.install();
    if (g_game_config.disable_lod_models) {
        // Do not scale LOD distance for character in multi
        AsmWriter{0x0052FAED, 0x0052FAFB}.nop();
        // Change default LOD scale
        gr_lod_dist_scale = 10.0f;
    }

    // Better error message in case of device creation error
    gr_d3d_init_error_patch.install();

    // Optimization - remove unused back buffer locking/unlocking in gr_d3d_flip
    AsmWriter(0x0054504A).jmp(0x0054508B);

    // Fix rendering of right and bottom edges of viewport (part 2)
    gr_d3d_zbuffer_clear_fix_rect.install();

    // Half pixel offset fix that resolves issues on the screen corner when MSAA is enabled
    write_mem<u8>(0x005478C6, asm_opcodes::fadd);
    write_mem<u8>(0x005478D7, asm_opcodes::fadd);
    write_mem_ptr(0x005478C6 + 2, &g_gr_clipped_geom_offset_x);
    write_mem_ptr(0x005478D7 + 2, &g_gr_clipped_geom_offset_y);
    gr_d3d_setup_3d_hook.install();

    // Always transfer entire framebuffer to entire window in Present call in gr_d3d_flip
    write_mem<u8>(0x00544FE6, asm_opcodes::jmp_rel_short);

    // Enable mip-mapping for textures bigger than 256x256 in gr_d3d_create_vram_texture_with_mipmaps
    write_mem<u8>(0x0055B739, asm_opcodes::jmp_rel_short);

    // Fix decal fade out in gr_d3d_render_static_solid
    write_mem<u8>(0x00560994 + 1, 3);

    // Flush instead of preparing D3D drawing buffers in gr_d3d_set_state, gr_d3d_set_state_and_texture, gr_d3d_tcache_set
    // Note: By default RF calls gr_d3d_tcache_set for many textures during level load process. It causes D3D buffers
    // to be locked and unlocked multiple times with discard flag. For some driver implementations (e.g. DXVK) such
    // behaviour leads to allocation of new vertex/index buffers during each lock and in the end it can cause end of
    // memory error.
    AsmWriter(0x0054F1A9).call(rf::gr::d3d::flush_buffers);
    AsmWriter(0x0055088A).call(rf::gr::d3d::flush_buffers);
    AsmWriter(0x0055CAE9).call(rf::gr::d3d::flush_buffers);
    AsmWriter(0x0055CB77).call(rf::gr::d3d::flush_buffers);
    AsmWriter(0x0055CB58).call(rf::gr::d3d::flush_buffers);

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

    // Support switching D3D present parameters in gr_d3d_flip to support fullscreen/windowed commands
    switch_d3d_mode_patch.install();

    after_gr_init_hook.install();

    // Fix memory leak if texture cannot be created
    gr_d3d_create_vram_texture_with_mipmaps_hook.install();
    gr_d3d_create_texture_fail_hook.install();

    // Crash-fix in case texture has not been created (this happens if gr_read_back_buffer fails)
    gr_d3d_lock_crash_fix.install();

    // Fix undefined behavior in D3D state handling: alpha operations cannot be disabled when color operations are enabled
    write_mem<u8>(0x0054F785 + 1, D3DTOP_SELECTARG2);
    write_mem<u8>(0x0054FF18 + 1, D3DTOP_SELECTARG2);

    // Fix glass_house level having faces that use multi-textured state but don't have any lightmap (stage 1 texture
    // from previous drawing operation was used)
    gr_d3d_queue_triangles_hook.install();

    // Interpret horizontal fov values below 2 as degrees in gr_d3d_setup_3d
    write_mem<u8>(0x00547257, asm_opcodes::jmp_rel_short);

    // Fix flamethrower "stroboscopic effect" on high FPS
    // gr_3d_bitmap_angle interprets parameters differently than gr_3d_bitmap_stretched_square expects - zero angle does not use
    // diamond shape and size is not divided by 2. Instead of calling it when p0 to p1 distance is small get rid of this
    // special case. gr_3d_bitmap_stretched_square should handle it properly even if distance is 0 because it
    // uses Vector3::normalize_safe() API.
    write_mem<u8>(0x00558E61, asm_opcodes::jmp_rel_short);

    // Use d3d8to9 instead of d3d8
    gr_d3d_init_load_library_injection.install();

    // Release default pool resources
    gr_d3d_device_lost_injection.install();
    gr_d3d_close_injection.install();

    // Commands
    fullscreen_cmd.register_cmd();
    windowed_cmd.register_cmd();
    antialiasing_cmd.register_cmd();
    nearest_texture_filtering_cmd.register_cmd();
    lod_distance_scale_cmd.register_cmd();
    pow2_tex_cmd.register_cmd();
#ifdef DEBUG
    profile_frame_cmd.register_cmd();
#endif
}

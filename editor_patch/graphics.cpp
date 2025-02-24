#include <common/config/BuildConfig.h>
#include <common/utils/os-utils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/MemUtils.h>
#include <xlog/xlog.h>
#include <d3d8.h>
#include <algorithm>
#include <cmath>

HWND GetMainFrameHandle();

namespace red
{
    struct GrScreen
    {
        int signature;
        int max_width;
        int max_height;
        int mode;
        int window_mode;
        int field_14;
        float aspect;
        int field_1c;
        int bits_per_pixel;
        int bytes_ber_pixel;
        int field_28;
        int offset_x;
        int offset_y;
        int clip_width;
        int clip_height;
        int max_tex_width;
        int max_tex_height;
        int clip_left;
        int clip_right;
        int clip_top;
        int clip_bottom;
        int current_color;
        int current_bitmap;
        int current_bitmap2;
        int fog_mode;
        int fog_color;
        float fog_near;
        float fog_far;
        float fog_far_scaled;
        bool recolor_enabled;
        float recolor_red;
        float recolor_green;
        float recolor_blue;
        int field_84;
        int field_88;
        int zbuffer_mode;
    };
    static_assert(sizeof(GrScreen) == 0x90);

    struct Vector3;
    struct Matrix3;

    auto& gr_d3d_buffers_locked = addr_as_ref<bool>(0x0183930D);
    auto& gr_d3d_primitive_type = addr_as_ref<D3DPRIMITIVETYPE>(0x0175A2C8);
    auto& gr_d3d_max_hw_vertex = addr_as_ref<int>(0x01621FA8);
    auto& gr_d3d_max_hw_index = addr_as_ref<int>(0x01621FAC);
    auto& gr_d3d_num_vertices = addr_as_ref<int>(0x01839310);
    auto& gr_d3d_num_indices = addr_as_ref<int>(0x01839314);
    auto& gr_screen = addr_as_ref<GrScreen>(0x014CF748);

}

CallHook<void()> frametime_calculate_hook{
    0x00483047,
    []() {
        HWND hwnd = GetMainFrameHandle();
        if (!IsWindowVisible(hwnd)) {
            // when minimized limit to 1 FPS
            Sleep(1000);
        }
        else {
            // Check if editor is the foreground window
            HWND foreground_wnd = GetForegroundWindow();
            bool editor_is_in_foreground = GetWindowThreadProcessId(foreground_wnd, nullptr) == GetCurrentThreadId();
            // when editor is in background limit to 4 FPS
            if (!editor_is_in_foreground) {
                Sleep(250);
            }
        }
        frametime_calculate_hook.call_target();
    },
};

CodeInjection after_gr_init_hook{
    0x004B8D4D,
    []() {
        // Fix performance issues caused by this field being initialized to inf
        red::gr_screen.fog_far_scaled = 255.0f / red::gr_screen.fog_far;
    },
};

CodeInjection gr_d3d_line_3d_patch_1{
    0x004E133E,
    [](auto& regs) {
        bool flush_needed = !red::gr_d3d_buffers_locked
                         || red::gr_d3d_primitive_type != D3DPT_LINELIST
                         || red::gr_d3d_max_hw_vertex + 2 > 6000
                         || red::gr_d3d_max_hw_index + red::gr_d3d_num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x004E1343;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers {} {} {} {}",
                 red::gr_d3d_buffers_locked, red::gr_d3d_primitive_type, red::gr_d3d_max_hw_vertex,
                 red::gr_d3d_max_hw_index + red::gr_d3d_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_3d_patch_2{
    0x004E1528,
    []() {
        red::gr_d3d_num_vertices += 2;
    },
};

CodeInjection gr_d3d_line_2d_patch_1{
    0x004E10BD,
    [](auto& regs) {
        bool flush_needed = !red::gr_d3d_buffers_locked
                         || red::gr_d3d_primitive_type != D3DPT_LINELIST
                         || red::gr_d3d_max_hw_vertex + 2 > 6000
                         || red::gr_d3d_max_hw_index + red::gr_d3d_num_indices + 2 > 10000;
        if (!flush_needed) {
            xlog::trace("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x004E10C2;
        }
        else {
            xlog::trace("Line drawing requires gr_d3d_prepare_buffers {} {} {} {}",
                 red::gr_d3d_buffers_locked, red::gr_d3d_primitive_type, red::gr_d3d_max_hw_vertex,
                 red::gr_d3d_max_hw_index + red::gr_d3d_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_line_2d_patch_2{
    0x004E11F2,
    []() {
        red::gr_d3d_num_vertices += 2;
    },
};

CodeInjection gr_d3d_poly_patch{
    0x004E1573,
    [](auto& regs) {
        if (red::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x004E1598;
        }
    },
};

CodeInjection gr_d3d_bitmap_patch_1{
    0x004E090E,
    [](auto& regs) {
        if (red::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x004E092C;
        }
    },
};

CodeInjection gr_d3d_bitmap_patch_2{
    0x004E0C97,
    [](auto& regs) {
        if (red::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x004E0CBB;
        }
    },
};

CodeInjection gr_d3d_render_geometry_face_patch_1{
    0x004E18F1,
    [](auto& regs) {
        if (red::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x004E1916;
        }
    },
};

CodeInjection gr_d3d_render_geometry_face_patch_2{
    0x004E1B2D,
    [](auto& regs) {
        if (red::gr_d3d_primitive_type != D3DPT_TRIANGLELIST) {
            regs.eip = 0x004E1B53;
        }
    },
};

CallHook<void(int, int, int, int, int, HWND, float, bool, int, D3DFORMAT)> gr_init_hook{
    0x00482B78,
    [](int max_w, int max_h, int bit_depth, int mode, int window_mode, HWND hwnd, float far_zvalue, bool sync_blit, int video_card, D3DFORMAT backbuffer_format) {
        max_w = GetSystemMetrics(SM_CXSCREEN);
        max_h = GetSystemMetrics(SM_CYSCREEN);
        gr_init_hook.call_target(max_w, max_h, bit_depth, mode, window_mode, hwnd, far_zvalue, sync_blit, video_card, backbuffer_format);
    },
};

CodeInjection gr_init_widescreen_patch{
    0x004B8CD1,
    []() {
        red::gr_screen.aspect = 1.0f;
    },
};

FunHook<void(red::Matrix3*, red::Vector3*, float, bool, bool)> gr_setup_3d_hook{
    0x004C5980,
    [](red::Matrix3* viewer_orient, red::Vector3* viewer_pos, float horizontal_fov, bool zbuffer_flag, bool z_scale) {
        // check if this is a perspective view
        if (z_scale) {
            // Note: RED uses h_fov value of 90 degrees in the perspective view
            // Use Hor+ scaling method to improve user experience when displayed on a widescreen
            // Assume provided FOV makes sense on a 4:3 screen
            float s = static_cast<float>(red::gr_screen.clip_width) / red::gr_screen.clip_height * 0.75f;
            constexpr float pi = 3.141592f;
            float h_fov_rad = horizontal_fov / 180.0f * pi;
            float x = std::tan(h_fov_rad / 2.0f);
            float y = x * s;
            h_fov_rad = 2.0f * std::atan(y);
            horizontal_fov = h_fov_rad / pi * 180.0f;
            // Clamp the value to avoid artifacts when view is very stretched
            horizontal_fov = std::clamp(horizontal_fov, 60.0f, 120.0f);
            //xlog::info("fov {}", horizontal_fov);
        }
        gr_setup_3d_hook.call_target(viewer_orient, viewer_pos, horizontal_fov, zbuffer_flag, z_scale);
    },
};

CodeInjection gr_d3d_init_load_library_injection{
    0x004EC50E,
    [](auto& regs) {
        extern HMODULE g_module;
        auto d3d8to9_path = get_module_dir(g_module) + "d3d8to9.dll";
        xlog::info("Loading d3d8to9.dll: {}", d3d8to9_path);
        HMODULE d3d8to9_module = LoadLibraryA(d3d8to9_path.c_str());
        if (d3d8to9_module) {
            regs.eax = d3d8to9_module;
            regs.eip = 0x004EC519;
        }
    },
};

FunHook<void(HWND)> gr_d3d_set_viewport_wnd_hook{
    0x004EB840,
    [](HWND hwnd) {
        // Original code:
        // * sets broken offset and clip size in gr_screen
        // * configures viewport using off by one window size
        // * reconfigures D3D matrices for no reason (they are unchanged)
        // * resets clipping rect and viewport short after (0x004B8E2B)
        // Rewrite it keeping only the parts that works properly and makes sense
        // Note: In all places where this code is called clip rect is manually changed after the call
        auto& gr_d3d_hwnd = addr_as_ref<HWND>(0x0183B950);
        auto& gr_d3d_wnd_client_rect = addr_as_ref<RECT>(0x0183B798);
        auto& gr_d3d_flush_buffer = addr_as_ref<void()>(0x004E99D0);
        gr_d3d_flush_buffer();
        gr_d3d_hwnd = hwnd;
        GetClientRect(hwnd, &gr_d3d_wnd_client_rect);
    },
};

void ApplyGraphicsPatches()
{
#if D3D_HW_VERTEX_PROCESSING
    // Use hardware vertex processing instead of software processing
    write_mem<u8>(0x004EC73E + 1, D3DCREATE_HARDWARE_VERTEXPROCESSING);
    write_mem<u32>(0x004EBC3D + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    write_mem<u32>(0x004EBC77 + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
#endif

    // Avoid flushing D3D buffers in GrSetColorRgba
    AsmWriter(0x004B976D).nop(5);

    // Add Sleep if window is inactive
    frametime_calculate_hook.install();

    // Improve performance
    after_gr_init_hook.install();

    // Reduce number of draw-calls for line rendering
    AsmWriter(0x004E1335).nop(5);
    gr_d3d_line_3d_patch_1.install();
    gr_d3d_line_3d_patch_2.install();
    AsmWriter(0x004E10B4).nop(5);
    gr_d3d_line_2d_patch_1.install();
    gr_d3d_line_2d_patch_2.install();
    gr_d3d_poly_patch.install();
    gr_d3d_bitmap_patch_1.install();
    gr_d3d_bitmap_patch_2.install();
    gr_d3d_render_geometry_face_patch_1.install();
    gr_d3d_render_geometry_face_patch_2.install();

    // Fix editor not using all space for rendering when used with a big monitor
    gr_init_hook.install();

    // Fix aspect ratio on wide screens
    gr_init_widescreen_patch.install();

    // Use Hor+ FOV scaling
    gr_setup_3d_hook.install();

    // Use d3d8to9 instead of d3d8
    gr_d3d_init_load_library_injection.install();

    // Fix setting viewport window
    gr_d3d_set_viewport_wnd_hook.install();
}

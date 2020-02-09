#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <d3d8.h>

extern HWND g_editor_wnd;

CallHook<void()> frametime_update_hook{
    0x00483047,
    []() {
        if (!IsWindowVisible(g_editor_wnd)) {
            // when minimized 1 FPS
            Sleep(1000);
        }
        else if (GetForegroundWindow() != g_editor_wnd) {
            // when in background 4 FPS
            Sleep(250);
        }
        frametime_update_hook.CallTarget();
    },
};

CodeInjection gr_d3d_draw_line_3d_patch_1{
    0x004E133E,
    [](auto& regs) {
        auto& gr_d3d_buffers_locked = AddrAsRef<bool>(0x0183930D);
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        auto& gr_max_hw_vertex = AddrAsRef<int>(0x01621FA8);
        auto& gr_max_hw_index = AddrAsRef<int>(0x01621FAC);
        auto& gr_current_num_indices = AddrAsRef<int>(0x01839314);
        bool flush_needed = !gr_d3d_buffers_locked
                         || gr_d3d_primitive_type != D3DPT_LINELIST
                         || gr_max_hw_vertex + 2 > 6000
                         || gr_max_hw_index + gr_current_num_indices + 2 > 10000;
        if (!flush_needed) {
            TRACE("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x004E1343;
        }
        else {
            TRACE("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 gr_d3d_buffers_locked, gr_d3d_primitive_type, gr_max_hw_vertex, gr_max_hw_index + gr_current_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_draw_line_3d_patch_2{
    0x004E1528,
    []() {
        auto& gr_current_num_vertices = AddrAsRef<int>(0x01839310);
        gr_current_num_vertices += 2;
    },
};

CodeInjection gr_d3d_draw_line_2d_patch_1{
    0x004E10BD,
    [](auto& regs) {
        auto& gr_d3d_buffers_locked = AddrAsRef<bool>(0x0183930D);
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        auto& gr_max_hw_vertex = AddrAsRef<int>(0x01621FA8);
        auto& gr_max_hw_index = AddrAsRef<int>(0x01621FAC);
        auto& gr_current_num_indices = AddrAsRef<int>(0x01839314);
        bool flush_needed = !gr_d3d_buffers_locked
                         || gr_d3d_primitive_type != D3DPT_LINELIST
                         || gr_max_hw_vertex + 2 > 6000
                         || gr_max_hw_index + gr_current_num_indices + 2 > 10000;
        if (!flush_needed) {
            TRACE("Skipping gr_d3d_prepare_buffers");
            regs.eip = 0x004E10C2;
        }
        else {
            TRACE("Line drawing requires gr_d3d_prepare_buffers %d %d %d %d",
                 gr_d3d_buffers_locked, gr_d3d_primitive_type, gr_max_hw_vertex, gr_max_hw_index + gr_current_num_indices);
        }
    },
};

CallHook<void()> gr_d3d_draw_line_2d_patch_2{
    0x004E11F2,
    []() {
        auto& gr_current_num_vertices = AddrAsRef<int>(0x01839310);
        gr_current_num_vertices += 2;
    },
};

CodeInjection gr_d3d_draw_poly_patch{
    0x004E1573,
    [](auto& regs) {
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        if (gr_d3d_primitive_type != D3DPT_TRIANGLELIST)
            regs.eip = 0x004E1598;
    },
};

CodeInjection gr_d3d_draw_texture_patch_1{
    0x004E090E,
    [](auto& regs) {
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        if (gr_d3d_primitive_type != D3DPT_TRIANGLELIST)
            regs.eip = 0x004E092C;
    },
};

CodeInjection gr_d3d_draw_texture_patch_2{
    0x004E0C97,
    [](auto& regs) {
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        if (gr_d3d_primitive_type != D3DPT_TRIANGLELIST)
            regs.eip = 0x004E0CBB;
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_1{
    0x004E18F1,
    [](auto& regs) {
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        if (gr_d3d_primitive_type != D3DPT_TRIANGLELIST)
            regs.eip = 0x004E1916;
    },
};

CodeInjection gr_d3d_draw_geometry_face_patch_2{
    0x004E1B2D,
    [](auto& regs) {
        auto& gr_d3d_primitive_type = AddrAsRef<int>(0x0175A2C8);
        if (gr_d3d_primitive_type != D3DPT_TRIANGLELIST)
            regs.eip = 0x004E1B53;
    },
};

void ApplyGraphicsPatches()
{
#if D3D_HW_VERTEX_PROCESSING
    // Use hardware vertex processing instead of software processing
    WriteMem<u8>(0x004EC73E + 1, D3DCREATE_HARDWARE_VERTEXPROCESSING);
    WriteMem<u32>(0x004EBC3D + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    WriteMem<u32>(0x004EBC77 + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
#endif

    // Avoid flushing D3D buffers in GrSetColor
    AsmWriter(0x004B976D).nop(5);

    // Add Sleep if window is inactive
    frametime_update_hook.Install();

    // Reduce number of draw-calls for line rendering
    AsmWriter(0x004E1335).nop(5);
    gr_d3d_draw_line_3d_patch_1.Install();
    gr_d3d_draw_line_3d_patch_2.Install();
    AsmWriter(0x004E10B4).nop(5);
    gr_d3d_draw_line_2d_patch_1.Install();
    gr_d3d_draw_line_2d_patch_2.Install();
    gr_d3d_draw_poly_patch.Install();
    gr_d3d_draw_texture_patch_1.Install();
    gr_d3d_draw_texture_patch_2.Install();
    gr_d3d_draw_geometry_face_patch_1.Install();
    gr_d3d_draw_geometry_face_patch_2.Install();
}

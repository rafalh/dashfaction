#include "exports.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWritter.h>
#include <patch_common/MemUtils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <cstdio>
#include <cstddef>
#include <d3d8.h>

#include <log/ConsoleAppender.h>
#include <log/FileAppender.h>
#include <log/Win32Appender.h>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

HMODULE g_Module;
HWND g_EditorWnd;
WNDPROC g_EditorWndProc_Orig;

static const auto g_EditorApp = reinterpret_cast<std::byte*>(0x006F9DA0);

void OpenLevel(const char* Path)
{
    void* DocManager = *reinterpret_cast<void**>(g_EditorApp + 0x80);
    void** DocManager_Vtbl = *reinterpret_cast<void***>(DocManager);
    typedef int(__thiscall * CDocManager_OpenDocumentFile_Ptr)(void* This, LPCSTR String2);
    auto pDocManager_OpenDocumentFile = reinterpret_cast<CDocManager_OpenDocumentFile_Ptr>(DocManager_Vtbl[7]);
    pDocManager_OpenDocumentFile(DocManager, Path);
}

LRESULT CALLBACK EditorWndProc_New(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DROPFILES) {
        HDROP Drop = (HDROP)wParam;
        char FileName[MAX_PATH];
        // Handle only first droped file
        if (DragQueryFile(Drop, 0, FileName, sizeof(FileName)))
            OpenLevel(FileName);
        DragFinish(Drop);
    }
    // Call original procedure
    return g_EditorWndProc_Orig(hwnd, uMsg, wParam, lParam);
}

BOOL CEditorApp__InitInstance_AfterHook()
{
    std::byte* unk = *reinterpret_cast<std::byte**>(g_EditorApp + 0x1C);
    g_EditorWnd = *reinterpret_cast<HWND*>(unk + 28);

    g_EditorWndProc_Orig = reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_EditorWnd, GWLP_WNDPROC));
    SetWindowLongPtr(g_EditorWnd, GWLP_WNDPROC, (LONG)EditorWndProc_New);
    DWORD ExStyle = GetWindowLongPtr(g_EditorWnd, GWL_EXSTYLE);
    SetWindowLongPtr(g_EditorWnd, GWL_EXSTYLE, ExStyle | WS_EX_ACCEPTFILES);
    return TRUE;
}

CallHook<void()> frametime_update_hook{
    0x00483047,
    []() {
        if (!IsWindowVisible(g_EditorWnd)) {
            // when minimized 1 FPS
            Sleep(1000);
        }
        else if (GetForegroundWindow() != g_EditorWnd) {
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

bool g_skip_wnd_set_text = false;

static auto& g_log_view = AddrAsRef<std::byte*>(0x006F9E68);
static auto& log_wnd_append = AddrAsRef<int(void* self, const char* format, ...)>(0x00444980);

using wnd_set_text_type = int __fastcall(void*, void*, const char*);
extern CallHook<wnd_set_text_type> log_append_wnd_set_text_hook;
int __fastcall log_append_wnd_set_text_new(void* self, void* edx, const char* str)
{
    if (g_skip_wnd_set_text)
        return strlen(str);
    else
        return log_append_wnd_set_text_hook.CallTarget(self, edx, str);
}
CallHook<wnd_set_text_type> log_append_wnd_set_text_hook{0x004449C6, log_append_wnd_set_text_new};

using group_mode_handle_selection_type = void __fastcall(void* self);
extern FunHook<group_mode_handle_selection_type> group_mode_handle_selection_hook;
void __fastcall group_mode_handle_selection_new(void* self)
{
    g_skip_wnd_set_text = true;
    group_mode_handle_selection_hook.CallTarget(self);
    g_skip_wnd_set_text = false;
    // TODO: print
    auto log_view = *reinterpret_cast<void**>(g_log_view + 692);
    log_wnd_append(log_view, "");
}
FunHook<group_mode_handle_selection_type> group_mode_handle_selection_hook{0x00423460, group_mode_handle_selection_new};

using brush_mode_handle_selection_type = void __fastcall(void* self);
extern FunHook<brush_mode_handle_selection_type> brush_mode_handle_selection_hook;
void __fastcall brush_mode_handle_selection_new(void* self)
{
    g_skip_wnd_set_text = true;
    brush_mode_handle_selection_hook.CallTarget(self);
    g_skip_wnd_set_text = false;
    // TODO: print
    auto log_view = *reinterpret_cast<void**>(g_log_view + 692);
    log_wnd_append(log_view, "");

}
FunHook<brush_mode_handle_selection_type> brush_mode_handle_selection_hook{0x0043F430, brush_mode_handle_selection_new};

void InitLogging()
{
    CreateDirectoryA("logs", nullptr);
    auto& logger_config = logging::LoggerConfig::root();
    logger_config.add_appender(std::make_unique<logging::FileAppender>("logs/DashEditor.log", false));
    logger_config.add_appender(std::make_unique<logging::ConsoleAppender>());
    logger_config.add_appender(std::make_unique<logging::Win32Appender>());
}

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* Unused)
{
    InitLogging();
    INFO("DashFaction Editor log started.");

    // Prepare command
    static char CmdBuf[512];
    GetModuleFileNameA(g_Module, CmdBuf, sizeof(CmdBuf));
    char* Ptr = strrchr(CmdBuf, '\\');
    strcpy(Ptr, "\\" LAUNCHER_FILENAME " -level \"");

    // Change command for starting level test
    WriteMemPtr(0x00447973 + 1, CmdBuf);
    WriteMemPtr(0x00447CB9 + 1, CmdBuf);

    // Zero first argument for CreateProcess call
    AsmWritter(0x00447B32, 0x00447B39).nop();
    AsmWritter(0x00447B32).xor_(asm_regs::eax, asm_regs::eax);
    AsmWritter(0x00448024, 0x0044802B).nop();
    AsmWritter(0x00448024).xor_(asm_regs::eax, asm_regs::eax);

    // InitInstance hook
    AsmWritter(0x00482C84).call(CEditorApp__InitInstance_AfterHook);

#if D3D_HW_VERTEX_PROCESSING
    // Use hardware vertex processing instead of software processing
    WriteMem<u8>(0x004EC73E + 1, D3DCREATE_HARDWARE_VERTEXPROCESSING);
    WriteMem<u32>(0x004EBC3D + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    WriteMem<u32>(0x004EBC77 + 1, D3DUSAGE_DYNAMIC|D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
#endif

    // Avoid flushing D3D buffers in GrSetColor
    AsmWritter(0x004B976D).nop(5);

    // Add Sleep if window is inactive
    frametime_update_hook.Install();

    // Reduce number of draw-calls for line rendering
    AsmWritter(0x004E1335).nop(5);
    gr_d3d_draw_line_3d_patch_1.Install();
    gr_d3d_draw_line_3d_patch_2.Install();
    AsmWritter(0x004E10B4).nop(5);
    gr_d3d_draw_line_2d_patch_1.Install();
    gr_d3d_draw_line_2d_patch_2.Install();
    gr_d3d_draw_poly_patch.Install();
    gr_d3d_draw_texture_patch_1.Install();
    gr_d3d_draw_texture_patch_2.Install();
    gr_d3d_draw_geometry_face_patch_1.Install();
    gr_d3d_draw_geometry_face_patch_2.Install();

    // Increase memory size of log view buffer (500 lines * 64 characters)
    WriteMem<int32_t>(0x0044489C + 1, 32000);

    // Optimize object selection logging
    log_append_wnd_set_text_hook.Install();
    brush_mode_handle_selection_hook.Install();
    group_mode_handle_selection_hook.Install();
    return 1; // success
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    ERR("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

BOOL WINAPI DllMain(HINSTANCE Instance, [[maybe_unused]] DWORD Reason, [[maybe_unused]] LPVOID Reserved)
{
    g_Module = (HMODULE)Instance;
    DisableThreadLibraryCalls(Instance);
    return TRUE;
}

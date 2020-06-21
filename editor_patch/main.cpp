#include "exports.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/MemUtils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <cstddef>
#include <cstring>
#include <crash_handler_stub.h>

#include <xlog/ConsoleAppender.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

HMODULE g_module;
HWND g_editor_wnd;
WNDPROC g_editor_wnd_proc_orig;
bool g_skip_wnd_set_text = false;

static auto& g_log_view = AddrAsRef<std::byte*>(0x006F9E68);
static const auto g_editor_app = reinterpret_cast<std::byte*>(0x006F9DA0);

static auto& log_wnd_append = AddrAsRef<int(void* self, const char* format, ...)>(0x00444980);

void OpenLevel(const char* level_path)
{
    void* doc_manager = *reinterpret_cast<void**>(g_editor_app + 0x80);
    void** doc_manager_vtbl = *reinterpret_cast<void***>(doc_manager);
    typedef int(__thiscall * CDocManager_OpenDocumentFile_Ptr)(void* this_, LPCSTR path);
    auto DocManager_OpenDocumentFile = reinterpret_cast<CDocManager_OpenDocumentFile_Ptr>(doc_manager_vtbl[7]);
    DocManager_OpenDocumentFile(doc_manager, level_path);
}

LRESULT CALLBACK EditorWndProc_New(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_DROPFILES) {
        HDROP drop = (HDROP)wparam;
        char file_name[MAX_PATH];
        // Handle only first droped file
        if (DragQueryFile(drop, 0, file_name, sizeof(file_name)))
            OpenLevel(file_name);
        DragFinish(drop);
    }
    // Call original procedure
    return g_editor_wnd_proc_orig(hwnd, msg, wparam, lparam);
}

BOOL CEditorApp__InitInstance_AfterHook()
{
    std::byte* unk = *reinterpret_cast<std::byte**>(g_editor_app + 0x1C);
    g_editor_wnd = *reinterpret_cast<HWND*>(unk + 28);

    g_editor_wnd_proc_orig = reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_editor_wnd, GWLP_WNDPROC));
    SetWindowLongPtr(g_editor_wnd, GWLP_WNDPROC, (LONG)EditorWndProc_New);
    DWORD ex_style = GetWindowLongPtr(g_editor_wnd, GWL_EXSTYLE);
    SetWindowLongPtr(g_editor_wnd, GWL_EXSTYLE, ex_style | WS_EX_ACCEPTFILES);
    return TRUE;
}

using wnd_set_text_type = int __fastcall(void*, void*, const char*);
extern CallHook<wnd_set_text_type> log_append_wnd_set_text_hook;
int __fastcall log_append_wnd_set_text_new(void* self, void* edx, const char* str)
{
    if (g_skip_wnd_set_text)
        return std::strlen(str);
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
    xlog::LoggerConfig::get()
        .add_appender<xlog::FileAppender>("logs/DashEditor.log", false)
        .add_appender<xlog::ConsoleAppender>()
        .add_appender<xlog::Win32Appender>();
    xlog::info("DashFaction Editor log started.");
}

void ApplyGraphicsPatches();

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    InitLogging();
    CrashHandlerStubInstall(g_module);

    // Change command for Play Level action to use Dash Faction launcher
    static char module_filename[MAX_PATH];
    if (GetModuleFileNameA(g_module, module_filename, sizeof(module_filename)) != sizeof(module_filename)) {
        static std::string play_level_cmd_part;
        char* dir_path_end = std::strrchr(module_filename, '\\');
        play_level_cmd_part.assign(module_filename, dir_path_end + 1);
        play_level_cmd_part += LAUNCHER_FILENAME;
        play_level_cmd_part += " -level \"";

        WriteMemPtr(0x00447973 + 1, play_level_cmd_part.c_str());
        WriteMemPtr(0x00447CB9 + 1, play_level_cmd_part.c_str());
    }
    else {
        xlog::warn("GetModuleFileNameA failed");
    }

    // Zero first argument for CreateProcess call
    using namespace asm_regs;
    AsmWriter(0x00447B32, 0x00447B39).nop();
    AsmWriter(0x00447B32).xor_(eax, eax);
    AsmWriter(0x00448024, 0x0044802B).nop();
    AsmWriter(0x00448024).xor_(eax, eax);

    // InitInstance hook
    AsmWriter(0x00482C84).call(CEditorApp__InitInstance_AfterHook);

    // Increase memory size of log view buffer (500 lines * 64 characters)
    WriteMem<int32_t>(0x0044489C + 1, 32000);

    // Optimize object selection logging
    log_append_wnd_set_text_hook.Install();
    brush_mode_handle_selection_hook.Install();
    group_mode_handle_selection_hook.Install();

    // Apply patches defined in other files
    ApplyGraphicsPatches();

    return 1; // success
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    xlog::error("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

BOOL WINAPI DllMain(HINSTANCE instance, [[maybe_unused]] DWORD reason, [[maybe_unused]] LPVOID reserved)
{
    g_module = (HMODULE)instance;
    DisableThreadLibraryCalls(instance);
    return TRUE;
}

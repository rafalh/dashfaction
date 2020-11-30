#include "exports.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/MemUtils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <cstddef>
#include <cstring>
#include <crash_handler_stub.h>
#include <string_view>
#include "resources.h"
#include "mfc_types.h"

#include <xlog/ConsoleAppender.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

struct String
{
    int max_len;
    char* buf;
};

HMODULE g_module;
bool g_skip_wnd_set_text = false;

static auto& g_log_view = AddrAsRef<std::byte*>(0x006F9E68);
static const auto g_editor_app = reinterpret_cast<std::byte*>(0x006F9DA0);

static auto& log_wnd_append = AddrAsRef<int(void* self, const char* format, ...)>(0x00444980);

HWND GetMainFrameHandle()
{
    auto main_frame = StructFieldRef<CWnd*>(g_editor_app, 0xC8);
    return WndToHandle(main_frame);
}

void OpenLevel(const char* level_path)
{
    void* doc_manager = *reinterpret_cast<void**>(g_editor_app + 0x80);
    void** doc_manager_vtbl = *reinterpret_cast<void***>(doc_manager);
    typedef int(__thiscall * CDocManager_OpenDocumentFile_Ptr)(void* this_, LPCSTR path);
    auto DocManager_OpenDocumentFile = reinterpret_cast<CDocManager_OpenDocumentFile_Ptr>(doc_manager_vtbl[7]);
    DocManager_OpenDocumentFile(doc_manager, level_path);
}

CodeInjection CMainFrame_PreCreateWindow_injection{
    0x00447134,
    [](auto& regs) {
        auto& cs = AddrAsRef<CREATESTRUCTA>(regs.eax);
        cs.dwExStyle |= WS_EX_ACCEPTFILES;
    },
};

CodeInjection CEditorApp_InitInstance_additional_file_paths_injection{
    0x0048290D,
    []() {
        // Load v3m files from more localizations instead of only VPP packfiles
        auto file_add_path = AddrAsRef<int(const char *path, const char *exts, bool cd)>(0x004C3950);
        file_add_path("red\\meshes", ".v3m", false);
        file_add_path("user_maps\\meshes", ".v3m", false);
    },
};

CodeInjection CEditorApp_InitInstance_open_level_injection{
    0x00482BF1,
    []() {
        static auto& argv = AddrAsRef<char**>(0x01DBF8E4);
        static auto& argc = AddrAsRef<int>(0x01DBF8E0);
        const char* level_param = nullptr;
        for (int i = 1; i < argc; ++i) {
            xlog::trace("argv[%d] = %s", i, argv[i]);
            std::string_view arg = argv[i];
            if (arg == "-level") {
                ++i;
                if (i < argc) {
                    level_param = argv[i];
                }
            }
        }
        if (level_param) {
            OpenLevel(level_param);
        }
    },
};

CodeInjection CWnd_CreateDlg_injection{
    0x0052F112,
    [](auto& regs) {
        auto& hCurrentResourceHandle = regs.esi;
        auto lpszTemplateName = AddrAsRef<LPCSTR>(regs.esp);
        // Dialog resource customizations:
        // - 136: main window top bar (added tool buttons)
        if (lpszTemplateName == MAKEINTRESOURCE(IDD_MAIN_FRAME_TOP_BAR)) {
            hCurrentResourceHandle = reinterpret_cast<int>(g_module);
        }
    },
};

CodeInjection CDialog_DoModal_injection{
    0x0052F461,
    [](auto& regs) {
        auto& hCurrentResourceHandle = regs.ebx;
        auto lpszTemplateName = AddrAsRef<LPCSTR>(regs.esp);
        // Customize:
        // - 148: trigger properties dialog
        if (lpszTemplateName == MAKEINTRESOURCE(IDD_TRIGGER_PROPERTIES)) {
            hCurrentResourceHandle = reinterpret_cast<int>(g_module);
        }
    },
};

CodeInjection CFrameWnd_Create_injection{
    0x00540733,
    [](auto& regs) {
        auto& hCurrentResourceHandle = regs.eax;
        auto lpszTemplateName = AddrAsRef<LPCSTR>(regs.esp);
        // Customize:
        // - 128: main menu
        if (lpszTemplateName == MAKEINTRESOURCE(IDR_MAIN_MENU)) {
            hCurrentResourceHandle = reinterpret_cast<int>(g_module);
        }
    },
};

CodeInjection CDedLevel_OpenRespawnPointProperties_injection{
    0x00404CB8,
    [](auto& regs) {
        // Fix wrong respawn point reference being used when copying booleans from checkboxes
        regs.esi = regs.ebx;
    },
};

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

void __fastcall DedLight_UpdateLevelLight(void *this_);
FunHook DedLight_UpdateLevelLight_hook{
    0x00453200,
    DedLight_UpdateLevelLight,
};
void __fastcall DedLight_UpdateLevelLight(void *this_)
{
    auto& this_is_enabled = StructFieldRef<bool>(this_, 0xD5);
    auto& level_light = StructFieldRef<void*>(this_, 0xD8);
    auto& level_light_is_enabled = StructFieldRef<bool>(level_light, 0x91);
    level_light_is_enabled = this_is_enabled;
    DedLight_UpdateLevelLight_hook.CallTarget(this_);
}

struct CTextureBrowserDialog;
String * __fastcall CTextureBrowserDialog_GetFolderName(CTextureBrowserDialog *this_, int edx, String *folder_name);
FunHook CTextureBrowserDialog_GetFolderName_hook{
    0x00471260,
    CTextureBrowserDialog_GetFolderName,
};
String * __fastcall CTextureBrowserDialog_GetFolderName(CTextureBrowserDialog *this_, int edx, String *folder_name)
{
    auto& texture_browser_folder_index = AddrAsRef<int>(0x006CA404);
    if (texture_browser_folder_index > 0) {
        return CTextureBrowserDialog_GetFolderName_hook.CallTarget(this_, edx, folder_name);
    }
    folder_name->buf = nullptr;
    folder_name->max_len = 0;
    return folder_name;
}

CodeInjection CCutscenePropertiesDialog_ct_crash_fix{
    0x00458A84,
    [](auto& regs) {
        auto this_ = reinterpret_cast<void*>(regs.esi);
        auto& this_num_shots = StructFieldRef<int>(this_, 0x60);
        this_num_shots = 0;
    },
};

BOOL __fastcall CMainFrame_OnCmdMsg(CWnd* this_, int, UINT nID, int nCode, void* pExtra, void* pHandlerInfo)
{
    constexpr int CN_COMMAND = 0;
    if (nCode == CN_COMMAND && (nID == ID_WIKI_EDITING_MAIN_PAGE || nID == ID_WIKI_HOTKEYS)) {
        // Tell MFC that this command has a handler so it does not disable menu item
        if (pHandlerInfo) {
            // It seems handler info is not used but it's better to initialize it just in case
            ZeroMemory(pHandlerInfo, 8);
        }
        return TRUE;
    }
    return AddrCaller{0x00540C5B}.this_call<BOOL>(this_, nID, nCode, pExtra, pHandlerInfo);
}

BOOL __fastcall CMainFrame_OnCommand(CWnd* this_, int, WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_WIKI_HOTKEYS) {
        ShellExecuteA(WndToHandle(this_), "open", "https://redfactionwiki.com/wiki/RED_Hotkey_Reference", nullptr, nullptr, SW_SHOW);
        //MessageBox(0,"OnCommand",0,0);
        return TRUE;
    }
    if (wParam == ID_WIKI_EDITING_MAIN_PAGE) {
        ShellExecuteA(WndToHandle(this_), "open", "https://redfactionwiki.com/wiki/RF1_Editing_Main_Page", nullptr, nullptr, SW_SHOW);
        //MessageBox(0,"OnCommand",0,0);
        return TRUE;
    }
    return AddrCaller{0x005402B9}.this_call<BOOL>(this_, wParam, lParam);
}

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
void ApplyTriggerPatches();

std::string GetModuleDir(HMODULE module)
{
    std::string buf(MAX_PATH, '\0');
    auto num_copied = GetModuleFileNameA(module, buf.data(), buf.size());
    if (num_copied == buf.size()) {
        xlog::error("GetModuleFileNameA failed (%lu)", GetLastError());
        return {};
    }
    buf.resize(num_copied);
    auto last_sep = buf.rfind('\\');
    if (last_sep != std::string::npos) {
        buf.resize(last_sep + 1);
    }
    return buf;
}

void LoadDashEditorPackfile()
{
    static auto& vpackfile_add = AddrAsRef<int __cdecl(const char *name, const char *dir)>(0x004CA930);
    static auto& root_path = AddrAsRef<char[256]>(0x0158CA10);

    auto df_dir = GetModuleDir(g_module);
    std::string old_root_path = root_path;
    std::strncpy(root_path, df_dir.c_str(), sizeof(root_path) - 1);
    if (!vpackfile_add("dashfaction.vpp", nullptr)) {
        xlog::error("Failed to load dashfaction.vpp from %s", df_dir.c_str());
    }
    std::strncpy(root_path, old_root_path.c_str(), sizeof(root_path) - 1);
}

CodeInjection vpackfile_init_injection{
    0x004CA533,
    []() {
        LoadDashEditorPackfile();
    },
};

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    InitLogging();
    CrashHandlerStubInstall(g_module);

    // Change command for Play Level action to use Dash Faction launcher
    static std::string play_level_cmd_part = GetModuleDir(g_module);
    play_level_cmd_part += LAUNCHER_FILENAME;
    play_level_cmd_part += " -level \"";

    WriteMemPtr(0x00447973 + 1, play_level_cmd_part.c_str());
    WriteMemPtr(0x00447CB9 + 1, play_level_cmd_part.c_str());

    // Zero first argument for CreateProcess call
    using namespace asm_regs;
    AsmWriter(0x00447B32, 0x00447B39).nop();
    AsmWriter(0x00447B32).xor_(eax, eax);
    AsmWriter(0x00448024, 0x0044802B).nop();
    AsmWriter(0x00448024).xor_(eax, eax);

    // Add additional file paths for V3M loading
    CEditorApp_InitInstance_additional_file_paths_injection.Install();

    // Add handling for "-level" command line argument
    CEditorApp_InitInstance_open_level_injection.Install();

    // Change main frame style flags to allow dropping files
    CMainFrame_PreCreateWindow_injection.Install();

    // Increase memory size of log view buffer (500 lines * 64 characters)
    WriteMem<int32_t>(0x0044489C + 1, 32000);

    // Optimize object selection logging
    log_append_wnd_set_text_hook.Install();
    brush_mode_handle_selection_hook.Install();
    group_mode_handle_selection_hook.Install();

    // Replace some dialog resources
    CWnd_CreateDlg_injection.Install();
    CDialog_DoModal_injection.Install();
    CFrameWnd_Create_injection.Install();

    // Remove "Packfile saved successfully!" message
    AsmWriter{0x0044CCA3, 0x0044CCB3}.nop();

    // Fix changing properties of multiple respawn points
    CDedLevel_OpenRespawnPointProperties_injection.Install();

    // Apply patches defined in other files
    ApplyGraphicsPatches();
    ApplyTriggerPatches();

    // Browse for .v3m files instead of .v3d
    static char mesh_ext_filter[] = "Mesh (*.v3m)|*.v3m|All Files (*.*)|*.*||";
    WriteMemPtr(0x0044243E + 1, mesh_ext_filter);
    WriteMemPtr(0x00462490 + 1, mesh_ext_filter);
    WriteMemPtr(0x0044244F + 1, ".v3m");
    WriteMemPtr(0x004624A9 + 1, ".v3m");
    WriteMemPtr(0x0044244A + 1, "RFBrush.v3m");

    // Fix rendering of VBM textures from user_maps/textures
    WriteMemPtr(0x004828C2 + 1, ".tga .vbm");

    // Fix lights sometimes not working
    DedLight_UpdateLevelLight_hook.Install();

    // Fix crash when selecting decal texture from 'All' folder
    CTextureBrowserDialog_GetFolderName_hook.Install();

    // No longer require "-sound" argument to enable sound support
    AsmWriter{0x00482BC4}.nop(2);

    // Fix random crash when opening cutscene properties
    CCutscenePropertiesDialog_ct_crash_fix.Install();

    // Load DashEditor.vpp
    vpackfile_init_injection.Install();

    // Add maps_df.txt to the collection of files scanned for default textures in order to add more textures from the
    // base game to the texture browser
    // Especially add Rck_DefaultP.tga to default textures to fix error when packing a level containing a particle
    // emitter with default properties
    static const char* maps_files_names[] = {
        "maps.txt",
        "maps1.txt",
        "maps2.txt",
        "maps3.txt",
        "maps4.txt",
        "maps_df.txt",
    };
    WriteMemPtr(0x0041B813 + 1, maps_files_names);
    WriteMemPtr(0x0041B824 + 1, maps_files_names);

    // Fix path node connections sometimes being rendered incorrectly
    // For some reason editor code uses copies of radius and position fields in some situation and those copies
    // are sometimes uninitialized
    WriteMem<u8>(0x004190EB, asm_opcodes::jmp_rel_short);
    WriteMem<u8>(0x0041924A, asm_opcodes::jmp_rel_short);

    // Support additional commands
    WriteMemPtr(0x00556574, &CMainFrame_OnCmdMsg);
    WriteMemPtr(0x005565E0, &CMainFrame_OnCommand);

    // Fix F4 key (Maximize active viewport) for screens larger than 1024x768
    constexpr int max_size = 0x7FFF;
    WriteMem<int>(0x0044770D + 1, max_size);
    WriteMem<int>(0x0044771D + 1, max_size);
    WriteMem<int>(0x00447750 + 1, -max_size);
    WriteMem<int>(0x004477E1 + 1, -max_size);
    WriteMem<int>(0x00447797 + 1, max_size);
    WriteMem<int>(0x00447761 + 1, max_size);
    WriteMem<int>(0x004477A0 + 2, -max_size);
    WriteMem<int>(0x004477EE + 2, -max_size);

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

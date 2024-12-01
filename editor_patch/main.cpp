#include <cstddef>
#include <cstring>
#include <functional>
#include <string_view>
#include <windows.h>
#include <shellapi.h>
#include <common/version/version.h>
#include <common/config/BuildConfig.h>
#include <common/utils/os-utils.h>
#include <xlog/xlog.h>
#include <xlog/ConsoleAppender.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>
#include <patch_common/MemUtils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <crash_handler_stub.h>
#include "exports.h"
#include "resources.h"
#include "mfc_types.h"
#include "vtypes.h"

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

constexpr size_t max_texture_name_len = 31;

HMODULE g_module;
bool g_skip_wnd_set_text = false;

static const auto g_editor_app = reinterpret_cast<std::byte*>(0x006F9DA0);
static auto& g_main_frame = addr_as_ref<std::byte*>(0x006F9E68);

static auto& LogDlg_Append = addr_as_ref<int(void* self, const char* format, ...)>(0x00444980);

void* GetMainFrame()
{
    return struct_field_ref<void*>(g_editor_app, 0xC8);
}

void* GetLogDlg()
{
    return struct_field_ref<void*>(GetMainFrame(), 692);
}

HWND GetMainFrameHandle()
{
    auto* main_frame = struct_field_ref<CWnd*>(g_editor_app, 0xC8);
    return WndToHandle(main_frame);
}

void OpenLevel(const char* level_path)
{
    void* doc_manager = *reinterpret_cast<void**>(g_editor_app + 0x80);
    void** doc_manager_vtbl = *reinterpret_cast<void***>(doc_manager);
    using CDocManager_OpenDocumentFile_Ptr = int(__thiscall*)(void* this_, LPCSTR path);
    auto DocManager_OpenDocumentFile = reinterpret_cast<CDocManager_OpenDocumentFile_Ptr>(doc_manager_vtbl[7]);
    DocManager_OpenDocumentFile(doc_manager, level_path);
}

CodeInjection CMainFrame_PreCreateWindow_injection{
    0x0044713C,
    [](auto& regs) {
        auto& cs = addr_as_ref<CREATESTRUCTA>(regs.eax);
        cs.style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        cs.dwExStyle |= WS_EX_ACCEPTFILES;
    },
};

CodeInjection CEditorApp_InitInstance_additional_file_paths_injection{
    0x0048290D,
    []() {
        // Load v3m files from more localizations instead of only VPP packfiles
        auto file_add_path = addr_as_ref<int(const char* path, const char* exts, bool cd)>(0x004C3950);
        file_add_path("red\\meshes", ".v3m .vfx", false);
        file_add_path("user_maps\\meshes", ".v3m .vfx", false);
    },
};

CodeInjection CEditorApp_InitInstance_open_level_injection{
    0x00482BF1,
    []() {
        static auto& argv = addr_as_ref<char**>(0x01DBF8E4);
        static auto& argc = addr_as_ref<int>(0x01DBF8E0);
        const char* level_param = nullptr;
        for (int i = 1; i < argc; ++i) {
            xlog::trace("argv[{}] = {}", i, argv[i]);
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
        auto lpszTemplateName = addr_as_ref<LPCSTR>(regs.esp);
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
        auto lpszTemplateName = addr_as_ref<LPCSTR>(regs.esp);
        // Customize:
        // - 148: trigger properties dialog
        if (lpszTemplateName == MAKEINTRESOURCE(IDD_TRIGGER_PROPERTIES)) {
            hCurrentResourceHandle = reinterpret_cast<int>(g_module);
        }
    },
};

HMENU WINAPI LoadMenuA_new(HINSTANCE hInstance, LPCSTR lpMenuName)
{
    HMENU result = LoadMenuA(g_module, lpMenuName);
    if (!result) {
        result = LoadMenuA(hInstance, lpMenuName);
    }
    return result;
}

HICON WINAPI LoadIconA_new(HINSTANCE hInstance, LPCSTR lpIconName)
{
    HICON result = LoadIconA(g_module, lpIconName);
    if (!result) {
        result = LoadIconA(hInstance, lpIconName);
    }
    return result;
}

HACCEL WINAPI LoadAcceleratorsA_new(HINSTANCE hInstance, LPCSTR lpTableName)
{
    HACCEL result = LoadAcceleratorsA(g_module, lpTableName);
    if (!result) {
        result = LoadAcceleratorsA(hInstance, lpTableName);
    }
    return result;
}

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
    if (g_skip_wnd_set_text) {
        return std::strlen(str);
    }
    return log_append_wnd_set_text_hook.call_target(self, edx, str);
}
CallHook<wnd_set_text_type> log_append_wnd_set_text_hook{0x004449C6, log_append_wnd_set_text_new};

using group_mode_handle_selection_type = void __fastcall(void* self);
extern FunHook<group_mode_handle_selection_type> group_mode_handle_selection_hook;
void __fastcall group_mode_handle_selection_new(void* self)
{
    g_skip_wnd_set_text = true;
    group_mode_handle_selection_hook.call_target(self);
    g_skip_wnd_set_text = false;
    // TODO: print
    LogDlg_Append(GetLogDlg(), "");
}
FunHook<group_mode_handle_selection_type> group_mode_handle_selection_hook{0x00423460, group_mode_handle_selection_new};

using brush_mode_handle_selection_type = void __fastcall(void* self);
extern FunHook<brush_mode_handle_selection_type> brush_mode_handle_selection_hook;
void __fastcall brush_mode_handle_selection_new(void* self)
{
    g_skip_wnd_set_text = true;
    brush_mode_handle_selection_hook.call_target(self);
    g_skip_wnd_set_text = false;
    // TODO: print
    LogDlg_Append(GetLogDlg(), "");
}
FunHook<brush_mode_handle_selection_type> brush_mode_handle_selection_hook{0x0043F430, brush_mode_handle_selection_new};

void __fastcall DedLight_UpdateLevelLight(void* this_);
FunHook DedLight_UpdateLevelLight_hook{
    0x00453200,
    DedLight_UpdateLevelLight,
};
void __fastcall DedLight_UpdateLevelLight(void* this_)
{
    auto& this_is_enabled = struct_field_ref<bool>(this_, 0xD5);
    auto& level_light = struct_field_ref<void*>(this_, 0xD8);
    auto& level_light_is_enabled = struct_field_ref<bool>(level_light, 0x91);
    level_light_is_enabled = this_is_enabled;
    DedLight_UpdateLevelLight_hook.call_target(this_);
}

struct CTextureBrowserDialog;
rf::String* __fastcall CTextureBrowserDialog_GetFolderName(
    CTextureBrowserDialog* this_, int edx, rf::String* folder_name
);
FunHook CTextureBrowserDialog_GetFolderName_hook{
    0x00471260,
    CTextureBrowserDialog_GetFolderName,
};
rf::String* __fastcall CTextureBrowserDialog_GetFolderName(
    CTextureBrowserDialog* this_, int edx, rf::String* folder_name
)
{
    auto& texture_browser_folder_index = addr_as_ref<int>(0x006CA404);
    if (texture_browser_folder_index > 0) {
        return CTextureBrowserDialog_GetFolderName_hook.call_target(this_, edx, folder_name);
    }
    folder_name->buf = nullptr;
    folder_name->max_len = 0;
    return folder_name;
}

CodeInjection CCutscenePropertiesDialog_ct_crash_fix{
    0x00458A84,
    [](auto& regs) {
        void* this_ = regs.esi;
        auto& this_num_shots = struct_field_ref<int>(this_, 0x60);
        this_num_shots = 0;
    },
};

class CMainFrame;

static auto RedrawEditorAfterModification = addr_as_ref<int __cdecl()>(0x00483560);

void* GetLevelFromMainFrame(CWnd* main_frame)
{
    auto* doc = struct_field_ref<void*>(main_frame, 0xD0);
    return &struct_field_ref<int>(doc, 0x60);
}

void CMainFrame_OpenHelp(CWnd* this_)
{
    ShellExecuteA(
        WndToHandle(this_), "open", "https://redfactionwiki.com/wiki/RF1_Editing_Main_Page", nullptr, nullptr, SW_SHOW
    );
}

void CMainFrame_OpenHotkeysHelp(CWnd* this_)
{
    ShellExecuteA(
        WndToHandle(this_), "open", "https://redfactionwiki.com/wiki/RED_Hotkey_Reference", nullptr, nullptr, SW_SHOW
    );
}

void CMainFrame_HideAllObjects(CWnd* this_)
{
    AddrCaller{0x0042DCA0}.this_call(GetLevelFromMainFrame(this_));
    RedrawEditorAfterModification();
}

void CMainFrame_ShowAllObjects(CWnd* this_)
{
    AddrCaller{0x0042DDA0}.this_call(GetLevelFromMainFrame(this_));
    RedrawEditorAfterModification();
}

void CMainFrame_HideSelected(CWnd* this_)
{
    AddrCaller{0x0042DBF0}.this_call(GetLevelFromMainFrame(this_));
    RedrawEditorAfterModification();
}

void CMainFrame_SelectObjectByUid(CWnd* this_)
{
    AddrCaller{0x0042E720}.this_call(GetLevelFromMainFrame(this_));
    RedrawEditorAfterModification();
}

void CMainFrame_InvertSelection(CWnd* this_)
{
    AddrCaller{0x0042E890}.this_call(GetLevelFromMainFrame(this_));
    RedrawEditorAfterModification();
}

BOOL __fastcall CMainFrame_OnCmdMsg(CWnd* this_, int, UINT nID, int nCode, void* pExtra, void* pHandlerInfo)
{
    constexpr int CN_COMMAND = 0;

    if (nCode == CN_COMMAND) {
        std::function<void()> handler;
        switch (nID) {
            case ID_WIKI_EDITING_MAIN_PAGE:
                handler = std::bind(CMainFrame_OpenHelp, this_);
                break;
            case ID_WIKI_HOTKEYS:
                handler = std::bind(CMainFrame_OpenHotkeysHelp, this_);
                break;
            case ID_HIDE_ALL_OBJECTS:
                handler = std::bind(CMainFrame_HideAllObjects, this_);
                break;
            case ID_SHOW_ALL_OBJECTS:
                handler = std::bind(CMainFrame_ShowAllObjects, this_);
                break;
            case ID_HIDE_SELECTED:
                handler = std::bind(CMainFrame_HideSelected, this_);
                break;
            case ID_SELECT_OBJECT_BY_UID:
                handler = std::bind(CMainFrame_SelectObjectByUid, this_);
                break;
            case ID_INVERT_SELECTION:
                handler = std::bind(CMainFrame_InvertSelection, this_);
                break;
        }

        if (handler) {
            // Tell MFC that this command has a handler so it does not disable menu item
            if (pHandlerInfo) {
                // It seems handler info is not used but it's better to initialize it just in case
                ZeroMemory(pHandlerInfo, 8);
            }
            else {
                // Run the handler
                handler();
            }
            return TRUE;
        }
    }
    return AddrCaller{0x00540C5B}.this_call<BOOL>(this_, nID, nCode, pExtra, pHandlerInfo);
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

void InitCrashHandler()
{
    char current_dir[MAX_PATH] = ".";
    GetCurrentDirectoryA(std::size(current_dir), current_dir);

    CrashHandlerConfig config;
    config.this_module_handle = g_module;
    std::snprintf(config.log_file, std::size(config.log_file), "%s\\logs\\DashEditor.log", current_dir);
    std::snprintf(config.output_dir, std::size(config.output_dir), "%s\\logs", current_dir);
    std::snprintf(config.app_name, std::size(config.app_name), "DashEditor");
    config.add_known_module("RED");
    config.add_known_module("DashEditor");

    CrashHandlerStubInstall(config);
}

void ApplyGraphicsPatches();
void ApplyTriggerPatches();

void LoadDashEditorPackfile()
{
    static auto& vpackfile_add = addr_as_ref<int __cdecl(const char* name, const char* dir)>(0x004CA930);
    static auto& root_path = addr_as_ref<char[256]>(0x0158CA10);

    auto df_dir = get_module_dir(g_module);
    std::string old_root_path = root_path;
    std::strncpy(root_path, df_dir.c_str(), sizeof(root_path) - 1);
    if (!vpackfile_add("dashfaction.vpp", nullptr)) {
        xlog::error("Failed to load dashfaction.vpp from {}", df_dir);
    }
    std::strncpy(root_path, old_root_path.c_str(), sizeof(root_path) - 1);
}

CodeInjection vpackfile_init_injection{
    0x004CA533,
    []() { LoadDashEditorPackfile(); },
};

CodeInjection CMainFrame_OnPlayLevelCmd_skip_level_dir_injection{
    0x004479AD,
    [](auto& regs) {
        char* level_pathname = regs.eax;
        regs.eax = std::strrchr(level_pathname, '\\') + 1;
    },
};

CodeInjection CMainFrame_OnPlayLevelFromCameraCmd_skip_level_dir_injection{
    0x00447CF2,
    [](auto& regs) {
        char* level_pathname = regs.eax;
        regs.eax = std::strrchr(level_pathname, '\\') + 1;
    },
};

CodeInjection CDedLevel_CloneObject_injection{
    0x004135B9,
    [](auto& regs) {
        void* that = regs.ecx;
        int type = regs.eax;
        void* obj = regs.edi;
        if (type == 0xC) {
            // clone cutscene path node
            regs.eax = AddrCaller{0x00413C70}.this_call<void*>(that, obj);
            regs.eip = 0x004135CF;
        }
    },
};

CodeInjection CDedEvent_Copy_injection{
    0x0045146D,
    [](auto& regs) {
        void* src_event = regs.edi;
        void* dst_event = regs.esi;
        // copy bool2 field
        struct_field_ref<bool>(dst_event, 0xB1) = struct_field_ref<bool>(src_event, 0xB1);
    },
};

CodeInjection texture_name_buffer_overflow_injection1{
    0x00445297,
    [](auto& regs) {
        const char* filename = regs.esi;
        if (std::strlen(filename) > max_texture_name_len) {
            LogDlg_Append(GetLogDlg(), "Texture name too long: %s\n", filename);
            regs.eip = 0x00445273;
        }
    },
};

CodeInjection texture_name_buffer_overflow_injection2{
    0x004703EC,
    [](auto& regs) {
        const char* filename = regs.ebp;
        if (std::strlen(filename) > max_texture_name_len) {
            LogDlg_Append(GetLogDlg(), "Texture name too long: %s\n", filename);
            regs.eip = 0x0047047F;
        }
    },
};

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    InitLogging();
    InitCrashHandler();

    // Change command for Play Level action to use Dash Faction launcher
    static std::string launcher_pathname = get_module_dir(g_module) + LAUNCHER_FILENAME;
    using namespace asm_regs;
    AsmWriter(0x00447B32, 0x00447B39).mov(eax, launcher_pathname.c_str());
    AsmWriter(0x00448024, 0x0044802B).mov(eax, launcher_pathname.c_str());
    CMainFrame_OnPlayLevelCmd_skip_level_dir_injection.install();
    CMainFrame_OnPlayLevelFromCameraCmd_skip_level_dir_injection.install();

    // Add additional file paths for V3M loading
    CEditorApp_InitInstance_additional_file_paths_injection.install();

    // Add handling for "-level" command line argument
    CEditorApp_InitInstance_open_level_injection.install();

    // Change main frame style flags to allow dropping files
    CMainFrame_PreCreateWindow_injection.install();

    // Increase memory size of log view buffer (500 lines * 64 characters)
    write_mem<int32_t>(0x0044489C + 1, 32000);

    // Optimize object selection logging
    log_append_wnd_set_text_hook.install();
    brush_mode_handle_selection_hook.install();
    group_mode_handle_selection_hook.install();

    // Replace some editor resources
    CWnd_CreateDlg_injection.install();
    CDialog_DoModal_injection.install();
    write_mem_ptr(0x0055456C, &LoadMenuA_new);
    write_mem_ptr(0x005544FC, &LoadIconA_new);
    write_mem_ptr(0x005543AC, &LoadAcceleratorsA_new);

    // Remove "Packfile saved successfully!" message
    AsmWriter{0x0044CCA3, 0x0044CCB3}.nop();

    // Fix changing properties of multiple respawn points
    CDedLevel_OpenRespawnPointProperties_injection.install();

    // Apply patches defined in other files
    ApplyGraphicsPatches();
    ApplyTriggerPatches();

    // Browse for .v3m files instead of .v3d
    static char mesh_ext_filter[] = "Mesh (*.v3m)|*.v3m|All Files (*.*)|*.*||";
    write_mem_ptr(0x0044243E + 1, mesh_ext_filter);
    write_mem_ptr(0x00462490 + 1, mesh_ext_filter);
    write_mem_ptr(0x0044244F + 1, ".v3m");
    write_mem_ptr(0x004624A9 + 1, ".v3m");
    write_mem_ptr(0x0044244A + 1, "RFBrush.v3m");

    // Fix rendering of VBM textures from user_maps/textures
    write_mem_ptr(0x004828C2 + 1, ".tga .vbm");

    // Fix lights sometimes not working
    DedLight_UpdateLevelLight_hook.install();

    // Fix crash when selecting decal texture from 'All' folder
    CTextureBrowserDialog_GetFolderName_hook.install();

    // No longer require "-sound" argument to enable sound support
    AsmWriter{0x00482BC4}.nop(2);

    // Fix random crash when opening cutscene properties
    CCutscenePropertiesDialog_ct_crash_fix.install();

    // Load DashEditor.vpp
    vpackfile_init_injection.install();

    // Add maps_df.txt to the collection of files scanned for default textures in order to add more textures from the
    // base game to the texture browser
    // Especially add Rck_DefaultP.tga to default textures to fix error when packing a level containing a particle
    // emitter with default properties
    static const char* maps_files_names[] = {
        "maps.txt", "maps1.txt", "maps2.txt", "maps3.txt", "maps4.txt", "maps_df.txt", nullptr,
    };
    write_mem_ptr(0x0041B813 + 1, maps_files_names);
    write_mem_ptr(0x0041B824 + 1, maps_files_names);

    // Fix path node connections sometimes being rendered incorrectly
    // For some reason editor code uses copies of radius and position fields in some situation and those copies
    // are sometimes uninitialized
    write_mem<u8>(0x004190EB, asm_opcodes::jmp_rel_short);
    write_mem<u8>(0x0041924A, asm_opcodes::jmp_rel_short);

    // Support additional commands
    write_mem_ptr(0x00556574, &CMainFrame_OnCmdMsg);

    // Fix F4 key (Maximize active viewport) for screens larger than 1024x768
    constexpr int max_size = 0x7FFF;
    write_mem<int>(0x0044770D + 1, max_size);
    write_mem<int>(0x0044771D + 1, max_size);
    write_mem<int>(0x00447750 + 1, -max_size);
    write_mem<int>(0x004477E1 + 1, -max_size);
    write_mem<int>(0x00447797 + 1, max_size);
    write_mem<int>(0x00447761 + 1, max_size);
    write_mem<int>(0x004477A0 + 2, -max_size);
    write_mem<int>(0x004477EE + 2, -max_size);

    // Fix editor crash when building geometry after lightmap resolution for a face was set to Undefined
    write_mem<i8>(0x00402DFA + 1, 0);

    // Allow more decals before displaying a warning message about too many decals in the level
    write_mem<i8>(0x0041E2A9 + 2, 127);
    write_mem<i8>(0x0041E2BA + 2, 127);
    write_mem_ptr(
        0x0041E2C6 + 1, "There are more than 127 decals in the level! It can result in a crash for older game clients."
    );

    // Fix copying cutscene path node
    CDedLevel_CloneObject_injection.install();

    // Fix copying bool2 field in events
    CDedEvent_Copy_injection.install();

    // Remove uid limit (50k) by removing cmp and jge instructions in FindBiggestUid function
    AsmWriter{0x004844AC, 0x004844B3}.nop();

    // Ignore textures with filename longer than 31 characters to avoid buffer overflow errors
    texture_name_buffer_overflow_injection1.install();
    texture_name_buffer_overflow_injection2.install();

    // Increase face limit in g_boolean_find_all_pairs
    static void* found_faces_a[0x10000];
    static void* found_faces_b[0x10000];
    write_mem_ptr(0x004A7290 + 3, found_faces_a);
    write_mem_ptr(0x004A7158 + 1, found_faces_a);
    write_mem_ptr(0x004A72E5 + 4, found_faces_a);
    write_mem_ptr(0x004A717D + 1, found_faces_b);
    write_mem_ptr(0x004A71A6 + 1, found_faces_b);
    write_mem_ptr(0x004A72A2 + 3, found_faces_b);
    write_mem_ptr(0x004A72F9 + 1, found_faces_b);

    return 1; // success
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    xlog::error("SubHook unknown opcode 0x{:x} at {}", *opcode, static_cast<void*>(opcode));
}

BOOL WINAPI DllMain(HINSTANCE instance, [[maybe_unused]] DWORD reason, [[maybe_unused]] LPVOID reserved)
{
    g_module = (HMODULE)instance;
    DisableThreadLibraryCalls(instance);
    return TRUE;
}

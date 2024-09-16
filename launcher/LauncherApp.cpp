#include "LauncherApp.h"
#include "MainDlg.h"
#include "LauncherCommandLineInfo.h"
#include <common/config/GameConfig.h>
#include <common/version/version.h>
#include <common/error/error-utils.h>
#include <launcher_common/PatchedAppLauncher.h>
#include <xlog/xlog.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// LauncherApp initialization
int LauncherApp::Run()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
    Win32xx::LoadCommonControls();

    // Command line parsing
    xlog::info("Parsing command line");
    m_cmd_line_info.Parse();

    if (m_cmd_line_info.HasHelpFlag()) {
        // Note: we can't use stdio console API in win32 application
        Message(nullptr,
            "Usage: DashFactionLauncher [-game] [-level name] [-editor] args...\n"
            "-game        Starts game immediately\n"
            "-level name  Starts game immediately and loads specified level\n"
            "-editor      Starts level editor immediately\n"
            "-exepath     Override patched executable file location\n"
            "args...      Additional arguments passed to game or editor\n",
            "Dash Faction Launcher Help", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Migrate Dash Faction config from old version
    MigrateConfig();

    // Disable elevation (UAC)
    SetEnvironmentVariableA("__COMPAT_LAYER", "RunAsInvoker");


    // Launch game or editor based on command line flag
    if (m_cmd_line_info.HasGameFlag()) {
        LaunchGame(nullptr, nullptr);
        return 0;
    }

    if (m_cmd_line_info.HasEditorFlag()) {
        LaunchEditor(nullptr, nullptr);
        return 0;
    }

    // Show main dialog
    xlog::info("Showing main dialog");
	MainDlg dlg;
	dlg.DoModal();

    xlog::info("Closing the launcher");
	return 0;
}

void LauncherApp::MigrateConfig()
{
    try {
        GameConfig config;
        if (config.load() && config.dash_faction_version.value() != VERSION_STR) {
            xlog::info("Migrating config");
            if (config.tracker.value() == "rf.thqmultiplay.net" && config.dash_faction_version->empty()) // < 1.1.0
                config.tracker = GameConfig::default_rf_tracker;
            config.dash_faction_version = VERSION_STR;
            config.save();
        }
    }
    catch (std::exception&) {
        // ignore
    }
}

bool LauncherApp::LaunchGame(HWND hwnd, const char* mod_name)
{
    WatchDogTimer::ScopedStartStop wdt_start{m_watch_dog_timer};
    GameLauncher launcher;
    auto exe_path_opt = m_cmd_line_info.GetExePath();
    if (exe_path_opt) {
        launcher.set_app_exe_path(exe_path_opt.value());
    }
    if (mod_name) {
        launcher.set_mod(mod_name);
    }
    launcher.set_args(m_cmd_line_info.GetPassThroughArgs());

    try {
        xlog::info("Checking installation");
        launcher.check_installation();
        xlog::info("Installation is okay");
    }
    catch (FileNotFoundException &e) {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File is missing:\n" << e.get_file_name() << "\n"
            << "Please make sure game executable specified in options is located inside a valid Red Faction installation "
            << "root directory.";
        std::string str = ss.str();
        Message(hwnd, str.c_str(), nullptr, MB_OK | MB_ICONWARNING);
        return false;
    }
    catch (FileHashVerificationException &e) {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File " << e.get_file_name() << " has unrecognized hash sum.\n\n"
            << "SHA1:\n" << e.get_sha1();
        if (e.get_file_name() == "tables.vpp") {
            ss << "\n\nIt can prevent multiplayer functionality or entire game from working properly.\n"
                << "If your game has not been updated to 1.20 please do it first. If this warning still shows up "
                << "replace your tables.vpp file with original 1.20 NA " << e.get_file_name() << " available on FactionFiles.com.\n"
                << "Do you want to open download page?";
            std::string str = ss.str();
            download_url = "https://www.factionfiles.com/ff.php?action=file&id=517871";
            int result = Message(hwnd, str.c_str(), nullptr, MB_YESNOCANCEL | MB_ICONWARNING);
            if (result == IDYES) {
                ShellExecuteA(hwnd, "open", download_url.c_str(), nullptr, nullptr, SW_SHOW);
                return false;
            }
            if (result == IDCANCEL) {
                return false;
            }
        }
        else {
            std::string str = ss.str();
            if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL) {
                return false;
            }
        }
    }

    try {
        xlog::info("Launching the game...");
        launcher.launch();
        xlog::info("Game launched!");
        return true;
    }
    catch (PrivilegeElevationRequiredException&){
        Message(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileNotFoundException&) {
        Message(hwnd, "Game executable has not been found. Please set a proper path in Options.",
                nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileHashVerificationException &e) {
        std::stringstream ss;
        ss << "Unsupported game executable has been detected!\n\n"
            << "SHA1:\n" << e.get_sha1() << "\n\n"
            << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
            << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
            << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
            << "Click OK to open download page.";
        std::string str = ss.str();
        if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONERROR) == IDOK) {
            ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", nullptr, nullptr,
                SW_SHOW);
        }
    }
    catch (std::exception &e) {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    }
    return false;
}

bool LauncherApp::LaunchEditor(HWND hwnd, const char* mod_name)
{
    WatchDogTimer::ScopedStartStop wdt_start{m_watch_dog_timer};
    EditorLauncher launcher;
    auto exe_path_opt = m_cmd_line_info.GetExePath();
    if (exe_path_opt) {
        launcher.set_app_exe_path(exe_path_opt.value());
    }
    if (mod_name) {
        launcher.set_mod(mod_name);
    }
    launcher.set_args(m_cmd_line_info.GetPassThroughArgs());

    try {
        xlog::info("Launching editor...");
        launcher.launch();
        xlog::info("Editor launched!");
        return true;
    }
    catch (std::exception &e) {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
}

int LauncherApp::Message(HWND hwnd, const char *text, const char *title, int flags)
{
    WatchDogTimer::ScopedPause wdt_pause{m_watch_dog_timer};
    xlog::info("{}: {}", title ? title : "Error", text);
    bool no_gui = GetSystemMetrics(SM_CMONITORS) == 0;
    if (no_gui) {
        std::fprintf(stderr, "%s: %s", title ? title : "Error", text);
        return -1;
    }
    return MessageBoxA(hwnd, text, title, flags);
}

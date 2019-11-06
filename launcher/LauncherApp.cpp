
// DashFactionLauncher.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "LauncherCommandLineInfo.h"
#include <common/GameConfig.h>
#include <common/version.h>
#include <common/ErrorUtils.h>
#include <launcher_common/PatchedAppLauncher.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// LauncherApp initialization
BOOL LauncherApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
    Win32xx::LoadCommonControls();

    // Command line parsing
    LauncherCommandLineInfo cmd_line_info;
    cmd_line_info.Parse();

    if (cmd_line_info.HasHelpFlag())
    {
        // Note: we can't use stdio console API in win32 application
        Message(NULL,
            "Usage: DashFactionLauncher [-game] [-level name] [-editor] args...\n"
            "-game        Starts game immediately\n"
            "-level name  Starts game immediately and loads specified level\n"
            "-editor      Starts level editor immediately\n"
            "args...      Additional arguments passed to game or editor\n",
            "Dash Faction Launcher Help", MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    // Migrate Dash Faction config from old version
    MigrateConfig();

    // Launch game or editor based on command line flag
    if (cmd_line_info.HasGameFlag())
    {
        LaunchGame(nullptr);
        return FALSE;
    }

    if (cmd_line_info.HasEditorFlag())
    {
        LaunchEditor(nullptr);
        return FALSE;
    }

    // Show main dialog
	MainDlg dlg;
	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void LauncherApp::MigrateConfig()
{
    try
    {
        GameConfig config;
        if (config.load() && config.dash_faction_version != VERSION_STR)
        {
            if (config.tracker == "rf.thqmultiplay.net" && config.dash_faction_version.empty()) // < 1.1.0
                config.tracker = DEFAULT_RF_TRACKER;
            config.dash_faction_version = VERSION_STR;
            config.save();
        }
    }
    catch (std::exception&)
    {
        // ignore
    }
}

bool LauncherApp::LaunchGame(HWND hwnd, const char* mod_name)
{
    GameLauncher launcher;

    try
    {
        launcher.check_installation();
    }
    catch (InstallationCheckFailedException &e)
    {
        std::stringstream ss;
        std::string download_url;

        if (e.getCrc32() == 0)
        {
            ss << "Game directory validation has failed: " << e.getFilename() << " file is missing!\n"
                << "Please make sure game executable specified in options is located inside a valid Red Faction installation "
                << "root directory.";
        }
        else
        {
            ss << "Game directory validation has failed: invalid " << e.getFilename() << " file has been detected "
                << "(CRC32 = 0x" << std::hex << e.getCrc32() << ").";
            if (e.getFilename() == std::string("tables.vpp"))
            {
                ss << "\nIt can prevent multiplayer functionality or entire game from working properly.\n"
                    << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
                    << "replace your tables.vpp file with original 1.20 NA " << e.getFilename() << " available on FactionFiles.com.\n"
                    << "Click OK to open download page. Click Cancel to skip this warning.";
                download_url = "https://www.factionfiles.com/ff.php?action=file&id=517871";
            }

        }
        std::string str = ss.str();
        if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONWARNING) == IDOK)
        {
            if (!download_url.empty())
                ShellExecuteA(hwnd, "open", download_url.c_str(), nullptr, nullptr, SW_SHOW);
            return true;
        }
    }

    try
    {
        launcher.launch(mod_name);
        return true;
    }
    catch (PrivilegeElevationRequiredException&)
    {
        Message(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            nullptr, MB_OK | MB_ICONERROR);
        return false;
    }
    catch (IntegrityCheckFailedException &e)
    {
        if (e.getCrc32() == 0)
        {
            Message(hwnd, "Game executable has not been found. Please set a proper path in Options.",
                nullptr, MB_OK | MB_ICONERROR);
        }
        else
        {
            std::stringstream ss;
            ss << "Unsupported game executable has been detected (CRC32 = 0x" << std::hex << e.getCrc32() << "). "
                << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
                << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
                << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
                << "Click OK to open download page.";
            std::string str = ss.str();
            if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONERROR) == IDOK)
                ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
        }
        return false;
    }
    catch (std::exception &e)
    {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
    return true;
}

bool LauncherApp::LaunchEditor(HWND hwnd, const char* mod_name)
{
    EditorLauncher launcher;
    try
    {
        launcher.launch(mod_name);
        return true;
    }
    catch (std::exception &e)
    {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
}

int LauncherApp::Message(HWND hwnd, const char *pszText, const char *pszTitle, int Flags)
{
    if (GetSystemMetrics(SM_CMONITORS) > 0)
        return MessageBoxA(hwnd, pszText, pszTitle, Flags);
    else
    {
        fprintf(stderr, "%s: %s", pszTitle ? pszTitle : "Error", pszText);
        return -1;
    }
}

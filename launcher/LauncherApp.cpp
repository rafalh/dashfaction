
// DashFactionLauncher.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "LauncherCommandLineInfo.h"
#include <common/GameConfig.h>
#include <common/version.h>
#include <launcher_common/PatchedAppLauncher.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// LauncherApp

BEGIN_MESSAGE_MAP(LauncherApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// LauncherApp construction

LauncherApp::LauncherApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only LauncherApp object

LauncherApp theApp;


// LauncherApp initialization

BOOL LauncherApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Command line parsing
    LauncherCommandLineInfo CmdLineInfo;
    ParseCommandLine(CmdLineInfo);

    if (CmdLineInfo.HasHelpFlag())
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
    if (CmdLineInfo.HasGameFlag())
    {
        LaunchGame(nullptr);
        return FALSE;
    }

    if (CmdLineInfo.HasEditorFlag())
    {
        LaunchEditor(nullptr);
        return FALSE;
    }

    // Show main dialog
	MainDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif

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

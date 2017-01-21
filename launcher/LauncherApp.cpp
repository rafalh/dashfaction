
// DashFactionLauncher.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "GameStarter.h"
#include "GameConfig.h"

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

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Dash Faction"));
    
    if (strstr(m_lpCmdLine, "-h"))
    {
        printf(
            "Usage: DashFactionLauncher [-game] [-level name] [-editor] args...\n"
            "-game        Starts game immediately\n"
            "-level name  Starts game immediately and loads specified level\n"
            "-editor      Starts level editor immediately\n"
            "args...      Additional arguments passed to game or editor\n");
        return FALSE;
    }
    
    if (strstr(m_lpCmdLine, "-game") || strstr(m_lpCmdLine, "-level"))
    {
        LaunchGame(nullptr);
        return FALSE;
    }

    if (strstr(m_lpCmdLine, "-editor"))
    {
        LaunchEditor(nullptr);
        return FALSE;
    }

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

bool LauncherApp::LaunchGame(HWND hwnd)
{
    GameStarter starter;
    try
    {
        starter.launch();
        return true;
    }
    catch (PrivilegeElevationRequiredException&)
    {
        MessageBoxA(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            NULL, MB_OK | MB_ICONERROR);
        return false;
    }
    catch (IntegrityCheckFailedException &e)
    {
        std::stringstream ss;
        if (e.getCrc32() == 0)
            ss << "Game executable has not been found. Please set a proper path in Options.";
        else
        {
            ss << "Unsupported game executable has been detected (CRC32 = 0x" << std::hex << e.getCrc32() << "). "
                << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
                << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
                << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
                << "Click OK to open download page.";
        }
        std::string str = ss.str();
        if (MessageBoxA(hwnd, str.c_str(), NULL, MB_OKCANCEL | MB_ICONERROR) == IDOK)
            ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
        return false;
    }
    catch (std::exception &e)
    {
        MessageBoxA(hwnd, e.what(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
    return true;
}

bool LauncherApp::LaunchEditor(HWND hwnd)
{
    GameConfig conf;
    conf.load();
    size_t pos = conf.gameExecutablePath.find_last_of("\\/");
    std::string gameWorkDir = conf.gameExecutablePath.substr(0, pos);
    std::string editorPath = gameWorkDir + "\\RED.exe";
    HINSTANCE res = ShellExecuteA(NULL, "open", editorPath.c_str(), GetCommandLineA(), gameWorkDir.c_str(), SW_SHOW);
    if (res < (HINSTANCE)32)
    {
        std::stringstream ss;
        ss << "Failed to start Level Editor. ShellExecute returned error " << (int)res << ".\n"
            << "Verify if game executalbe path in Options is valid.";
        std::string str = ss.str();
        MessageBoxA(hwnd, str.c_str(), NULL, MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}


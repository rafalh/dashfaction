
// DashFactionLauncher.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// LauncherApp:
// See DashFactionLauncher.cpp for the implementation of this class
//

class LauncherApp : public CWinApp
{
public:
	LauncherApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
    bool LaunchGame(HWND hwnd);
    bool LaunchEditor(HWND hwnd);

	DECLARE_MESSAGE_MAP()
};

extern LauncherApp theApp;
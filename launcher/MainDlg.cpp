
// MainDlg.cpp : implementation file
//

#include "stdafx.h"

#include <common/GameConfig.h>
#include "LauncherApp.h"
#include "MainDlg.h"
#include "OptionsDlg.h"
#include <common/version.h>
#include <common/ErrorUtils.h>
#include <launcher_common/PatchedAppLauncher.h>
#include <launcher_common/UpdateChecker.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <cstring>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// MainDlg dialog

#define WM_UPDATE_CHECK (WM_USER + 10)

MainDlg::MainDlg() : CDialog(IDD_MAIN)
{
    m_hIcon = GetApp().LoadIcon(IDR_ICON);
}

// MainDlg message handlers

BOOL MainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowTextA(PRODUCT_NAME_VERSION);

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);  // Set big icon
    SetIcon(m_hIcon, FALSE); // Set small icon

    // Set header bitmap
    picture.AttachDlgItem(IDC_HEADER_PIC, *this);
    HBITMAP hbm = (HBITMAP)GetApp().LoadImage(MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, 0);
    picture.SetBitmap(hbm);

    mod_selector.AttachDlgItem(IDC_MOD_COMBO, *this);

    // Fill mod selector
    RefreshModSelector();

    m_ToolTip.Create(*this);
    m_ToolTip.Activate(TRUE);

#ifdef NDEBUG
    m_pUpdateChecker = new UpdateChecker(*this);
    m_pUpdateChecker->check_async([=]() { PostMessageA(WM_UPDATE_CHECK, 0, 0); });
#else
    m_pUpdateChecker = nullptr;
#endif

    return TRUE; // return TRUE  unless you set the focus to a control
}

void MainDlg::OnOK()
{
    OnBnClickedOk();
}

BOOL MainDlg::OnCommand(WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(lparam);

    UINT id = LOWORD(wparam);
    switch (id)
    {
    case IDC_OPTIONS_BTN:
        OnBnClickedOptionsBtn();
        return TRUE;
    case IDC_EDITOR_BTN:
        OnBnClickedEditorBtn();
        return TRUE;
    case IDC_SUPPORT_BTN:
        OnBnClickedSupportBtn();
        return TRUE;
    }

    return FALSE;
}

INT_PTR MainDlg::DialogProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_UPDATE_CHECK)
        return OnUpdateCheck(wparam, lparam);
    return CDialog::DialogProc(msg, wparam, lparam);
}

void MainDlg::RefreshModSelector()
{
    CString selected_mod;
    selected_mod = mod_selector.GetWindowText();
    mod_selector.ResetContent();
    mod_selector.AddString("");

    GameConfig game_config;
    try {
        game_config.load();
    }
    catch (...) {
        return;
    }
    std::string game_dir = get_dir_from_path(game_config.game_executable_path);
    std::string mods_dir = game_dir + "\\mods\\*";
    WIN32_FIND_DATA fi;

    HANDLE hfind = FindFirstFileExA(mods_dir.c_str(), FindExInfoBasic, &fi, FindExSearchLimitToDirectories, NULL, 0);
    if (hfind != INVALID_HANDLE_VALUE) {
        do {
            if (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && std::strcmp(fi.cFileName, ".") != 0 &&
                std::strcmp(fi.cFileName, "..") != 0) {
                mod_selector.AddString(fi.cFileName);
            }
        } while (FindNextFileA(hfind, &fi));
        FindClose(hfind);
    }

    mod_selector.SetWindowTextA(selected_mod);
}

BOOL MainDlg::PreTranslateMessage(MSG& Msg)
{
    if (m_ToolTip)
        m_ToolTip.RelayEvent(Msg);

    return CDialog::PreTranslateMessage(Msg);
}

LRESULT MainDlg::OnUpdateCheck(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CStatic updateStatus;
    updateStatus.AttachDlgItem(IDC_UPDATE_STATUS, *this);

    if (m_pUpdateChecker->has_error()) {
        updateStatus.SetWindowText("Failed to check for update");
        m_ToolTip.AddTool(updateStatus, m_pUpdateChecker->get_error().c_str());
        return 0;
    }

    if (!m_pUpdateChecker->is_new_version_available())
        updateStatus.SetWindowText("No update is available.");
    else {
        updateStatus.SetWindowText("New version available!");
        int iResult = MessageBoxA(m_pUpdateChecker->get_message().c_str(), "Dash Faction update is available!",
                                  MB_OKCANCEL | MB_ICONEXCLAMATION);
        if (iResult == IDOK) {
            ShellExecuteA(*this, "open", m_pUpdateChecker->get_url().c_str(), NULL, NULL, SW_SHOW);
            EndDialog(0);
        }
    }

    return 0;
}

void MainDlg::OnBnClickedOptionsBtn()
{
    try {
        OptionsDlg dlg;
        INT_PTR nResponse = dlg.DoModal(*this);
        RefreshModSelector();
    }
    catch (std::exception& e) {
        std::string msg = generate_message_for_exception(e);
        MessageBoxA(msg.c_str(), NULL, MB_ICONERROR | MB_OK);
    }
}

void MainDlg::OnBnClickedOk()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    CString selected_mod = GetSelectedMod();

    if (static_cast<LauncherApp&>(GetApp()).LaunchGame(*this, selected_mod))
        AfterLaunch();
}

void MainDlg::OnBnClickedEditorBtn()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    CStringA selected_mod = GetSelectedMod();
    if (static_cast<LauncherApp&>(GetApp()).LaunchEditor(*this, selected_mod))
        AfterLaunch();
}

void MainDlg::OnBnClickedSupportBtn()
{
    ShellExecuteA(*this, "open", "https://discord.gg/bC2WzvJ", NULL, NULL, SW_SHOW);
}

CString MainDlg::GetSelectedMod()
{
    CString selected_mod = mod_selector.GetWindowText();
    return selected_mod;
}

void MainDlg::AfterLaunch()
{
    GameConfig config;
    try {
        config.load();
    }
    catch (...) {
        // ignore
    }

    if (!config.keep_launcher_open)
        CDialog::OnOK();
}

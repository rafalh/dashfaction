#include "MainDlg.h"
#include "LauncherApp.h"
#include "OptionsDlg.h"
#include "AboutDlg.h"
#include <wxx_wincore.h>
#include <xlog/xlog.h>
#include <cstring>
#include <common/version.h>
#include <common/ErrorUtils.h>
#include <launcher_common/PatchedAppLauncher.h>
#include <launcher_common/UpdateChecker.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATE_CHECK (WM_USER + 10)

MainDlg::MainDlg() : CDialog(IDD_MAIN)
{
}

MainDlg::~MainDlg()
{
}

BOOL MainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog
    SetIconLarge(IDR_ICON);
    SetIconSmall(IDR_ICON);

    // Attach controls
    AttachItem(IDC_HEADER_PIC, m_picture);
    AttachItem(IDC_MOD_COMBO, m_mod_selector);
    AttachItem(IDC_UPDATE_STATUS, m_update_status);

    // Set header bitmap
    HBITMAP hbm = (HBITMAP)GetApp()->LoadImage(MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, 0);
    m_picture.SetBitmap(hbm);


    // Fill mod selector
    RefreshModSelector();

    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(m_mod_selector, "Select a game mod (you can download them from FactionFiles.com)");

#ifdef NDEBUG
    m_update_checker.check_async([=]() { PostMessageA(WM_UPDATE_CHECK, 0, 0); });
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
    switch (id) {
    case IDC_OPTIONS_BTN:
        OnBnClickedOptionsBtn();
        return TRUE;
    case IDC_EDITOR_BTN:
        OnBnClickedEditorBtn();
        return TRUE;
    }

    return FALSE;
}

LRESULT MainDlg::OnNotify([[ maybe_unused ]] WPARAM wparam, LPARAM lparam)
{
    auto& nmhdr = *reinterpret_cast<LPNMHDR>(lparam);
    switch (nmhdr.code) {
    case NM_CLICK:
        // fall through
    case NM_RETURN:
        if (nmhdr.idFrom == IDC_ABOUT_LINK) {
            OnAboutLinkClick();
        }
        else if (nmhdr.idFrom == IDC_SUPPORT_LINK) {
            OnSupportLinkClick();
        }
        break;
    }
    return 0;
}

INT_PTR MainDlg::DialogProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_UPDATE_CHECK)
        return OnUpdateCheck(wparam, lparam);
    return CDialog::DialogProc(msg, wparam, lparam);
}

void MainDlg::RefreshModSelector()
{
    xlog::info("Refreshing mods list");
    CString selected_mod;
    selected_mod = m_mod_selector.GetWindowText();
    m_mod_selector.ResetContent();
    m_mod_selector.AddString("");

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
                m_mod_selector.AddString(fi.cFileName);
            }
        } while (FindNextFileA(hfind, &fi));
        FindClose(hfind);
    }

    m_mod_selector.SetWindowTextA(selected_mod);
}

LRESULT MainDlg::OnUpdateCheck(WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(wparam);
    UNREFERENCED_PARAMETER(lparam);

    UpdateChecker::CheckResult chk_result;
    try {
        chk_result = m_update_checker.get_result();
    }
    catch (std::exception& e) {
        m_update_status.SetWindowText("Failed to check for update");
        m_tool_tip.AddTool(m_update_status, e.what());
        return 0;
    }

    if (!chk_result)
        m_update_status.SetWindowText("No update is available.");
    else {
        m_update_status.SetWindowText("New version available!");
        int result = MessageBoxA(chk_result.message.c_str(), "Dash Faction update is available!",
                                  MB_OKCANCEL | MB_ICONEXCLAMATION);
        if (result == IDOK) {
            auto exec_ret = ShellExecuteA(*this, "open", chk_result.url.c_str(), NULL, NULL, SW_SHOW);
            if (reinterpret_cast<int>(exec_ret) <= 32) {
                xlog::error("ShellExecuteA failed %p", exec_ret);
            }
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
    CString selected_mod = GetSelectedMod();
    if (GetLauncherApp()->LaunchGame(*this, selected_mod))
        AfterLaunch();
}

void MainDlg::OnBnClickedEditorBtn()
{
    CStringA selected_mod = GetSelectedMod();
    if (GetLauncherApp()->LaunchEditor(*this, selected_mod))
        AfterLaunch();
}

void MainDlg::OnSupportLinkClick()
{
    xlog::info("Opening support channel");
    auto ret = ShellExecuteA(*this, "open", "https://discord.gg/bC2WzvJ", NULL, NULL, SW_SHOW);
    if (reinterpret_cast<int>(ret) <= 32) {
        xlog::error("ShellExecuteA failed %p", ret);
    }
}

void MainDlg::OnAboutLinkClick()
{
    AboutDlg dlg;
    dlg.DoModal(*this);
}

CString MainDlg::GetSelectedMod()
{
    return m_mod_selector.GetWindowText();
}

void MainDlg::AfterLaunch()
{
    xlog::info("Checking if launcher should be closed");
    GameConfig config;
    try {
        config.load();
    }
    catch (...) {
        // ignore
    }

    if (!config.keep_launcher_open) {
        xlog::info("Closing launcher after launch");
        CDialog::OnOK();
    }
}

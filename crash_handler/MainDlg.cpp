#include "CrashReportApp.h"
#include "MainDlg.h"
#include "AdditionalInfoDlg.h"
#include <common/version.h>
#include <common/ErrorUtils.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include "res/resource.h"

MainDlg::MainDlg() : CDialog(IDD_MAIN)
{
}

BOOL MainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Update text in header
    auto new_text = GetDlgItemText(IDC_HEADER_TEXT);
    new_text.Replace("@PROGRAM_NAME@", PRODUCT_NAME);
    SetDlgItemText(IDC_HEADER_TEXT, new_text);

    // Make header font bold
    CWnd header_text;
    AttachItem(IDC_HEADER_TEXT, header_text);
    auto font = header_text.GetFont();
    auto log_font = header_text.GetFont().GetLogFont();
    log_font.lfWeight = FW_BOLD;
    font.CreateFontIndirect(log_font);
    header_text.SetFont(font);
    font.Detach();
    header_text.Detach();

    // Attach controls
    AttachItem(IDC_HEADER_PIC, m_picture);

    // Set header bitmap
    auto hbm = static_cast<HBITMAP>(GetApp()->LoadImage(MAKEINTRESOURCE(IDB_HDR_PIC), IMAGE_BITMAP, 0, 0, 0));
    m_picture.SetBitmap(hbm);

    return TRUE; // return TRUE unless you set the focus to a control
}

INT_PTR MainDlg::DialogProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_CTLCOLORSTATIC) {
        // Make header background white
        auto ctrl = reinterpret_cast<HWND>(lparam);
        if (ctrl == GetDlgItem(IDC_HEADER_BACKGROUND) || ctrl == GetDlgItem(IDC_HEADER_TEXT))
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
    }
    return CDialog::DialogProc(msg, wparam, lparam);
}

LRESULT MainDlg::OnNotify([[ maybe_unused ]] WPARAM wparam, LPARAM lparam)
{
    auto& nmhdr = *reinterpret_cast<LPNMHDR>(lparam);
    switch (nmhdr.code)
    {
    case NM_CLICK:
        // Fall through to the next case.
    case NM_RETURN:
        if (nmhdr.idFrom == IDC_OPEN_REPORT) {
            OpenArchivedReport();
        }
        else if (nmhdr.idFrom == IDC_ADDITIONAL_INFO) {
            OpenAdditionalInfoDlg();
        }
        break;
    }
    return 0;
}

void MainDlg::OpenArchivedReport()
{
    auto archived_report_path = GetCrashReportApp()->GetArchivedReportFilePath();
    auto ret = ShellExecuteA(GetHwnd(), nullptr, archived_report_path.c_str(), nullptr, nullptr, SW_SHOW);
    if (reinterpret_cast<int>(ret) <= 32) {
        // ShellExecuteA failed - fallback to opening the directory
        CString args;
        args = "/select,";
        args += archived_report_path.c_str();
        ShellExecuteA(GetHwnd(), nullptr, "explorer.exe", args, nullptr, SW_SHOW);
    }
}

void MainDlg::OpenAdditionalInfoDlg()
{
    AdditionalInfoDlg dlg{m_additional_info};
    if (dlg.DoModal(GetHwnd()) == IDOK)
        m_additional_info = dlg.GetInfo();
}

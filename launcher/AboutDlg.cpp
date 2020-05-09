#include "resource.h"
#include "AboutDlg.h"
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <xlog/xlog.h>
#include <common/version.h>

AboutDlg::AboutDlg()
	: CDialog(IDD_ABOUT)
{
}

BOOL AboutDlg::OnInitDialog()
{
    // Change font of application name label
    CWnd app_name_label;
    AttachItem(IDC_APP_NAME, app_name_label);
    auto font = app_name_label.GetFont();
    auto log_font = app_name_label.GetFont().GetLogFont();
    log_font.lfWeight = FW_BOLD;
    log_font.lfHeight = 20;
    font.CreateFontIndirect(log_font);
    app_name_label.SetFont(font);
    font.Detach();
    app_name_label.Detach();

    // Set version label text
    // Note: it is set here to avoid joining strings in .rc file (it is problematic in MSVC and not supported by
    // visual editor)
    SetDlgItemTextA(IDC_APP_VERSION, "Version: " VERSION_STR);

    return TRUE;
}

LRESULT AboutDlg::OnNotify([[ maybe_unused ]] WPARAM wparam, LPARAM lparam)
{
    auto& nmhdr = *reinterpret_cast<LPNMHDR>(lparam);
    switch (nmhdr.code) {
    case NM_CLICK:
        // Fall through to the next case.
    case NM_RETURN:
        if (nmhdr.idFrom == IDC_LICENSE_LINK) {
            OpenLicensingInfo();
        }
        else if (nmhdr.idFrom == IDC_SRC_LINK) {
            auto& nmlink = *reinterpret_cast<PNMLINK>(lparam);
            ShellExecuteW(NULL, L"open", nmlink.item.szUrl, NULL, NULL, SW_SHOW);
        }
        break;
    }
    return 0;
}

inline std::wstring get_dir_from_path(const std::wstring& path)
{
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L".";
    return path.substr(0, pos);
}

void AboutDlg::OpenLicensingInfo()
{
    wchar_t buf[MAX_PATH];
    DWORD result = GetModuleFileNameW(nullptr, buf, std::size(buf));
    if (!result) {
        xlog::error("GetModuleFileNameW failed: %lu", GetLastError());
        return;
    }

    std::wstring licenses_path = get_dir_from_path(buf) + L"\\licenses.txt";
    auto exec_result = ShellExecuteW(*this, L"open", licenses_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<int>(exec_result) <= 32) {
        xlog::error("ShellExecuteA failed %p", exec_result);
    }
}

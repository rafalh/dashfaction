
// MainDlg.h : header file
//

#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class UpdateChecker;

// MainDlg dialog
class MainDlg : public CDialog
{
// Construction
public:
    MainDlg();	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MAIN };
#endif

protected:
    void RefreshModSelector();
    CString GetSelectedMod();
    void AfterLaunch();
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;

// Implementation
protected:
    HICON m_hIcon;
    UpdateChecker *m_pUpdateChecker;
    CToolTip m_ToolTip;
    CStatic picture;
    CComboBox mod_selector;

	// Generated message map functions
    BOOL OnInitDialog() override;
    INT_PTR DialogProc(UINT msg, WPARAM wparam, LPARAM lparam) override;
    BOOL PreTranslateMessage(MSG& Msg) override;

    LRESULT OnUpdateCheck(WPARAM wParam, LPARAM lParam);
public:
    void OnBnClickedOptionsBtn();
    void OnBnClickedOk();
    void OnBnClickedEditorBtn();
    void OnBnClickedSupportBtn();
};

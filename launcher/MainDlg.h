
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
public:
    // constructor
    MainDlg();

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;
    INT_PTR DialogProc(UINT msg, WPARAM wparam, LPARAM lparam) override;
    BOOL PreTranslateMessage(MSG& Msg) override;

private:
    LRESULT OnUpdateCheck(WPARAM wParam, LPARAM lParam);
    void OnBnClickedOptionsBtn();
    void OnBnClickedOk();
    void OnBnClickedEditorBtn();
    void OnBnClickedSupportBtn();
    void RefreshModSelector();
    CString GetSelectedMod();
    void AfterLaunch();

protected:
    // Controls
    CStatic m_picture;
    CStatic m_update_status;
    CComboBox m_mod_selector;

    UpdateChecker *m_pUpdateChecker;
    CToolTip m_ToolTip;
};

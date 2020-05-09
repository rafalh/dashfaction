#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>
#include <memory>

class UpdateChecker;

class MainDlg : public CDialog
{
public:
    MainDlg();
    ~MainDlg();

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;
    LRESULT OnNotify(WPARAM wparam, LPARAM lparam) override;
    INT_PTR DialogProc(UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
    LRESULT OnUpdateCheck(WPARAM wParam, LPARAM lParam);
    void OnBnClickedOptionsBtn();
    void OnBnClickedOk();
    void OnBnClickedEditorBtn();
    void OnSupportLinkClick();
    void OnAboutLinkClick();
    void RefreshModSelector();
    CString GetSelectedMod();
    void AfterLaunch();

protected:
    // Controls
    CStatic m_picture;
    CStatic m_update_status;
    CComboBox m_mod_selector;

    std::unique_ptr<UpdateChecker> m_update_checker;
    CToolTip m_tool_tip;
};

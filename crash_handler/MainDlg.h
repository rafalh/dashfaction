#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_stdcontrols.h>
#include <optional>

class MainDlg : public CDialog
{
public:
    MainDlg();

    std::optional<std::string> GetAdditionalInfo() const
    {
        return m_additional_info;
    }

protected:
    BOOL OnInitDialog() override;
    INT_PTR DialogProc(UINT msg, WPARAM wparam, LPARAM lparam) override;
    LRESULT OnNotify(WPARAM wparam, LPARAM lparam) override;

private:
    void OpenArchivedReport();
    void OpenAdditionalInfoDlg();

    CStatic m_picture;
    std::optional<std::string> m_additional_info;
};

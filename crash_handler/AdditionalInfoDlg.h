#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>
#include "res/resource.h"

class AdditionalInfoDlg : public CDialog
{
    std::optional<std::string> m_info;

public:
    AdditionalInfoDlg(std::optional<std::string> info) :
        CDialog(IDD_ADDITIONAL_INFO),
        m_info(info)
    {
    }

    std::optional<std::string> GetInfo() const
    {
        return m_info;
    }

protected:
    BOOL OnInitDialog() override
    {
        if (m_info)
            SetDlgItemText(IDC_ADDITIONAL_INFO_EDIT, m_info.value().c_str());
        return TRUE;
    }

    void OnOK() override
    {
        auto text = GetDlgItemText(IDC_ADDITIONAL_INFO_EDIT);
        m_info = std::optional{std::string{text}};
        EndDialog(IDOK);
    }
};

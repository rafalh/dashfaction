#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>

class AboutDlg : public CDialog
{
public:
    AboutDlg();

protected:
    BOOL OnInitDialog() override;
    LRESULT OnNotify([[maybe_unused]] WPARAM wparam, LPARAM lparam) override;
    void OpenLicensingInfo();
};

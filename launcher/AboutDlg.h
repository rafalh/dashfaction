#pragma once

#include <launcher_common/VideoDeviceInfoProvider.h>
#include <common/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class AboutDlg : public CDialog
{
public:
	AboutDlg();

protected:
    BOOL OnInitDialog() override;
    LRESULT OnNotify([[ maybe_unused ]] WPARAM wparam, LPARAM lparam) override;
    void OpenLicensingInfo();
};

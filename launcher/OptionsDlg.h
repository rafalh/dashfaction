#pragma once

#include <launcher_common/VideoDeviceInfoProvider.h>
#include <common/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

// OptionsDlg dialog

class OptionsDlg : public CDialog
{
public:
	OptionsDlg();   // standard constructor

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;

private:
    void InitToolTip();
    void InitResolutionCombo();
    void InitMsaaCombo();
    void InitAnisotropyCheckbox();
    void OnBnClickedOk();
    void OnBnClickedExeBrowse();
    void OnBnClickedResetTrackerBtn();
    void OnForcePortClick();

private:
    VideoDeviceInfoProvider m_videoInfo;
    std::vector<unsigned> m_multiSampleTypes;
    CToolTip m_toolTip;
    GameConfig m_conf;
    CComboBox m_resCombo;
    CComboBox m_msaaCombo;
    CComboBox m_langCombo;
};

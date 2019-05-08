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
	virtual ~OptionsDlg();
    BOOL OnInitDialog() override;
    void InitToolTip();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPTIONS };
#endif

protected:
    BOOL PreTranslateMessage(MSG& pMsg) override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;

public:
    void OnBnClickedOk();
    void OnBnClickedExeBrowse();
    void OnBnClickedResetTrackerBtn();
    void OnForcePortClick();

private:
    VideoDeviceInfoProvider m_videoInfo;
    std::vector<unsigned> m_multiSampleTypes;
    CToolTip *m_toolTip;
    GameConfig m_conf;
    CComboBox resCombo;
    CComboBox msaaCombo;
};

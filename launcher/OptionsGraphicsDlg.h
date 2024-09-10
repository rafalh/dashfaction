#pragma once

#include <launcher_common/VideoDeviceInfoProvider.h>
#include <common/config/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class OptionsGraphicsDlg : public CDialog
{
public:
    OptionsGraphicsDlg(GameConfig& conf);
    void OnSave();
    void OnRendererChange();
    void OnAdapterChange();
    void OnColorDepthChange();
    void OnWindowModeChange();

    void SetVideoInfo(VideoDeviceInfoProvider* video_info)
    {
        m_video_info = video_info;
    }

protected:
    BOOL OnInitDialog() override;

private:
    void InitToolTip();
    void UpdateMsaaCombo();
    void UpdateAnisotropyCheckbox();

    GameConfig& m_conf;
    VideoDeviceInfoProvider* m_video_info;
    std::vector<unsigned> m_multi_sample_types;
    CToolTip m_tool_tip;
    CComboBox m_msaa_combo;
    CComboBox m_clamp_combo;
};

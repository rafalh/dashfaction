#pragma once

#include <common/config/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class OptionsMultiplayerDlg : public CDialog
{
public:
    OptionsMultiplayerDlg(GameConfig& conf);
    void OnSave();

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;

private:
    GameConfig& m_conf;
    CToolTip m_tool_tip;

    void InitToolTip();
    void OnBnClickedResetTrackerBtn();
    void OnForcePortClick();
};

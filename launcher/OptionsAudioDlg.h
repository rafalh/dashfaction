#pragma once

#include <common/config/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class OptionsAudioDlg : public CDialog
{
public:
    OptionsAudioDlg(GameConfig& conf);
    void OnSave();

protected:
    BOOL OnInitDialog() override;

private:
    GameConfig& m_conf;
    CToolTip m_tool_tip;

    void InitToolTip();
};

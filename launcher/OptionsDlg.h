#pragma once

#include <common/config/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>
#include "OptionsDisplayDlg.h"
#include "OptionsGraphicsDlg.h"
#include "OptionsMiscDlg.h"
#include "OptionsAudioDlg.h"
#include "OptionsMultiplayerDlg.h"
#include "OptionsInputDlg.h"
#include "OptionsInterfaceDlg.h"

class OptionsDlg : public CDialog
{
public:
    OptionsDlg();
    OptionsDlg(const OptionsDlg&) = delete;

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM wparam, LPARAM lparam) override;

private:
    void InitToolTip();
    void OnBnClickedOk();
    void OnBnClickedExeBrowse();
    void OnBnClickedResetTrackerBtn();
    void OnForcePortClick();
    void InitNestedDialog(CDialog& dlg, int placeholder_id);

    CToolTip m_tool_tip;
    GameConfig m_conf;
    OptionsDisplayDlg m_display_dlg;
    OptionsGraphicsDlg m_graphics_dlg;
    OptionsMiscDlg m_misc_dlg;
    OptionsAudioDlg m_audio_dlg;
    OptionsMultiplayerDlg m_multiplayer_dlg;
    OptionsInputDlg m_input_dlg;
    OptionsInterfaceDlg m_iface_dlg;
};

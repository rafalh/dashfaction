#pragma once

#include <common/config/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>

class OptionsInterfaceDlg : public CDialog
{
public:
    OptionsInterfaceDlg(GameConfig& conf);
    void OnSave();

protected:
    BOOL OnInitDialog() override;

private:
    GameConfig& m_conf;
    CToolTip m_tool_tip;
    CComboBox m_lang_combo;

    void InitToolTip();
};

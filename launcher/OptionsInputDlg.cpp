#include <wxx_wincore.h>
#include "OptionsInputDlg.h"
#include "LauncherApp.h"
#include <wxx_commondlg.h>

OptionsInputDlg::OptionsInputDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_INPUT), m_conf(conf)
{
}

BOOL OptionsInputDlg::OnInitDialog()
{
    InitToolTip();

    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, m_conf.direct_input ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_LINEAR_PITCH_CHECK, m_conf.linear_pitch ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SWAP_ASSAULT_RIFLE_CONTROLS, m_conf.swap_assault_rifle_controls ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SWAP_GRENADE_CONTROLS, m_conf.swap_grenade_controls ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}

void OptionsInputDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");
    m_tool_tip.AddTool(GetDlgItem(IDC_LINEAR_PITCH_CHECK), "Stop mouse movement from slowing down when looking up and down");
}

void OptionsInputDlg::OnSave()
{
    m_conf.direct_input = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
    m_conf.linear_pitch = (IsDlgButtonChecked(IDC_LINEAR_PITCH_CHECK) == BST_CHECKED);
    m_conf.swap_assault_rifle_controls = (IsDlgButtonChecked(IDC_SWAP_ASSAULT_RIFLE_CONTROLS) == BST_CHECKED);
    m_conf.swap_grenade_controls = (IsDlgButtonChecked(IDC_SWAP_GRENADE_CONTROLS) == BST_CHECKED);
}

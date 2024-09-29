#include <wxx_wincore.h>
#include "OptionsAudioDlg.h"
#include "LauncherApp.h"
#include <wxx_commondlg.h>

OptionsAudioDlg::OptionsAudioDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_AUDIO), m_conf(conf)
{
}

BOOL OptionsAudioDlg::OnInitDialog()
{
    InitToolTip();

    CheckDlgButton(IDC_EAX_SOUND_CHECK, m_conf.eax_sound ? BST_CHECKED : BST_UNCHECKED);
    if (m_conf.level_sound_volume == 1.0f)
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, BST_CHECKED);
    else
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, m_conf.level_sound_volume == 0.0f ? BST_UNCHECKED : BST_INDETERMINATE);

    return TRUE;
}

void OptionsAudioDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_LEVEL_SOUNDS_CHECK), "Enable/disable Play Sound and Ambient Sound objects in level. You can also specify volume multiplier by using levelsounds command in game.");
    m_tool_tip.AddTool(GetDlgItem(IDC_EAX_SOUND_CHECK), "Enable/disable 3D sound and EAX extension if supported");
}

void OptionsAudioDlg::OnSave()
{
    m_conf.eax_sound = (IsDlgButtonChecked(IDC_EAX_SOUND_CHECK) == BST_CHECKED);
    if (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) != BST_INDETERMINATE)
        m_conf.level_sound_volume = (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) == BST_CHECKED ? 1.0f : 0.0f);
}

#include <wxx_wincore.h>
#include "OptionsMiscDlg.h"
#include "LauncherApp.h"
#include <wxx_commondlg.h>

OptionsMiscDlg::OptionsMiscDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_MISC), m_conf(conf)
{
}

BOOL OptionsMiscDlg::OnInitDialog()
{
    InitToolTip();

    CheckDlgButton(IDC_FAST_START_CHECK, m_conf.fast_start ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_ALLOW_OVERWRITE_GAME_CHECK, m_conf.allow_overwrite_game_files ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_REDUCED_SPEED_IN_BG_CHECK, m_conf.reduced_speed_in_background ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PLAYER_JOIN_BEEP_CHECK, m_conf.player_join_beep ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_AUTOSAVE_CHECK, m_conf.autosave ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_DISGUISE_AS_AF_CHECK, m_conf.disguise_as_af ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_IGNORE_LEVEL_VERSION_CHECK, m_conf.ignore_level_version ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}

void OptionsMiscDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_FAST_START_CHECK), "Skip game intro videos and go straight to Main Menu");
    m_tool_tip.AddTool(GetDlgItem(IDC_ALLOW_OVERWRITE_GAME_CHECK), "Enable this if you want to modify game content by putting mods into user_maps folder. Can have side effect of level packfiles modyfing common textures/sounds.");
    m_tool_tip.AddTool(GetDlgItem(IDC_AUTOSAVE_CHECK), "Automatically save the game after a level transition");
    m_tool_tip.AddTool(GetDlgItem(IDC_IGNORE_LEVEL_VERSION_CHECK), "Try loading levels (RFL) with unsupported version. It may result in crashes, so use at your own risk.");
}

void OptionsMiscDlg::OnSave()
{
    m_conf.fast_start = (IsDlgButtonChecked(IDC_FAST_START_CHECK) == BST_CHECKED);
    m_conf.allow_overwrite_game_files = (IsDlgButtonChecked(IDC_ALLOW_OVERWRITE_GAME_CHECK) == BST_CHECKED);
    m_conf.reduced_speed_in_background = (IsDlgButtonChecked(IDC_REDUCED_SPEED_IN_BG_CHECK) == BST_CHECKED);
    m_conf.player_join_beep = (IsDlgButtonChecked(IDC_PLAYER_JOIN_BEEP_CHECK) == BST_CHECKED);
    m_conf.autosave = (IsDlgButtonChecked(IDC_AUTOSAVE_CHECK) == BST_CHECKED);
    m_conf.disguise_as_af = (IsDlgButtonChecked(IDC_DISGUISE_AS_AF_CHECK) == BST_CHECKED);
    m_conf.ignore_level_version = (IsDlgButtonChecked(IDC_IGNORE_LEVEL_VERSION_CHECK) == BST_CHECKED);
}

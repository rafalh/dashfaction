#include <wxx_wincore.h>
#include "OptionsInterfaceDlg.h"
#include "LauncherApp.h"
#include <wxx_commondlg.h>

OptionsInterfaceDlg::OptionsInterfaceDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_INTERFACE), m_conf(conf)
{
}

BOOL OptionsInterfaceDlg::OnInitDialog()
{
    // Attach controls
    AttachItem(IDC_LANG_COMBO, m_lang_combo);

    // Populate combo boxes with static content
    m_lang_combo.AddString("Auto");
    m_lang_combo.AddString("English");
    m_lang_combo.AddString("German");
    m_lang_combo.AddString("French");

    InitToolTip();

    CheckDlgButton(IDC_BIG_HUD_CHECK, m_conf.big_hud ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FPS_COUNTER_CHECK, m_conf.fps_counter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SCOREBOARD_ANIM_CHECK, m_conf.scoreboard_anim);
    CheckDlgButton(IDC_KEEP_LAUNCHER_OPEN_CHECK, m_conf.keep_launcher_open ? BST_CHECKED : BST_UNCHECKED);
    m_lang_combo.SetCurSel(m_conf.language + 1);

    return TRUE;
}

void OptionsInterfaceDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_BIG_HUD_CHECK), "Make HUD bigger in the game");
    m_tool_tip.AddTool(GetDlgItem(IDC_SCOREBOARD_ANIM_CHECK), "Scoreboard open/close animations");
    m_tool_tip.AddTool(GetDlgItem(IDC_FPS_COUNTER_CHECK), "Enable FPS counter in right-top corner of the screen");
    m_tool_tip.AddTool(GetDlgItem(IDC_KEEP_LAUNCHER_OPEN_CHECK), "Keep launcher window open after game or editor launch");
}

void OptionsInterfaceDlg::OnSave()
{
    m_conf.big_hud = (IsDlgButtonChecked(IDC_BIG_HUD_CHECK) == BST_CHECKED);
    m_conf.scoreboard_anim = (IsDlgButtonChecked(IDC_SCOREBOARD_ANIM_CHECK) == BST_CHECKED);
    m_conf.fps_counter = (IsDlgButtonChecked(IDC_FPS_COUNTER_CHECK) == BST_CHECKED);
    m_conf.keep_launcher_open = (IsDlgButtonChecked(IDC_KEEP_LAUNCHER_OPEN_CHECK) == BST_CHECKED);
    m_conf.language = m_lang_combo.GetCurSel() - 1;
}

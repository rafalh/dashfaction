// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "OptionsDlg.h"
#include <common/GameConfig.h>
#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_commondlg.h>


// OptionsDlg dialog

OptionsDlg::OptionsDlg()
	: CDialog(IDD_OPTIONS)
{
}

BOOL OptionsDlg::OnInitDialog()
{
    // Attach controls
    AttachItem(IDC_RESOLUTIONS_COMBO, m_resCombo);
    AttachItem(IDC_MSAA_COMBO, m_msaaCombo);

    try
    {
        m_conf.load();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }

    SetDlgItemTextA(IDC_EXE_PATH_EDIT, m_conf.game_executable_path.c_str());

    InitResolutionCombo();
    InitMsaaCombo();

    CheckDlgButton(IDC_32BIT_RADIO, m_conf.res_bpp == 32 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_16BIT_RADIO, m_conf.res_bpp == 16 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FULL_SCREEN_RADIO, m_conf.wnd_mode == GameConfig::FULLSCREEN ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_WINDOWED_RADIO, m_conf.wnd_mode == GameConfig::WINDOWED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_STRETCHED_RADIO, m_conf.wnd_mode == GameConfig::STRETCHED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_VSYNC_CHECK, m_conf.vsync ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, m_conf.fast_anims ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_RENDERING_CACHE_EDIT, m_conf.geometry_cache_size, false);
    SetDlgItemInt(IDC_MAX_FPS_EDIT, m_conf.max_fps, false);

    InitAnisotropyCheckbox();

    CheckDlgButton(IDC_DISABLE_LOD_CHECK, m_conf.disable_lod_models ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FPS_COUNTER_CHECK, m_conf.fps_counter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, m_conf.high_scanner_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_MON_RES_CHECK, m_conf.high_monitor_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, m_conf.true_color_textures ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemTextA(IDC_TRACKER_EDIT, m_conf.tracker.c_str());
    CheckDlgButton(IDC_FORCE_PORT_CHECK, m_conf.force_port != 0);
    if (m_conf.force_port)
        SetDlgItemInt(IDC_PORT_EDIT, m_conf.force_port, false);
    else
        GetDlgItem(IDC_PORT_EDIT).EnableWindow(FALSE);
    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, m_conf.direct_input ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_EAX_SOUND_CHECK, m_conf.eax_sound ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_START_CHECK, m_conf.fast_start ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SCOREBOARD_ANIM_CHECK, m_conf.scoreboard_anim);
    if (m_conf.level_sound_volume == 1.0f)
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, BST_CHECKED);
    else
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, m_conf.level_sound_volume == 0.0f ? BST_UNCHECKED : BST_INDETERMINATE);
    CheckDlgButton(IDC_ALLOW_OVERWRITE_GAME_CHECK, m_conf.allow_overwrite_game_files ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_KEEP_LAUNCHER_OPEN_CHECK, m_conf.keep_launcher_open ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_LINEAR_PITCH_CHECK, m_conf.linear_pitch ? BST_CHECKED : BST_UNCHECKED);

    InitToolTip();

    return TRUE;
}

void OptionsDlg::InitResolutionCombo()
{
    CString buf;
    int selectedRes = -1;
    try
    {
        auto resolutions = m_videoInfo.get_resolutions(D3DFMT_X8R8G8B8);
        for (const auto &res : resolutions)
        {
            buf.Format("%dx%d", res.width, res.height);
            int pos = m_resCombo.AddString(buf);
            if (m_conf.res_width == res.width && m_conf.res_height == res.height)
                selectedRes = pos;
        }
    }
    catch (std::exception &e)
    {
        // Only 'Disabled' option available. Log error in console.
        printf("Cannot get available screen resolutions: %s", e.what());
    }
    if (selectedRes != -1)
        m_resCombo.SetCurSel(selectedRes);
    else
    {
        char buf[32];
        sprintf(buf, "%dx%d", m_conf.res_width, m_conf.res_height);
        m_resCombo.SetWindowTextA(buf);
    }
}

void OptionsDlg::InitMsaaCombo()
{
    m_msaaCombo.AddString("Disabled");
    int selectedMsaa = 0;
    m_multiSampleTypes.push_back(0);
    try
    {
        auto multiSampleTypes = m_videoInfo.get_multisample_types(D3DFMT_X8R8G8B8, FALSE);
        for (auto msaa : multiSampleTypes)
        {
            char buf[16];
            sprintf(buf, "MSAAx%u", msaa);
            int idx = m_msaaCombo.AddString(buf);
            if (m_conf.msaa == msaa)
                selectedMsaa = idx;
            m_multiSampleTypes.push_back(msaa);
        }
    }
    catch (std::exception &e)
    {
        printf("Cannot check available MSAA modes: %s", e.what());
    }
    m_msaaCombo.SetCurSel(selectedMsaa);
}

void OptionsDlg::InitAnisotropyCheckbox()
{
    bool anisotropySupported = false;
    try
    {
        anisotropySupported = m_videoInfo.has_anisotropy_support();
    }
    catch (std::exception &e)
    {
        printf("Cannot check anisotropy support: %s", e.what());
    }
    if (anisotropySupported)
        CheckDlgButton(IDC_ANISOTROPIC_CHECK, m_conf.anisotropic_filtering ? BST_CHECKED : BST_UNCHECKED);
    else
        GetDlgItem(IDC_ANISOTROPIC_CHECK).EnableWindow(FALSE);
}

void OptionsDlg::InitToolTip()
{
    m_toolTip.Create(*this);

    m_toolTip.AddTool(GetDlgItem(IDC_RESOLUTIONS_COMBO), "Please select resolution from provided dropdown list - custom resolution is supposed to work in Windowed/Stretched mode only");
    m_toolTip.AddTool(GetDlgItem(IDC_STRETCHED_RADIO), "Full Screen Windowed - reduced performance but faster to switch to other window");
    m_toolTip.AddTool(GetDlgItem(IDC_VSYNC_CHECK), "Enable vertical synchronization (should limit FPS to monitor refresh rate - usually 60)");
    m_toolTip.AddTool(GetDlgItem(IDC_MAX_FPS_EDIT), "FPS limit - maximal value is 240 - high FPS can trigger minor bugs in game");
    m_toolTip.AddTool(GetDlgItem(IDC_FAST_ANIMS_CHECK), "Reduce animation smoothness for far models");
    m_toolTip.AddTool(GetDlgItem(IDC_DISABLE_LOD_CHECK), "Improve details for far models");
    m_toolTip.AddTool(GetDlgItem(IDC_ANISOTROPIC_CHECK), "Improve far textures quality");
    m_toolTip.AddTool(GetDlgItem(IDC_FPS_COUNTER_CHECK), "Enable FPS counter in right-top corner of the screen");
    m_toolTip.AddTool(GetDlgItem(IDC_HIGH_SCANNER_RES_CHECK), "Increase scanner resolution (used by Rail Gun, Rocket Launcher and Fusion Launcher)");
    m_toolTip.AddTool(GetDlgItem(IDC_HIGH_MON_RES_CHECK), "Increase monitors and mirrors resolution");
    m_toolTip.AddTool(GetDlgItem(IDC_TRUE_COLOR_TEXTURES_CHECK), "Increase texture color depth - especially visible for lightmaps and shadows");
    m_toolTip.AddTool(GetDlgItem(IDC_TRACKER_EDIT), "Hostname of tracker used to find avaliable Multiplayer servers");
    m_toolTip.AddTool(GetDlgItem(IDC_FAST_START_CHECK), "Skip game intro videos and go straight to Main Menu");
    m_toolTip.AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");
    m_toolTip.AddTool(GetDlgItem(IDC_FORCE_PORT_CHECK), "If not checked automatic port is used");
    m_toolTip.AddTool(GetDlgItem(IDC_SCOREBOARD_ANIM_CHECK), "Scoreboard open/close animations");
    m_toolTip.AddTool(GetDlgItem(IDC_LEVEL_SOUNDS_CHECK), "Enable/disable Play Sound and Ambient Sound objects in level. You can also specify volume multiplier by using levelsounds command in game.");
    m_toolTip.AddTool(GetDlgItem(IDC_ALLOW_OVERWRITE_GAME_CHECK), "Enable this if you want to modify game content by putting mods into user_maps folder. Can have side effect of level packfiles modyfing common textures/sounds.");
    m_toolTip.AddTool(GetDlgItem(IDC_KEEP_LAUNCHER_OPEN_CHECK), "Keep launcher window open after game or editor launch");
    m_toolTip.AddTool(GetDlgItem(IDC_LINEAR_PITCH_CHECK), "Stop mouse movement from slowing down when looking up and down");
}

void OptionsDlg::OnOK()
{
    OnBnClickedOk();
    CDialog::OnOK();
}

BOOL OptionsDlg::OnCommand(WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(lparam);

    UINT id = LOWORD(wparam);
    switch (id)
    {
    case IDC_EXE_BROWSE:
        OnBnClickedExeBrowse();
        return TRUE;
    case IDC_RESET_TRACKER_BTN:
        OnBnClickedResetTrackerBtn();
        return TRUE;
    case IDC_FORCE_PORT_CHECK:
        OnForcePortClick();
        return TRUE;
    }

    return FALSE;
}

void OptionsDlg::OnBnClickedOk()
{
    CString str;

    str = GetDlgItemTextA(IDC_EXE_PATH_EDIT);
    m_conf.game_executable_path = str;

    str = GetDlgItemTextA(IDC_RESOLUTIONS_COMBO);
    char *ptr = (char*)(const char *)str;
    const char *widthStr = strtok(ptr, "x");
    const char *heightStr = strtok(nullptr, "x");
    if (widthStr && heightStr)
    {
        m_conf.res_width = atoi(widthStr);
        m_conf.res_height = atoi(heightStr);
    }

    m_conf.res_bpp = IsDlgButtonChecked(IDC_16BIT_RADIO) == BST_CHECKED ? 16 : 32;
    m_conf.res_backbuffer_format = m_conf.res_bpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;

    if (IsDlgButtonChecked(IDC_FULL_SCREEN_RADIO) == BST_CHECKED)
        m_conf.wnd_mode = GameConfig::FULLSCREEN;
    if (IsDlgButtonChecked(IDC_WINDOWED_RADIO) == BST_CHECKED)
        m_conf.wnd_mode = GameConfig::WINDOWED;
    if (IsDlgButtonChecked(IDC_STRETCHED_RADIO) == BST_CHECKED)
        m_conf.wnd_mode = GameConfig::STRETCHED;

    m_conf.vsync = (IsDlgButtonChecked(IDC_VSYNC_CHECK) == BST_CHECKED);
    m_conf.fast_anims = (IsDlgButtonChecked(IDC_FAST_ANIMS_CHECK) == BST_CHECKED);
    m_conf.geometry_cache_size = GetDlgItemInt(IDC_RENDERING_CACHE_EDIT, false);
    m_conf.max_fps = GetDlgItemInt(IDC_MAX_FPS_EDIT, false);

    m_conf.msaa = m_multiSampleTypes[m_msaaCombo.GetCurSel()];

    m_conf.anisotropic_filtering = (IsDlgButtonChecked(IDC_ANISOTROPIC_CHECK) == BST_CHECKED);
    m_conf.disable_lod_models = (IsDlgButtonChecked(IDC_DISABLE_LOD_CHECK) == BST_CHECKED);
    m_conf.fps_counter = (IsDlgButtonChecked(IDC_FPS_COUNTER_CHECK) == BST_CHECKED);
    m_conf.high_scanner_res = (IsDlgButtonChecked(IDC_HIGH_SCANNER_RES_CHECK) == BST_CHECKED);
    m_conf.high_monitor_res = (IsDlgButtonChecked(IDC_HIGH_MON_RES_CHECK) == BST_CHECKED);
    m_conf.true_color_textures = (IsDlgButtonChecked(IDC_TRUE_COLOR_TEXTURES_CHECK) == BST_CHECKED);

    str = GetDlgItemTextA(IDC_TRACKER_EDIT);
    m_conf.tracker = str;
    bool force_port = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    m_conf.force_port = force_port ? GetDlgItemInt(IDC_PORT_EDIT, false) : 0;
    m_conf.direct_input = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
    m_conf.eax_sound = (IsDlgButtonChecked(IDC_EAX_SOUND_CHECK) == BST_CHECKED);
    m_conf.fast_start = (IsDlgButtonChecked(IDC_FAST_START_CHECK) == BST_CHECKED);
    m_conf.scoreboard_anim = (IsDlgButtonChecked(IDC_SCOREBOARD_ANIM_CHECK) == BST_CHECKED);
    if (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) != BST_INDETERMINATE)
        m_conf.level_sound_volume = (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) == BST_CHECKED ? 1.0f : 0.0f);
    m_conf.allow_overwrite_game_files = (IsDlgButtonChecked(IDC_ALLOW_OVERWRITE_GAME_CHECK) == BST_CHECKED);
    m_conf.keep_launcher_open = (IsDlgButtonChecked(IDC_KEEP_LAUNCHER_OPEN_CHECK) == BST_CHECKED);
    m_conf.linear_pitch = (IsDlgButtonChecked(IDC_LINEAR_PITCH_CHECK) == BST_CHECKED);

    try
    {
        m_conf.save();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }
}

void OptionsDlg::OnBnClickedExeBrowse()
{
    LPCTSTR filter = "Executable Files (*.exe)|*.exe||";
    CFileDialog dlg(TRUE, ".exe", "RF.exe", OFN_HIDEREADONLY, filter);
    dlg.SetTitle("Select game executable (RF.exe)");

    if (dlg.DoModal(*this) == IDOK)
        SetDlgItemText(IDC_EXE_PATH_EDIT, dlg.GetPathName());
}

void OptionsDlg::OnBnClickedResetTrackerBtn()
{
    SetDlgItemTextA(IDC_TRACKER_EDIT, DEFAULT_RF_TRACKER);
}

void OptionsDlg::OnForcePortClick()
{
    bool force_port = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    GetDlgItem(IDC_PORT_EDIT).EnableWindow(force_port);
}

#include <wxx_wincore.h>
#include "OptionsDlg.h"
#include "LauncherApp.h"
#include <xlog/xlog.h>
#include <wxx_dialog.h>
#include <wxx_commondlg.h>

OptionsDlg::OptionsDlg()
	: CDialog(IDD_OPTIONS)
{
}

BOOL OptionsDlg::OnInitDialog()
{
    // Attach controls
    AttachItem(IDC_ADAPTER_COMBO, m_adapter_combo);
    AttachItem(IDC_RESOLUTIONS_COMBO, m_res_combo);
    AttachItem(IDC_COLOR_DEPTH_COMBO, m_color_depth_combo);
    AttachItem(IDC_WND_MODE_COMBO, m_wnd_mode_combo);
    AttachItem(IDC_MSAA_COMBO, m_msaa_combo);
    AttachItem(IDC_LANG_COMBO, m_lang_combo);

    // Populate combo boxes with static content
    m_wnd_mode_combo.AddString("Exclusive Fullscreen");
    m_wnd_mode_combo.AddString("Windowed");
    m_wnd_mode_combo.AddString("Borderless Window");

    m_lang_combo.AddString("Auto");
    m_lang_combo.AddString("English");
    m_lang_combo.AddString("German");
    m_lang_combo.AddString("French");

    try {
        m_conf.load();
    }
    catch (std::exception &e) {
        MessageBoxA(e.what(), nullptr, MB_ICONERROR | MB_OK);
    }

    SetDlgItemTextA(IDC_EXE_PATH_EDIT, m_conf.game_executable_path->c_str());

    // Display
    UpdateAdapterCombo();
    UpdateColorDepthCombo(); // should be before resolution
    UpdateResolutionCombo();
    m_wnd_mode_combo.SetCurSel(static_cast<int>(m_conf.wnd_mode));
    CheckDlgButton(IDC_VSYNC_CHECK, m_conf.vsync ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_RENDERING_CACHE_EDIT, m_conf.geometry_cache_size, false);
    SetDlgItemInt(IDC_MAX_FPS_EDIT, m_conf.max_fps, false);

    // Graphics
    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, m_conf.fast_anims ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_DISABLE_LOD_CHECK, m_conf.disable_lod_models ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, m_conf.high_scanner_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_MON_RES_CHECK, m_conf.high_monitor_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, m_conf.true_color_textures ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_MESH_STATIC_LIGHTING_CHECK, m_conf.mesh_static_lighting ? BST_CHECKED : BST_UNCHECKED);

    // Audio
    CheckDlgButton(IDC_EAX_SOUND_CHECK, m_conf.eax_sound ? BST_CHECKED : BST_UNCHECKED);
    if (m_conf.level_sound_volume == 1.0f)
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, BST_CHECKED);
    else
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, m_conf.level_sound_volume == 0.0f ? BST_UNCHECKED : BST_INDETERMINATE);

    // Multiplayer
    SetDlgItemTextA(IDC_TRACKER_EDIT, m_conf.tracker->c_str());
    CheckDlgButton(IDC_FORCE_PORT_CHECK, m_conf.force_port != 0);
    if (m_conf.force_port)
        SetDlgItemInt(IDC_PORT_EDIT, m_conf.force_port, false);
    else
        GetDlgItem(IDC_PORT_EDIT).EnableWindow(FALSE);
    SetDlgItemInt(IDC_RATE_EDIT, m_conf.update_rate, false);

    // Input
    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, m_conf.direct_input ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_LINEAR_PITCH_CHECK, m_conf.linear_pitch ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SWAP_ASSAULT_RIFLE_CONTROLS, m_conf.swap_assault_rifle_controls ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SWAP_GRENADE_CONTROLS, m_conf.swap_grenade_controls ? BST_CHECKED : BST_UNCHECKED);

    // Interface
    CheckDlgButton(IDC_BIG_HUD_CHECK, m_conf.big_hud ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FPS_COUNTER_CHECK, m_conf.fps_counter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SCOREBOARD_ANIM_CHECK, m_conf.scoreboard_anim);
    m_lang_combo.SetCurSel(m_conf.language + 1);

    // Misc
    CheckDlgButton(IDC_FAST_START_CHECK, m_conf.fast_start ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_ALLOW_OVERWRITE_GAME_CHECK, m_conf.allow_overwrite_game_files ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_KEEP_LAUNCHER_OPEN_CHECK, m_conf.keep_launcher_open ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_REDUCED_SPEED_IN_BG_CHECK, m_conf.reduced_speed_in_background ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PLAYER_JOIN_BEEP_CHECK, m_conf.player_join_beep ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_AUTOSAVE_CHECK, m_conf.autosave ? BST_CHECKED : BST_UNCHECKED);

    InitToolTip();

    return TRUE;
}

void OptionsDlg::UpdateAdapterCombo()
{
    m_adapter_combo.ResetContent();
    int selected_idx = -1;
    try {
        auto adapters = m_video_info.get_adapters();
        for (const auto &adapter : adapters) {
            int idx = m_adapter_combo.AddString(adapter.c_str());
            if (idx < 0)
                throw std::runtime_error("failed to add string to combo box");
            if (m_conf.selected_video_card == static_cast<unsigned>(idx))
                selected_idx = idx;
        }
    }
    catch (std::exception &e) {
        xlog::error("Cannot get video adapters: %s", e.what());
    }
    if (selected_idx != -1)
        m_adapter_combo.SetCurSel(selected_idx);
}

void OptionsDlg::UpdateResolutionCombo()
{
    CString buf;
    int selected_res = -1;
    m_res_combo.ResetContent();
    try {
        auto format = m_conf.res_bpp == 32 ? D3DFMT_X8R8G8B8 : D3DFMT_R5G6B5;
        auto resolutions = m_video_info.get_resolutions(m_conf.selected_video_card, format);
        for (const auto &res : resolutions) {
            buf.Format("%dx%d", res.width, res.height);
            int pos = m_res_combo.AddString(buf);
            if (m_conf.res_width == res.width && m_conf.res_height == res.height)
                selected_res = pos;
        }
    }
    catch (std::exception &e) {
        // Only 'Disabled' option available. Log error in console.
        xlog::error("Cannot get available screen resolutions: %s", e.what());
    }
    if (selected_res != -1)
        m_res_combo.SetCurSel(selected_res);
    else {
        char buf[32];
        sprintf(buf, "%dx%d", m_conf.res_width.value(), m_conf.res_height.value());
        m_res_combo.SetWindowTextA(buf);
    }
}

void OptionsDlg::UpdateColorDepthCombo()
{
    bool has_16bpp_modes = !m_video_info.get_resolutions(m_conf.selected_video_card, D3DFMT_R5G6B5).empty();
    bool has_32bpp_modes = !m_video_info.get_resolutions(m_conf.selected_video_card, D3DFMT_X8R8G8B8).empty();

    if (!has_16bpp_modes) {
        m_conf.res_bpp = 32;
        m_conf.res_backbuffer_format = D3DFMT_X8R8G8B8;
    }
    if (!has_32bpp_modes) {
        m_conf.res_bpp = 16;
        m_conf.res_backbuffer_format = D3DFMT_R5G6B5;
    }
    int index_32 = -1;
    int index_16 = -1;
    if (has_32bpp_modes) {
        index_32 = m_color_depth_combo.AddString("32 bit");
    }
    if (has_16bpp_modes) {
        index_16 = m_color_depth_combo.AddString("16 bit");
    }
    m_color_depth_combo.SetCurSel(m_conf.res_bpp == 16 ? index_16 : index_32);
}

void OptionsDlg::UpdateMsaaCombo()
{
    m_msaa_combo.ResetContent();
    m_msaa_combo.AddString("Disabled");
    int selected_msaa = 0;
    m_multi_sample_types.push_back(0);
    try {
        BOOL windowed = m_conf.wnd_mode != GameConfig::FULLSCREEN;
        auto format = static_cast<D3DFORMAT>(m_conf.res_backbuffer_format.value());
        auto multi_sample_types = m_video_info.get_multisample_types(m_conf.selected_video_card, format, windowed);
        for (auto msaa : multi_sample_types) {
            char buf[16];
            sprintf(buf, "MSAAx%u", msaa);
            int idx = m_msaa_combo.AddString(buf);
            if (m_conf.msaa == msaa)
                selected_msaa = idx;
            m_multi_sample_types.push_back(msaa);
        }
    }
    catch (std::exception &e) {
        xlog::error("Cannot check available MSAA modes: %s", e.what());
    }
    m_msaa_combo.SetCurSel(selected_msaa);
}

void OptionsDlg::UpdateAnisotropyCheckbox()
{
    bool anisotropy_supported = false;
    try {
        anisotropy_supported = m_video_info.has_anisotropy_support(m_conf.selected_video_card);
    }
    catch (std::exception &e) {
        xlog::error("Cannot check anisotropy support: %s", e.what());
    }
    if (anisotropy_supported) {
        GetDlgItem(IDC_ANISOTROPIC_CHECK).EnableWindow(TRUE);
        CheckDlgButton(IDC_ANISOTROPIC_CHECK, m_conf.anisotropic_filtering ? BST_CHECKED : BST_UNCHECKED);
    }
    else
        GetDlgItem(IDC_ANISOTROPIC_CHECK).EnableWindow(FALSE);
}

void OptionsDlg::InitToolTip()
{
    m_tool_tip.Create(*this);

    m_tool_tip.AddTool(GetDlgItem(IDC_EXE_PATH_EDIT), "Path to RF.exe file in Red Faction directory");

    // Display
    m_tool_tip.AddTool(GetDlgItem(IDC_RESOLUTIONS_COMBO), "Please select resolution from provided dropdown list - custom resolution is supposed to work in Windowed/Borderless mode only");
    m_tool_tip.AddTool(GetDlgItem(IDC_VSYNC_CHECK), "Enable vertical synchronization (should limit FPS to monitor refresh rate - usually 60)");
    m_tool_tip.AddTool(GetDlgItem(IDC_MAX_FPS_EDIT), "FPS limit - high FPS can trigger minor bugs in game");

    // Graphics
    m_tool_tip.AddTool(GetDlgItem(IDC_FAST_ANIMS_CHECK), "Reduce animation smoothness for far models");
    m_tool_tip.AddTool(GetDlgItem(IDC_DISABLE_LOD_CHECK), "Use more detailed LOD models for objects in the distance");
    m_tool_tip.AddTool(GetDlgItem(IDC_ANISOTROPIC_CHECK), "Improve far textures quality");
    m_tool_tip.AddTool(GetDlgItem(IDC_HIGH_SCANNER_RES_CHECK), "Increase scanner resolution (used by Rail Gun, Rocket Launcher and Fusion Launcher)");
    m_tool_tip.AddTool(GetDlgItem(IDC_HIGH_MON_RES_CHECK), "Increase monitors and mirrors resolution");
    m_tool_tip.AddTool(GetDlgItem(IDC_TRUE_COLOR_TEXTURES_CHECK), "Increase texture color depth - especially visible for lightmaps and shadows");

    // Audio
    m_tool_tip.AddTool(GetDlgItem(IDC_LEVEL_SOUNDS_CHECK), "Enable/disable Play Sound and Ambient Sound objects in level. You can also specify volume multiplier by using levelsounds command in game.");
    m_tool_tip.AddTool(GetDlgItem(IDC_EAX_SOUND_CHECK), "Enable/disable 3D sound and EAX extension if supported");

    // Multiplayer
    m_tool_tip.AddTool(GetDlgItem(IDC_TRACKER_EDIT), "Hostname of tracker used to find avaliable Multiplayer servers");
    m_tool_tip.AddTool(GetDlgItem(IDC_RATE_EDIT), "Internet connection speed in bytes/s (default value - 200000 - should work fine for all modern setups)");
    m_tool_tip.AddTool(GetDlgItem(IDC_FORCE_PORT_CHECK), "If not checked automatic port is used");

    // Input
    m_tool_tip.AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");
    m_tool_tip.AddTool(GetDlgItem(IDC_LINEAR_PITCH_CHECK), "Stop mouse movement from slowing down when looking up and down");

    // Interface
    m_tool_tip.AddTool(GetDlgItem(IDC_BIG_HUD_CHECK), "Make HUD bigger in the game");
    m_tool_tip.AddTool(GetDlgItem(IDC_SCOREBOARD_ANIM_CHECK), "Scoreboard open/close animations");
    m_tool_tip.AddTool(GetDlgItem(IDC_FPS_COUNTER_CHECK), "Enable FPS counter in right-top corner of the screen");

    // Misc
    m_tool_tip.AddTool(GetDlgItem(IDC_FAST_START_CHECK), "Skip game intro videos and go straight to Main Menu");
    m_tool_tip.AddTool(GetDlgItem(IDC_ALLOW_OVERWRITE_GAME_CHECK), "Enable this if you want to modify game content by putting mods into user_maps folder. Can have side effect of level packfiles modyfing common textures/sounds.");
    m_tool_tip.AddTool(GetDlgItem(IDC_KEEP_LAUNCHER_OPEN_CHECK), "Keep launcher window open after game or editor launch");
    m_tool_tip.AddTool(GetDlgItem(IDC_AUTOSAVE_CHECK), "Automatically save the game after a level transition");
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
    switch (id) {
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

LRESULT OptionsDlg::OnNotify([[ maybe_unused ]] WPARAM wparam, LPARAM lparam)
{
    auto& nmhdr = *reinterpret_cast<LPNMHDR>(lparam);
    switch (nmhdr.code) {
    case CBN_SELCHANGE:
        if (nmhdr.idFrom == IDC_ADAPTER_COMBO) {
            OnAdapterChange();
        }
        else if (nmhdr.idFrom == IDC_RESOLUTIONS_COMBO) {
            OnResolutionChange();
        }
        else if (nmhdr.idFrom == IDC_WND_MODE_COMBO) {
            OnWindowModeChange();
        }
        else if (nmhdr.idFrom == IDC_COLOR_DEPTH_COMBO) {
            OnColorDepthChange();
        }
        break;
    }
    return 0;
}

void OptionsDlg::OnBnClickedOk()
{
    m_conf.game_executable_path = GetDlgItemTextA(IDC_EXE_PATH_EDIT).GetString();

    // Display
    m_conf.selected_video_card = m_adapter_combo.GetCurSel();

    CString resolution_str = GetDlgItemTextA(IDC_RESOLUTIONS_COMBO);
    char *ptr = const_cast<char*>(resolution_str.c_str());
    const char *width_str = strtok(ptr, "x");
    const char *height_str = strtok(nullptr, "x");
    if (width_str && height_str) {
        m_conf.res_width = atoi(width_str);
        m_conf.res_height = atoi(height_str);
    }

    m_conf.res_bpp = m_color_depth_combo.GetWindowTextA() == "32 bit" ? 32 : 16;
    m_conf.res_backbuffer_format = m_conf.res_bpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    m_conf.wnd_mode = static_cast<GameConfig::WndMode>(m_wnd_mode_combo.GetCurSel());
    m_conf.vsync = (IsDlgButtonChecked(IDC_VSYNC_CHECK) == BST_CHECKED);
    m_conf.geometry_cache_size = GetDlgItemInt(IDC_RENDERING_CACHE_EDIT, false);
    m_conf.max_fps = GetDlgItemInt(IDC_MAX_FPS_EDIT, false);

    // Graphics
    m_conf.fast_anims = (IsDlgButtonChecked(IDC_FAST_ANIMS_CHECK) == BST_CHECKED);
    m_conf.msaa = m_multi_sample_types[m_msaa_combo.GetCurSel()];
    m_conf.anisotropic_filtering = (IsDlgButtonChecked(IDC_ANISOTROPIC_CHECK) == BST_CHECKED);
    m_conf.disable_lod_models = (IsDlgButtonChecked(IDC_DISABLE_LOD_CHECK) == BST_CHECKED);
    m_conf.high_scanner_res = (IsDlgButtonChecked(IDC_HIGH_SCANNER_RES_CHECK) == BST_CHECKED);
    m_conf.high_monitor_res = (IsDlgButtonChecked(IDC_HIGH_MON_RES_CHECK) == BST_CHECKED);
    m_conf.true_color_textures = (IsDlgButtonChecked(IDC_TRUE_COLOR_TEXTURES_CHECK) == BST_CHECKED);
    m_conf.mesh_static_lighting = (IsDlgButtonChecked(IDC_MESH_STATIC_LIGHTING_CHECK) == BST_CHECKED);

    // Audio
    m_conf.eax_sound = (IsDlgButtonChecked(IDC_EAX_SOUND_CHECK) == BST_CHECKED);
    if (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) != BST_INDETERMINATE)
        m_conf.level_sound_volume = (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) == BST_CHECKED ? 1.0f : 0.0f);

    // Multiplayer
    m_conf.tracker = GetDlgItemTextA(IDC_TRACKER_EDIT).c_str();
    bool force_port = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    m_conf.force_port = force_port ? GetDlgItemInt(IDC_PORT_EDIT, false) : 0;
    m_conf.update_rate = GetDlgItemInt(IDC_RATE_EDIT, false);

    // Input
    m_conf.direct_input = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
    m_conf.linear_pitch = (IsDlgButtonChecked(IDC_LINEAR_PITCH_CHECK) == BST_CHECKED);
    m_conf.swap_assault_rifle_controls = (IsDlgButtonChecked(IDC_SWAP_ASSAULT_RIFLE_CONTROLS) == BST_CHECKED);
    m_conf.swap_grenade_controls = (IsDlgButtonChecked(IDC_SWAP_GRENADE_CONTROLS) == BST_CHECKED);

    // Interface
    m_conf.big_hud = (IsDlgButtonChecked(IDC_BIG_HUD_CHECK) == BST_CHECKED);
    m_conf.scoreboard_anim = (IsDlgButtonChecked(IDC_SCOREBOARD_ANIM_CHECK) == BST_CHECKED);
    m_conf.language = m_lang_combo.GetCurSel() - 1;
    m_conf.fps_counter = (IsDlgButtonChecked(IDC_FPS_COUNTER_CHECK) == BST_CHECKED);

    // Misc
    m_conf.fast_start = (IsDlgButtonChecked(IDC_FAST_START_CHECK) == BST_CHECKED);
    m_conf.allow_overwrite_game_files = (IsDlgButtonChecked(IDC_ALLOW_OVERWRITE_GAME_CHECK) == BST_CHECKED);
    m_conf.keep_launcher_open = (IsDlgButtonChecked(IDC_KEEP_LAUNCHER_OPEN_CHECK) == BST_CHECKED);
    m_conf.reduced_speed_in_background = (IsDlgButtonChecked(IDC_REDUCED_SPEED_IN_BG_CHECK) == BST_CHECKED);
    m_conf.player_join_beep = (IsDlgButtonChecked(IDC_PLAYER_JOIN_BEEP_CHECK) == BST_CHECKED);
    m_conf.autosave = (IsDlgButtonChecked(IDC_AUTOSAVE_CHECK) == BST_CHECKED);

    try {
        m_conf.save();
    }
    catch (std::exception &e) {
        MessageBoxA(e.what(), nullptr, MB_ICONERROR | MB_OK);
    }
}

void OptionsDlg::OnBnClickedExeBrowse()
{
    LPCTSTR filter = "Executable Files (*.exe)|*.exe|All Files (*.*)|*.*||";
    auto exe_path = GetDlgItemTextA(IDC_EXE_PATH_EDIT);
    CFileDialog dlg(TRUE, ".exe", exe_path, OFN_HIDEREADONLY, filter);
    dlg.SetTitle("Select game executable (RF.exe)");

    if (dlg.DoModal(*this) == IDOK)
        SetDlgItemText(IDC_EXE_PATH_EDIT, dlg.GetPathName());
}

void OptionsDlg::OnBnClickedResetTrackerBtn()
{
    SetDlgItemTextA(IDC_TRACKER_EDIT, GameConfig::default_rf_tracker);
}

void OptionsDlg::OnForcePortClick()
{
    bool force_port = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    GetDlgItem(IDC_PORT_EDIT).EnableWindow(force_port);
}

void OptionsDlg::OnAdapterChange()
{
    m_conf.selected_video_card = m_adapter_combo.GetCurSel();
    UpdateResolutionCombo();
    UpdateColorDepthCombo();
    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
}

void OptionsDlg::OnResolutionChange()
{
    // empty
}

void OptionsDlg::OnColorDepthChange()
{
    m_conf.res_bpp = m_color_depth_combo.GetWindowTextA() == "32 bit" ? 32 : 16;
    m_conf.res_backbuffer_format = m_conf.res_bpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    UpdateResolutionCombo();
    UpdateMsaaCombo();
}

void OptionsDlg::OnWindowModeChange()
{
    m_conf.wnd_mode = static_cast<GameConfig::WndMode>(m_wnd_mode_combo.GetCurSel());
    UpdateMsaaCombo();
}

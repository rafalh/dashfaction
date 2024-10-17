#include <wxx_wincore.h>
#include "OptionsGraphicsDlg.h"
#include "LauncherApp.h"
#include <format>
#include <xlog/xlog.h>
#include <wxx_dialog.h>
#include <wxx_commondlg.h>

OptionsGraphicsDlg::OptionsGraphicsDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_GRAPHICS), m_conf(conf), m_video_info(nullptr)
{
}

BOOL OptionsGraphicsDlg::OnInitDialog()
{
    // Attach controls
    AttachItem(IDC_MSAA_COMBO, m_msaa_combo);
    AttachItem(IDC_CLAMP_COMBO, m_clamp_combo);

    m_clamp_combo.AddString("Automatic");
    m_clamp_combo.AddString("Classic");
    m_clamp_combo.AddString("Full");
    m_clamp_combo.SetCurSel(static_cast<int>(m_conf.clamp_mode));

    // Graphics
    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, m_conf.fast_anims ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_DISABLE_LOD_CHECK, m_conf.disable_lod_models ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, m_conf.high_scanner_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_MON_RES_CHECK, m_conf.high_monitor_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, m_conf.true_color_textures ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_MESH_STATIC_LIGHTING_CHECK, m_conf.mesh_static_lighting ? BST_CHECKED : BST_UNCHECKED);

    InitToolTip();

    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, m_conf.fast_anims ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_DISABLE_LOD_CHECK, m_conf.disable_lod_models ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, m_conf.high_scanner_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_MON_RES_CHECK, m_conf.high_monitor_res ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, m_conf.true_color_textures ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_MESH_STATIC_LIGHTING_CHECK, m_conf.mesh_static_lighting ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}

void OptionsGraphicsDlg::UpdateMsaaCombo()
{
    m_msaa_combo.ResetContent();
    m_msaa_combo.AddString("Disabled");

    if (!m_video_info) {
        return;
    }

    int selected_msaa = 0;
    m_multi_sample_types.push_back(0);
    try {
        BOOL windowed = m_conf.wnd_mode != GameConfig::FULLSCREEN;
        auto multi_sample_types = m_video_info->get_multisample_types(m_conf.selected_video_card, m_conf.res_backbuffer_format, windowed);
        for (auto msaa : multi_sample_types) {
            auto s = std::format("MSAAx{}", msaa);
            int idx = m_msaa_combo.AddString(s.c_str());
            if (m_conf.msaa == msaa)
                selected_msaa = idx;
            m_multi_sample_types.push_back(msaa);
        }
    }
    catch (std::exception &e) {
        xlog::error("Cannot check available MSAA modes: {}", e.what());
    }
    m_msaa_combo.SetCurSel(selected_msaa);
}

void OptionsGraphicsDlg::UpdateAnisotropyCheckbox()
{
    bool anisotropy_supported = false;
    try {
        if (m_video_info) {
            anisotropy_supported = m_video_info->has_anisotropy_support(m_conf.selected_video_card);
        }
    }
    catch (std::exception &e) {
        xlog::error("Cannot check anisotropy support: {}", e.what());
    }
    if (anisotropy_supported) {
        GetDlgItem(IDC_ANISOTROPIC_CHECK).EnableWindow(TRUE);
        CheckDlgButton(IDC_ANISOTROPIC_CHECK, m_conf.anisotropic_filtering ? BST_CHECKED : BST_UNCHECKED);
    }
    else
        GetDlgItem(IDC_ANISOTROPIC_CHECK).EnableWindow(FALSE);
}

void OptionsGraphicsDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_CLAMP_COMBO), "Enable full color range lighting for - Automatic: all community-made levels, Classic: all levels newer than 8/9/24, Full: all levels");
    m_tool_tip.AddTool(GetDlgItem(IDC_FAST_ANIMS_CHECK), "Reduce animation smoothness for far models");
    m_tool_tip.AddTool(GetDlgItem(IDC_DISABLE_LOD_CHECK), "Use more detailed LOD models for objects in the distance");
    m_tool_tip.AddTool(GetDlgItem(IDC_ANISOTROPIC_CHECK), "Improve far textures quality");
    m_tool_tip.AddTool(GetDlgItem(IDC_HIGH_SCANNER_RES_CHECK), "Increase scanner resolution (used by Rail Gun, Rocket Launcher and Fusion Launcher)");
    m_tool_tip.AddTool(GetDlgItem(IDC_HIGH_MON_RES_CHECK), "Increase monitors and mirrors resolution");
    m_tool_tip.AddTool(GetDlgItem(IDC_TRUE_COLOR_TEXTURES_CHECK), "Increase texture color depth - especially visible for lightmaps and shadows");
}

void OptionsGraphicsDlg::OnSave()
{
    m_conf.fast_anims = (IsDlgButtonChecked(IDC_FAST_ANIMS_CHECK) == BST_CHECKED);
    m_conf.msaa = m_multi_sample_types[m_msaa_combo.GetCurSel()];
    m_conf.clamp_mode = static_cast<GameConfig::ClampMode>(m_clamp_combo.GetCurSel());
    m_conf.anisotropic_filtering = (IsDlgButtonChecked(IDC_ANISOTROPIC_CHECK) == BST_CHECKED);
    m_conf.disable_lod_models = (IsDlgButtonChecked(IDC_DISABLE_LOD_CHECK) == BST_CHECKED);
    m_conf.high_scanner_res = (IsDlgButtonChecked(IDC_HIGH_SCANNER_RES_CHECK) == BST_CHECKED);
    m_conf.high_monitor_res = (IsDlgButtonChecked(IDC_HIGH_MON_RES_CHECK) == BST_CHECKED);
    m_conf.true_color_textures = (IsDlgButtonChecked(IDC_TRUE_COLOR_TEXTURES_CHECK) == BST_CHECKED);
    m_conf.mesh_static_lighting = (IsDlgButtonChecked(IDC_MESH_STATIC_LIGHTING_CHECK) == BST_CHECKED);
}

void OptionsGraphicsDlg::OnRendererChange()
{
    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
}

void OptionsGraphicsDlg::OnAdapterChange()
{
    UpdateMsaaCombo();
    UpdateAnisotropyCheckbox();
}

void OptionsGraphicsDlg::OnColorDepthChange()
{
    UpdateMsaaCombo();
}

void OptionsGraphicsDlg::OnWindowModeChange()
{
    UpdateMsaaCombo();
}

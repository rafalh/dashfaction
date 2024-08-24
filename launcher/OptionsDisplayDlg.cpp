#include <wxx_wincore.h>
#include "OptionsDisplayDlg.h"
#include "OptionsGraphicsDlg.h"
#include "LauncherApp.h"
#include <xlog/xlog.h>
#include <wxx_dialog.h>
#include <wxx_commondlg.h>

OptionsDisplayDlg::OptionsDisplayDlg(GameConfig& conf, VideoDeviceInfoProvider& video_info, OptionsGraphicsDlg& graphics_dlg)
	: CDialog(IDD_OPTIONS_DISPLAY), m_conf(conf), m_video_info(video_info), m_graphics_dlg(graphics_dlg)
{
}

BOOL OptionsDisplayDlg::OnInitDialog()
{
    // Attach controls
    AttachItem(IDC_RENDERER_COMBO, m_renderer_combo);
    AttachItem(IDC_ADAPTER_COMBO, m_adapter_combo);
    AttachItem(IDC_RESOLUTIONS_COMBO, m_res_combo);
    AttachItem(IDC_COLOR_DEPTH_COMBO, m_color_depth_combo);
    AttachItem(IDC_WND_MODE_COMBO, m_wnd_mode_combo);

    // Populate combo boxes with static content
    m_renderer_combo.AddString("Direct3D 8");
    m_renderer_combo.AddString("Direct3D 9 (recommended)");
    m_renderer_combo.AddString("Direct3D 11 (experimental)");

    m_wnd_mode_combo.AddString("Exclusive Fullscreen");
    m_wnd_mode_combo.AddString("Windowed");
    m_wnd_mode_combo.AddString("Borderless Window");

    // Display
    m_renderer_combo.SetCurSel(static_cast<int>(m_conf.renderer.value()));
    UpdateAdapterCombo();
    UpdateColorDepthCombo(); // should be before resolution
    UpdateResolutionCombo();
    m_wnd_mode_combo.SetCurSel(static_cast<int>(m_conf.wnd_mode));
    CheckDlgButton(IDC_VSYNC_CHECK, m_conf.vsync ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_RENDERING_CACHE_EDIT, m_conf.geometry_cache_size, false);
    SetDlgItemInt(IDC_MAX_FPS_EDIT, m_conf.max_fps, false);

    InitToolTip();

    return TRUE;
}

void OptionsDisplayDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_RENDERER_COMBO), "Graphics API used for rendering");
    m_tool_tip.AddTool(GetDlgItem(IDC_ADAPTER_COMBO), "Graphics card/adapter used for rendering");
    m_tool_tip.AddTool(GetDlgItem(IDC_RENDERING_CACHE_EDIT), "RAM allocated for level geometry rendering, max 32 MB");
    m_tool_tip.AddTool(GetDlgItem(IDC_RESOLUTIONS_COMBO), "Please select resolution from provided dropdown list - custom resolution is supposed to work in Windowed/Borderless mode only");
    m_tool_tip.AddTool(GetDlgItem(IDC_VSYNC_CHECK), "Enable vertical synchronization (should limit FPS to monitor refresh rate - usually 60)");
    m_tool_tip.AddTool(GetDlgItem(IDC_MAX_FPS_EDIT), "FPS limit - maximal value is 240 - high FPS can trigger minor bugs in game");
}

void OptionsDisplayDlg::UpdateAdapterCombo()
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

void OptionsDisplayDlg::UpdateResolutionCombo()
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

void OptionsDisplayDlg::UpdateColorDepthCombo()
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

BOOL OptionsDisplayDlg::OnCommand(WPARAM wparam, [[ maybe_unused ]] LPARAM lparam)
{
    if (HIWORD(wparam) == CBN_SELCHANGE) {
        switch (LOWORD(wparam)) {
            case IDC_ADAPTER_COMBO:
                OnAdapterChange();
                break;
            case IDC_WND_MODE_COMBO:
                OnWindowModeChange();
                break;
            case IDC_COLOR_DEPTH_COMBO:
                OnColorDepthChange();
                break;
        }
    }
    return 0;
}

void OptionsDisplayDlg::OnSave()
{
    m_conf.renderer = static_cast<GameConfig::Renderer>(m_renderer_combo.GetCurSel());
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
}

void OptionsDisplayDlg::OnAdapterChange()
{
    m_conf.selected_video_card = m_adapter_combo.GetCurSel();
    UpdateResolutionCombo();
    UpdateColorDepthCombo();
    m_graphics_dlg.OnAdapterChange();
}

void OptionsDisplayDlg::OnColorDepthChange()
{
    m_conf.res_bpp = m_color_depth_combo.GetWindowTextA() == "32 bit" ? 32 : 16;
    m_conf.res_backbuffer_format = m_conf.res_bpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    UpdateResolutionCombo();
    m_graphics_dlg.OnColorDepthChange();
}

void OptionsDisplayDlg::OnWindowModeChange()
{
    m_conf.wnd_mode = static_cast<GameConfig::WndMode>(m_wnd_mode_combo.GetCurSel());
    m_graphics_dlg.OnWindowModeChange();
}

// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include <common/GameConfig.h>


// OptionsDlg dialog

IMPLEMENT_DYNAMIC(OptionsDlg, CDialogEx)

OptionsDlg::OptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_OPTIONS, pParent), m_toolTip(nullptr)
{

}

OptionsDlg::~OptionsDlg()
{
    if (m_toolTip)
    {
        delete m_toolTip;
        m_toolTip = nullptr;
    }
}

BOOL OptionsDlg::OnInitDialog()
{
    try
    {
        m_conf.load();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }

    SetDlgItemTextA(IDC_EXE_PATH_EDIT, m_conf.gameExecutablePath.c_str());

    CComboBox *resCombo = (CComboBox*)GetDlgItem(IDC_RESOLUTIONS_COMBO);
    char buf[256];

    int selectedRes = -1;
    try
    {
        auto resolutions = m_videoInfo.getResolutions(D3DFMT_X8R8G8B8);
        for (const auto &res : resolutions)
        {
            sprintf(buf, "%dx%d", res.width, res.height);
            int pos = resCombo->AddString(buf);
            if (m_conf.resWidth == res.width && m_conf.resHeight == res.height)
                selectedRes = pos;
        }
    }
    catch (std::exception &e)
    {
        // Only 'Disabled' option available. Log error in console.
        printf("Cannot get available screen resolutions: %s", e.what());
    }
    if (selectedRes != -1)
        resCombo->SetCurSel(selectedRes);
    else
    {
        char buf[32];
        sprintf(buf, "%dx%d", m_conf.resWidth, m_conf.resHeight);
        resCombo->SetWindowTextA(buf);
    }

    CheckDlgButton(IDC_32BIT_RADIO, m_conf.resBpp == 32 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_16BIT_RADIO, m_conf.resBpp == 16 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FULL_SCREEN_RADIO, m_conf.wndMode == GameConfig::FULLSCREEN ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_WINDOWED_RADIO, m_conf.wndMode == GameConfig::WINDOWED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_STRETCHED_RADIO, m_conf.wndMode == GameConfig::STRETCHED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_VSYNC_CHECK, m_conf.vsync ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, m_conf.fastAnims ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_RENDERING_CACHE_EDIT, m_conf.geometryCacheSize);
    SetDlgItemInt(IDC_MAX_FPS_EDIT, m_conf.maxFps);

    CComboBox *msaaCombo = (CComboBox*)GetDlgItem(IDC_MSAA_COMBO);
    msaaCombo->AddString("Disabled");
    int selectedMsaa = 0;
    m_multiSampleTypes.push_back(0);
    try
    {
        auto multiSampleTypes = m_videoInfo.getMultiSampleTypes(D3DFMT_X8R8G8B8, FALSE);
        for (auto msaa : multiSampleTypes)
        {
            char buf[16];
            sprintf(buf, "MSAAx%u", msaa);
            int idx = msaaCombo->AddString(buf);
            if (m_conf.msaa == msaa)
                selectedMsaa = idx;
            m_multiSampleTypes.push_back(msaa);
        }
    }
    catch (std::exception &e)
    {
        printf("Cannot check available MSAA modes: %s", e.what());
    }
    msaaCombo->SetCurSel(selectedMsaa);

    bool anisotropySupported = false;
    try
    {
        anisotropySupported = m_videoInfo.hasAnisotropySupport();
    }
    catch (std::exception &e)
    {
        printf("Cannot check anisotropy support: %s", e.what());
    }
    if (anisotropySupported)
        CheckDlgButton(IDC_ANISOTROPIC_CHECK, m_conf.anisotropicFiltering ? BST_CHECKED : BST_UNCHECKED);
    else
        GetDlgItem(IDC_ANISOTROPIC_CHECK)->EnableWindow(FALSE);

    CheckDlgButton(IDC_DISABLE_LOD_CHECK, m_conf.disableLodModels ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FPS_COUNTER_CHECK, m_conf.fpsCounter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, m_conf.highScannerRes ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_MON_RES_CHECK, m_conf.highMonitorRes ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, m_conf.trueColorTextures ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemTextA(IDC_TRACKER_EDIT, m_conf.tracker.c_str());
    CheckDlgButton(IDC_FORCE_PORT_CHECK, m_conf.forcePort != 0);
    if (m_conf.forcePort)
        SetDlgItemInt(IDC_PORT_EDIT, m_conf.forcePort);
    else
        GetDlgItem(IDC_PORT_EDIT)->EnableWindow(FALSE);
    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, m_conf.directInput ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_EAX_SOUND_CHECK, m_conf.eaxSound ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_START_CHECK, m_conf.fastStart ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SCOREBOARD_ANIM_CHECK, m_conf.scoreboardAnim);
    if (m_conf.levelSoundVolume == 1.0f)
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, BST_CHECKED);
    else
        CheckDlgButton(IDC_LEVEL_SOUNDS_CHECK, m_conf.levelSoundVolume == 0.0f ? BST_UNCHECKED : BST_INDETERMINATE);
    CheckDlgButton(IDC_ALLOW_OVERWRITE_GAME_CHECK, m_conf.allowOverwriteGameFiles ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_KEEP_LAUNCHER_OPEN_CHECK, m_conf.keepLauncherOpen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_LINEAR_PITCH_CHECK, m_conf.linearPitch ? BST_CHECKED : BST_UNCHECKED);

    InitToolTip();

    return TRUE;
}

void OptionsDlg::InitToolTip()
{
    m_toolTip = new CToolTipCtrl();
    m_toolTip->Create(this);

    m_toolTip->AddTool(GetDlgItem(IDC_RESOLUTIONS_COMBO), "Please select resolution from provided dropdown list - custom resolution is supposed to work in Windowed/Stretched mode only");
    m_toolTip->AddTool(GetDlgItem(IDC_STRETCHED_RADIO), "Full Screen Windowed - reduced performance but faster to switch to other window");
    m_toolTip->AddTool(GetDlgItem(IDC_VSYNC_CHECK), "Enable vertical synchronization (should limit FPS to monitor refresh rate - usually 60)");
    m_toolTip->AddTool(GetDlgItem(IDC_MAX_FPS_EDIT), "FPS limit - maximal value is 150 - high FPS can trigger minor bugs in game");
    m_toolTip->AddTool(GetDlgItem(IDC_FAST_ANIMS_CHECK), "Reduce animation smoothness for far models");
    m_toolTip->AddTool(GetDlgItem(IDC_DISABLE_LOD_CHECK), "Improve details for far models");
    m_toolTip->AddTool(GetDlgItem(IDC_ANISOTROPIC_CHECK), "Improve far textures quality");
    m_toolTip->AddTool(GetDlgItem(IDC_FPS_COUNTER_CHECK), "Enable FPS counter in right-top corner of the screen");
    m_toolTip->AddTool(GetDlgItem(IDC_HIGH_SCANNER_RES_CHECK), "Increase scanner resolution (used by Rail Gun, Rocket Launcher and Fusion Launcher)");
    m_toolTip->AddTool(GetDlgItem(IDC_HIGH_SCANNER_RES_CHECK), "Increase monitors and mirrors resolution");
    m_toolTip->AddTool(GetDlgItem(IDC_TRUE_COLOR_TEXTURES_CHECK), "Increase texture color depth - especially visible for lightmaps and shadows");
    m_toolTip->AddTool(GetDlgItem(IDC_TRACKER_EDIT), "Hostname of tracker used to find avaliable Multiplayer servers");
    m_toolTip->AddTool(GetDlgItem(IDC_FAST_START_CHECK), "Skip game intro videos and go straight to Main Menu");
    m_toolTip->AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");
    m_toolTip->AddTool(GetDlgItem(IDC_FORCE_PORT_CHECK), "If not checked automatic port is used");
    m_toolTip->AddTool(GetDlgItem(IDC_SCOREBOARD_ANIM_CHECK), "Scoreboard open/close animations");
    m_toolTip->AddTool(GetDlgItem(IDC_LEVEL_SOUNDS_CHECK), "Enable/disable Play Sound and Ambient Sound objects in level. You can also specify volume multiplier by using levelsounds command in game.");
    m_toolTip->AddTool(GetDlgItem(IDC_ALLOW_OVERWRITE_GAME_CHECK), "Enable this if you want to modify game content by putting mods into user_maps folder. Can have side effect of level packfiles modyfing common textures/sounds.");
    m_toolTip->AddTool(GetDlgItem(IDC_KEEP_LAUNCHER_OPEN_CHECK), "Keep launcher window open after game or editor launch");
    m_toolTip->AddTool(GetDlgItem(IDC_LINEAR_PITCH_CHECK), "Stop mouse movement from slowing down when looking up and down");

    m_toolTip->Activate(TRUE);
}

BOOL OptionsDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip)
        m_toolTip->RelayEvent(pMsg);

    return CDialogEx::PreTranslateMessage(pMsg);
}

void OptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(OptionsDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &OptionsDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_EXE_BROWSE, &OptionsDlg::OnBnClickedExeBrowse)
    ON_BN_CLICKED(IDC_RESET_TRACKER_BTN, &OptionsDlg::OnBnClickedResetTrackerBtn)
    ON_BN_CLICKED(IDC_FORCE_PORT_CHECK, &OptionsDlg::OnForcePortClick)
END_MESSAGE_MAP()


// OptionsDlg message handlers


void OptionsDlg::OnBnClickedOk()
{
    CString str;

    GetDlgItemTextA(IDC_EXE_PATH_EDIT, str);
    m_conf.gameExecutablePath = str;

    GetDlgItemTextA(IDC_RESOLUTIONS_COMBO, str);
    char *ptr = (char*)(const char *)str;
    const char *widthStr = strtok(ptr, "x");
    const char *heightStr = strtok(nullptr, "x");
    if (widthStr && heightStr)
    {
        m_conf.resWidth = atoi(widthStr);
        m_conf.resHeight = atoi(heightStr);
    }

    m_conf.resBpp = IsDlgButtonChecked(IDC_16BIT_RADIO) == BST_CHECKED ? 16 : 32;
    m_conf.resBackbufferFormat = m_conf.resBpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;

    if (IsDlgButtonChecked(IDC_FULL_SCREEN_RADIO) == BST_CHECKED)
        m_conf.wndMode = GameConfig::FULLSCREEN;
    if (IsDlgButtonChecked(IDC_WINDOWED_RADIO) == BST_CHECKED)
        m_conf.wndMode = GameConfig::WINDOWED;
    if (IsDlgButtonChecked(IDC_STRETCHED_RADIO) == BST_CHECKED)
        m_conf.wndMode = GameConfig::STRETCHED;

    m_conf.vsync = (IsDlgButtonChecked(IDC_VSYNC_CHECK) == BST_CHECKED);
    m_conf.fastAnims = (IsDlgButtonChecked(IDC_FAST_ANIMS_CHECK) == BST_CHECKED);
    m_conf.geometryCacheSize = GetDlgItemInt(IDC_RENDERING_CACHE_EDIT);
    m_conf.maxFps = GetDlgItemInt(IDC_MAX_FPS_EDIT);

    CComboBox *msaaCombo = (CComboBox*)GetDlgItem(IDC_MSAA_COMBO);
    m_conf.msaa = m_multiSampleTypes[msaaCombo->GetCurSel()];

    m_conf.anisotropicFiltering = (IsDlgButtonChecked(IDC_ANISOTROPIC_CHECK) == BST_CHECKED);
    m_conf.disableLodModels = (IsDlgButtonChecked(IDC_DISABLE_LOD_CHECK) == BST_CHECKED);
    m_conf.fpsCounter = (IsDlgButtonChecked(IDC_FPS_COUNTER_CHECK) == BST_CHECKED);
    m_conf.highScannerRes = (IsDlgButtonChecked(IDC_HIGH_SCANNER_RES_CHECK) == BST_CHECKED);
    m_conf.highMonitorRes = (IsDlgButtonChecked(IDC_HIGH_MON_RES_CHECK) == BST_CHECKED);
    m_conf.trueColorTextures = (IsDlgButtonChecked(IDC_TRUE_COLOR_TEXTURES_CHECK) == BST_CHECKED);

    GetDlgItemTextA(IDC_TRACKER_EDIT, str);
    m_conf.tracker = str;
    bool forcePort = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    m_conf.forcePort = forcePort ? GetDlgItemInt(IDC_PORT_EDIT) : 0;
    m_conf.directInput = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
    m_conf.eaxSound = (IsDlgButtonChecked(IDC_EAX_SOUND_CHECK) == BST_CHECKED);
    m_conf.fastStart = (IsDlgButtonChecked(IDC_FAST_START_CHECK) == BST_CHECKED);
    m_conf.scoreboardAnim = (IsDlgButtonChecked(IDC_SCOREBOARD_ANIM_CHECK) == BST_CHECKED);
    if (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) != BST_INDETERMINATE)
        m_conf.levelSoundVolume = (IsDlgButtonChecked(IDC_LEVEL_SOUNDS_CHECK) == BST_CHECKED ? 1.0f : 0.0f);
    m_conf.allowOverwriteGameFiles = (IsDlgButtonChecked(IDC_ALLOW_OVERWRITE_GAME_CHECK) == BST_CHECKED);
    m_conf.keepLauncherOpen = (IsDlgButtonChecked(IDC_KEEP_LAUNCHER_OPEN_CHECK) == BST_CHECKED);
    m_conf.linearPitch = (IsDlgButtonChecked(IDC_LINEAR_PITCH_CHECK) == BST_CHECKED);

    try
    {
        m_conf.save();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }

    CDialogEx::OnOK();
}


void OptionsDlg::OnBnClickedExeBrowse()
{
    LPCTSTR filter = "Executable Files (*.exe)|*.exe||";
    CFileDialog dlg(TRUE, ".exe", "RF.exe", 0, filter, this);
    OPENFILENAME &ofn = dlg.GetOFN();
    ofn.lpstrTitle = "Select game executable (RF.exe)";

    if (dlg.DoModal() == IDOK)
        SetDlgItemTextA(IDC_EXE_PATH_EDIT, dlg.GetPathName());
}


void OptionsDlg::OnBnClickedResetTrackerBtn()
{
    SetDlgItemTextA(IDC_TRACKER_EDIT, DEFAULT_RF_TRACKER);
}

void OptionsDlg::OnForcePortClick()
{
    bool forcePort = IsDlgButtonChecked(IDC_FORCE_PORT_CHECK) == BST_CHECKED;
    GetDlgItem(IDC_PORT_EDIT)->EnableWindow(forcePort);
}

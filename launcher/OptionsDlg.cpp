// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include "GameConfig.h"


// OptionsDlg dialog

IMPLEMENT_DYNAMIC(OptionsDlg, CDialogEx)

OptionsDlg::OptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_OPTIONS, pParent)
{
    
}

OptionsDlg::~OptionsDlg()
{
    if (m_toolTip)
    {
        m_toolTip->DestroyToolTipCtrl();
        delete m_toolTip;
        m_toolTip = nullptr;
    }
}

BOOL OptionsDlg::OnInitDialog()
{
    GameConfig conf;
    try
    {
        conf.load();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }

    SetDlgItemTextA(IDC_EXE_PATH_EDIT, conf.gameExecutablePath.c_str());

    CComboBox *resCombo = (CComboBox*)GetDlgItem(IDC_RESOLUTIONS_COMBO);
    char buf[256];

    auto resolutions = m_videoInfo.getResolutions(D3DFMT_X8R8G8B8);
    int selectedRes = -1;
    for (const auto &res : resolutions)
    {
        sprintf(buf, "%dx%d", res.width, res.height);
        int pos = resCombo->AddString(buf);
        if (conf.resWidth == res.width && conf.resHeight == res.height)
            selectedRes = pos;
    }
    if (selectedRes != -1)
        resCombo->SetCurSel(selectedRes);
    else
    {
        char buf[32];
        sprintf(buf, "%dx%d", conf.resWidth, conf.resHeight);
        resCombo->SetWindowTextA(buf);
    }

    CheckDlgButton(IDC_32BIT_RADIO, conf.resBpp == 32 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_16BIT_RADIO, conf.resBpp == 16 ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FULL_SCREEN_RADIO, conf.wndMode == GameConfig::FULLSCREEN ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_WINDOWED_RADIO, conf.wndMode == GameConfig::WINDOWED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_STRETCHED_RADIO, conf.wndMode == GameConfig::STRETCHED ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_VSYNC_CHECK, conf.vsync ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_ANIMS_CHECK, conf.fastAnims ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_RENDERING_CACHE_EDIT, conf.geometryCacheSize);
    SetDlgItemInt(IDC_MAX_FPS_EDIT, conf.maxFps);

    CComboBox *msaaCombo = (CComboBox*)GetDlgItem(IDC_MSAA_COMBO);
    auto multiSampleTypes = m_videoInfo.getMultiSampleTypes(D3DFMT_X8R8G8B8, FALSE);
    msaaCombo->AddString("Disabled");
    int selectedMsaa = 0;
    m_multiSampleTypes.push_back(0);
    for (auto msaa : multiSampleTypes)
    {
        char buf[16];
        sprintf(buf, "MSAAx%u", msaa);
        int idx = msaaCombo->AddString(buf);
        if (conf.msaa == msaa)
            selectedMsaa = idx;
        m_multiSampleTypes.push_back(msaa);
    }
    msaaCombo->SetCurSel(selectedMsaa);

    if (m_videoInfo.hasAnisotropySupport())
        CheckDlgButton(IDC_ANISOTROPIC_CHECK, conf.anisotropicFiltering ? BST_CHECKED : BST_UNCHECKED);
    else
        GetDlgItem(IDC_ANISOTROPIC_CHECK)->EnableWindow(FALSE);
    CheckDlgButton(IDC_DISABLE_LOD_CHECK, conf.disableLodModels ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FPS_COUNTER_CHECK, conf.fpsCounter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_HIGH_SCANNER_RES_CHECK, conf.highScannerRes ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRUE_COLOR_TEXTURES_CHECK, conf.trueColorTextures ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemTextA(IDC_TRACKER_EDIT, conf.tracker.c_str());
    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, conf.directInput ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_EAX_SOUND_CHECK, conf.eaxSound ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_FAST_START_CHECK, conf.fastStart ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_ALLOW_OVERWRITE_GAME_CHECK, conf.allowOverwriteGameFiles ? BST_CHECKED : BST_UNCHECKED);

    InitToolTip();

    return TRUE;
}

void OptionsDlg::InitToolTip()
{
    //create a tool tip control
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
    m_toolTip->AddTool(GetDlgItem(IDC_TRUE_COLOR_TEXTURES_CHECK), "Increase texture color depth - especially visible for lightmaps and shadows");
    m_toolTip->AddTool(GetDlgItem(IDC_TRACKER_EDIT), "Hostname of tracker used to find avaliable Multiplayer servers");
    m_toolTip->AddTool(GetDlgItem(IDC_FAST_START_CHECK), "Skip game intro videos and go straight to Main Menu");
    m_toolTip->AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");

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
END_MESSAGE_MAP()


// OptionsDlg message handlers


void OptionsDlg::OnBnClickedOk()
{
    GameConfig conf;
    CString str;

    GetDlgItemTextA(IDC_EXE_PATH_EDIT, str);
    conf.gameExecutablePath = str;

    GetDlgItemTextA(IDC_RESOLUTIONS_COMBO, str);
    char *ptr = (char*)(const char *)str;
    const char *widthStr = strtok(ptr, "x");
    const char *heightStr = strtok(nullptr, "x");
    if (widthStr && heightStr)
    {
        conf.resWidth = atoi(widthStr);
        conf.resHeight = atoi(heightStr);
    }
    
    conf.resBpp = IsDlgButtonChecked(IDC_16BIT_RADIO) == BST_CHECKED ? 16 : 32;
    conf.resBackbufferFormat = conf.resBpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;

    if (IsDlgButtonChecked(IDC_FULL_SCREEN_RADIO) == BST_CHECKED)
        conf.wndMode = GameConfig::FULLSCREEN;
    if (IsDlgButtonChecked(IDC_WINDOWED_RADIO) == BST_CHECKED)
        conf.wndMode = GameConfig::WINDOWED;
    if (IsDlgButtonChecked(IDC_STRETCHED_RADIO) == BST_CHECKED)
        conf.wndMode = GameConfig::STRETCHED;

    conf.vsync = (IsDlgButtonChecked(IDC_VSYNC_CHECK) == BST_CHECKED);
    conf.fastAnims = (IsDlgButtonChecked(IDC_FAST_ANIMS_CHECK) == BST_CHECKED);
    conf.geometryCacheSize = GetDlgItemInt(IDC_RENDERING_CACHE_EDIT);
    conf.maxFps = GetDlgItemInt(IDC_MAX_FPS_EDIT);

    CComboBox *msaaCombo = (CComboBox*)GetDlgItem(IDC_MSAA_COMBO);
    conf.msaa = m_multiSampleTypes[msaaCombo->GetCurSel()];

    conf.anisotropicFiltering = (IsDlgButtonChecked(IDC_ANISOTROPIC_CHECK) == BST_CHECKED);
    conf.disableLodModels = (IsDlgButtonChecked(IDC_DISABLE_LOD_CHECK) == BST_CHECKED);
    conf.fpsCounter = (IsDlgButtonChecked(IDC_FPS_COUNTER_CHECK) == BST_CHECKED);
    conf.highScannerRes = (IsDlgButtonChecked(IDC_HIGH_SCANNER_RES_CHECK) == BST_CHECKED);
    conf.trueColorTextures = (IsDlgButtonChecked(IDC_TRUE_COLOR_TEXTURES_CHECK) == BST_CHECKED);

    GetDlgItemTextA(IDC_TRACKER_EDIT, str);
    conf.tracker = str;
    conf.directInput = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
    conf.eaxSound = (IsDlgButtonChecked(IDC_EAX_SOUND_CHECK) == BST_CHECKED);
    conf.fastStart = (IsDlgButtonChecked(IDC_FAST_START_CHECK) == BST_CHECKED);
    conf.allowOverwriteGameFiles = (IsDlgButtonChecked(IDC_ALLOW_OVERWRITE_GAME_CHECK) == BST_CHECKED);

    try
    {
        conf.save();
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

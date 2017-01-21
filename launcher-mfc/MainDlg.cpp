
// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include "GameStarter.h"
#include "GameConfig.h"
#include "UpdateChecker.h"
#include "version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// MainDlg dialog

#define WM_UPDATE_CHECK (WM_USER+10)

MainDlg::MainDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MAIN, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_ICON);
}

void MainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_UPDATE_CHECK, OnUpdateCheck)
    ON_BN_CLICKED(IDC_OPTIONS_BTN, &MainDlg::OnBnClickedOptionsBtn)
    ON_BN_CLICKED(IDOK, &MainDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_EDITOR_BTN, &MainDlg::OnBnClickedEditorBtn)
END_MESSAGE_MAP()


// MainDlg message handlers

BOOL MainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    SetWindowTextA(PRODUCT_NAME_VERSION);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // Set header bitmap
    CStatic *picture = (CStatic *)GetDlgItem(IDC_HEADER_PIC);
    HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, 0);
    picture->SetBitmap(hbm);

#ifdef NDEBUG
    m_pUpdateChecker = new UpdateChecker(m_hWnd);
    m_pUpdateChecker->checkAsync([=]() {
        PostMessageA(WM_UPDATE_CHECK, 0, 0);
    });
#else
    m_pUpdateChecker = nullptr;
#endif

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void MainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR MainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT MainDlg::OnUpdateCheck(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CStatic *updateStatus = (CStatic*)GetDlgItem(IDC_UPDATE_STATUS);

    if (m_pUpdateChecker->hasError())
    {
        MessageBoxA(m_pUpdateChecker->getError().c_str(), NULL, MB_OK | MB_ICONERROR);
        updateStatus->SetWindowTextA("Failed to check for update");
        return 0;
    }

    if (!m_pUpdateChecker->isNewVersionAvailable())
        updateStatus->SetWindowTextA("No update is available.");
    else
    {
        updateStatus->SetWindowTextA("New version available!");
        int iResult = MessageBoxA(m_pUpdateChecker->getMessage().c_str(), 
            "DashFaction update is available!", MB_OKCANCEL | MB_ICONEXCLAMATION);
        if (iResult == IDOK)
            ShellExecuteA(m_hWnd, "open", m_pUpdateChecker->getUrl().c_str(), NULL, NULL, SW_SHOW);
    }

    return 0;
}

void MainDlg::OnBnClickedOptionsBtn()
{
    OptionsDlg dlg;
    INT_PTR nResponse = dlg.DoModal();
}

void MainDlg::OnBnClickedOk()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    if (static_cast<LauncherApp*>(AfxGetApp())->LaunchGame(m_hWnd))
        CDialogEx::OnOK();
}

void MainDlg::OnBnClickedEditorBtn()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    if (static_cast<LauncherApp*>(AfxGetApp())->LaunchEditor(m_hWnd))
        EndDialog(0);
}


// MainDlg.h : header file
//

#pragma once


class UpdateChecker;

// MainDlg dialog
class MainDlg : public CDialogEx
{
// Construction
public:
    MainDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    void RefreshModSelector();
    CString GetSelectedMod();
    void AfterLaunch();

// Implementation
protected:
    HICON m_hIcon;
    UpdateChecker *m_pUpdateChecker;
    CToolTipCtrl m_ToolTip;

	// Generated message map functions
    virtual BOOL OnInitDialog();
    BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnUpdateCheck(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOptionsBtn();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedEditorBtn();
    afx_msg void OnBnClickedSupportBtn();
};

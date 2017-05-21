#pragma once

#include "VideoDeviceInfoProvider.h"
#include "GameConfig.h"

// OptionsDlg dialog

class OptionsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OptionsDlg)

public:
	OptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~OptionsDlg();
    BOOL OnInitDialog();
    void InitToolTip();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPTIONS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedExeBrowse();
    afx_msg void OnBnClickedResetTrackerBtn();
    afx_msg void OnForcePortClick();

private:
    VideoDeviceInfoProvider m_videoInfo;
    std::vector<unsigned> m_multiSampleTypes;
    CToolTipCtrl *m_toolTip;
    GameConfig m_conf;
};

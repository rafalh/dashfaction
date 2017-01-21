#pragma once

#include "VideoDeviceInfoProvider.h"

// OptionsDlg dialog

class OptionsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OptionsDlg)

public:
	OptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~OptionsDlg();
    BOOL OnInitDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPTIONS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedExeBrowse();
    afx_msg void OnBnClickedResetTrackerBtn();

private:
    VideoDeviceInfoProvider m_videoInfo;
    std::vector<unsigned> m_multiSampleTypes;
};

#pragma once

#include <windows.h>
#include <patch_common/MemUtils.h>

struct CWnd;

struct CDataExchange
{
	BOOL m_bSaveAndValidate;
	CWnd* m_pDlgWnd;
	HWND m_hWndLastControl;
	BOOL m_bEditLastControl;
};

inline HWND WndToHandle(CWnd* wnd)
{
    return StructFieldRef<HWND>(wnd, 4 + 0x18);
}

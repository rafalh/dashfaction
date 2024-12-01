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

struct CString
{
    LPTSTR m_pchData;

    void operator=(const char* s)
    {
        AddrCaller{0x0052FBDC}.this_call(this, s);
    }

    operator const char*() const
    {
        return m_pchData;
    }

    bool operator==(const char* s) const
    {
        return std::strcmp(m_pchData, s) == 0;
    }
};

inline HWND WndToHandle(CWnd* wnd)
{
    return struct_field_ref<HWND>(wnd, 4 + 0x18);
}

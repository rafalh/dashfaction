#pragma once

#include "afxwin.h"

class LauncherCommandLineInfo : public CCommandLineInfo
{
public:
    virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
    {
        if (bFlag && (!strcmp(pszParam, "game") || !strcmp(pszParam, "level") || !strcmp(pszParam, "dedicated")))
            m_game = true;
        if (bFlag && !strcmp(pszParam, "editor"))
            m_editor = true;
        if (bFlag && (!strcmp(pszParam, "help") || !strcmp(pszParam, "h")))
            m_help = true;
    }

    bool HasGameFlag() const
    {
        return m_game;
    }

    bool HasEditorFlag() const
    {
        return m_editor;
    }

    bool HasHelpFlag() const
    {
        return m_help;
    }

private:
    bool m_game = false;
    bool m_editor = false;
    bool m_help = false;
};

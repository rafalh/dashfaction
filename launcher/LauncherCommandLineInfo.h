#pragma once

#include <wxx_wincore.h>

class LauncherCommandLineInfo //: public CCommandLineInfo
{
public:
    void Parse()
    {
        auto args = Win32xx::GetCommandLineArgs();
        for (auto& arg : args) {
            if (arg[0] == '-') {
                ParseFlag(arg.c_str() + 1);
            }
        }
    }

    void ParseFlag(const char* flag_name)
    {
        if (!strcmp(flag_name, "game") || !strcmp(flag_name, "level") || !strcmp(flag_name, "dedicated"))
            m_game = true;
        if (!strcmp(flag_name, "editor"))
            m_editor = true;
        if (!strcmp(flag_name, "help") || !strcmp(flag_name, "h"))
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

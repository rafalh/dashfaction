#pragma once

#include <wxx_wincore.h>
#include <string_view>

class LauncherCommandLineInfo
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

    void ParseFlag(std::string_view flag_name)
    {
        if (flag_name == "game" || flag_name == "level" || flag_name == "dedicated")
            m_game = true;
        if (flag_name == "editor")
            m_editor = true;
        if (flag_name == "help" || flag_name == "h")
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

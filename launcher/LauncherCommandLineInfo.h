#pragma once

#include <wxx_wincore.h>
#include <string_view>
#include <string>
#include <optional>

class LauncherCommandLineInfo
{
public:
    void Parse()
    {
        auto args = Win32xx::GetCommandLineArgs();
        for (int i = 0; i < args.size(); ++i) {
            std::string_view arg = args[i].c_str();
            if (arg[0] == '-') {
                if (arg == "-game" || arg == "-level" || arg == "-dedicated") {
                    m_game = true;
                }
                else if (arg == "-editor") {
                    m_editor = true;
                }
                else if (arg == "-help" || arg == "-h") {
                    m_help = true;
                }
                else if (arg == "-exepath") {
                    m_exe_path = {args[++i].c_str()};
                }
            }
        }
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

    std::optional<std::string> GetExePath() const
    {
        return m_exe_path;
    }

private:
    bool m_game = false;
    bool m_editor = false;
    bool m_help = false;
    std::optional<std::string> m_exe_path;
};

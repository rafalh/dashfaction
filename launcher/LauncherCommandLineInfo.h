#pragma once

#include <wxx_wincore.h>
#include <string_view>
#include <string>
#include <optional>
#include <vector>

class LauncherCommandLineInfo
{
public:
    void Parse()
    {
        auto args = Win32xx::GetCommandLineArgs();
        bool has_level_arg = false;
        bool has_dedicated_arg = false;
        for (unsigned i = 1; i < args.size(); ++i) {
            std::string_view arg = args[i].c_str();
            if (arg == "-game") {
                m_game = true;
            }
            else if (arg == "-editor") {
                m_editor = true;
            }
            else if (arg == "-help" || arg == "-h") {
                m_help = true;
            }
            else if (arg == "-exe-path") {
                m_exe_path = {args[++i].c_str()};
            }
            else {
                if (arg == "-level") {
                    has_level_arg = true;
                }
                else if (arg == "-dedicated") {
                    has_dedicated_arg = true;
                }
                m_pass_through_args.emplace_back(arg);
            }
        }
        if (!m_game && !m_editor && (has_level_arg || has_dedicated_arg)) {
            m_game = true;
        }
    }

    [[nodiscard]] bool HasGameFlag() const
    {
        return m_game;
    }

    [[nodiscard]] bool HasEditorFlag() const
    {
        return m_editor;
    }

    [[nodiscard]] bool HasHelpFlag() const
    {
        return m_help;
    }

    [[nodiscard]] std::optional<std::string> GetExePath() const
    {
        return m_exe_path;
    }

    [[nodiscard]] const std::vector<std::string>& GetPassThroughArgs() const
    {
        return m_pass_through_args;
    }

private:
    bool m_game = false;
    bool m_editor = false;
    bool m_help = false;
    std::optional<std::string> m_exe_path;
    std::vector<std::string> m_pass_through_args;
};

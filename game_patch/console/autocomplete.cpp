#include "console.h"
#include "../main.h"
#include "../misc/packfile.h"
#include "../rf/player.h"
#include <patch_common/FunHook.h>
#include <cstring>

void DcShowCmdHelp(rf::ConsoleCommand* cmd)
{
    rf::console_run = 0;
    rf::console_help = 1;
    rf::console_status = 0;
    auto handler = reinterpret_cast<void(__thiscall*)(rf::ConsoleCommand*)>(cmd->func);
    handler(cmd);
}

int DcAutoCompleteGetComponent(int offset, std::string& result)
{
    const char *begin = rf::console_cmd_line + offset, *end = nullptr, *next;
    if (begin[0] == '"') {
        ++begin;
        end = strchr(begin, '"');
        next = end ? strchr(end, ' ') : nullptr;
    }
    else
        end = next = strchr(begin, ' ');

    if (!end)
        end = rf::console_cmd_line + rf::console_cmd_line_len;

    size_t len = end - begin;
    result.assign(begin, len);

    return next ? next + 1 - rf::console_cmd_line : -1;
}

void DcAutoCompletePutComponent(int offset, const std::string& component, bool finished)
{
    bool quote = component.find(' ') != std::string::npos;
    int max_len = std::size(rf::console_cmd_line);
    int len;
    if (quote) {
        len = snprintf(rf::console_cmd_line + offset, max_len - offset, "\"%s\"", component.c_str());
    }
    else {
        len = snprintf(rf::console_cmd_line + offset, max_len - offset, "%s", component.c_str());
    }
    rf::console_cmd_line_len = offset + len;
    if (finished)
        rf::console_cmd_line_len += snprintf(rf::console_cmd_line + rf::console_cmd_line_len, max_len - rf::console_cmd_line_len, " ");
}

template<typename T, typename F>
void DcAutoCompletePrintSuggestions(T& suggestions, F mapping_fun)
{
    for (auto& item : suggestions) {
        rf::ConsolePrintf("%s\n", mapping_fun(item));
    }
}

void DcAutoCompleteUpdateCommonPrefix(std::string& common_prefix, const std::string& value, bool& first,
                                      bool case_sensitive = false)
{
    if (first) {
        first = false;
        common_prefix = value;
    }
    if (common_prefix.size() > value.size())
        common_prefix.resize(value.size());
    for (size_t i = 0; i < common_prefix.size(); ++i) {
        bool match;
        if (case_sensitive) {
            match = common_prefix[i] == value[i];
        }
        else {
            char l = std::tolower(static_cast<unsigned char>(common_prefix[i]));
            char r = std::tolower(static_cast<unsigned char>(value[i]));
            match = l == r;
        }
        if (!match) {
            common_prefix.resize(i);
            break;
        }
    }
}

void DcAutoCompleteLevel(int offset)
{
    std::string level_name;
    DcAutoCompleteGetComponent(offset, level_name);
    if (level_name.size() < 3)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<std::string> matches;
    PackfileFindMatchingFiles(StringMatcher().Prefix(level_name).Suffix(".rfl"), [&](const char* name) {
        auto ext = strrchr(name, '.');
        auto name_len = ext ? ext - name : strlen(name);
        std::string name_without_ext(name, name_len);
        matches.push_back(name_without_ext);
        DcAutoCompleteUpdateCommonPrefix(common_prefix, name_without_ext, first);
    });

    if (matches.size() == 1)
        DcAutoCompletePutComponent(offset, matches[0], true);
    else if (!matches.empty()) {
        DcAutoCompletePrintSuggestions(matches, [](std::string& name) { return name.c_str(); });
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
}

void DcAutoCompletePlayer(int offset)
{
    std::string player_name;
    DcAutoCompleteGetComponent(offset, player_name);
    if (player_name.size() < 1)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<rf::Player*> matching_players;
    FindPlayer(StringMatcher().Prefix(player_name), [&](rf::Player* player) {
        matching_players.push_back(player);
        DcAutoCompleteUpdateCommonPrefix(common_prefix, player->name.CStr(), first);
    });

    if (matching_players.size() == 1)
        DcAutoCompletePutComponent(offset, matching_players[0]->name.CStr(), true);
    else if (!matching_players.empty()) {
        DcAutoCompletePrintSuggestions(matching_players, [](rf::Player* player) {
            // Print player names
            return player->name.CStr();
        });
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
}

void DcAutoCompleteCommand(int offset)
{
    std::string cmd_name;
    int next_offset = DcAutoCompleteGetComponent(offset, cmd_name);
    if (cmd_name.size() < 2)
        return;

    bool first = true;
    std::string common_prefix;

    std::vector<rf::ConsoleCommand*> matching_cmds;
    for (unsigned i = 0; i < rf::console_num_commands; ++i) {
        rf::ConsoleCommand* cmd = g_commands_buffer[i];
        if (!strnicmp(cmd->name, cmd_name.c_str(), cmd_name.size()) &&
            (next_offset == -1 || !cmd->name[cmd_name.size()])) {
            matching_cmds.push_back(cmd);
            DcAutoCompleteUpdateCommonPrefix(common_prefix, cmd->name, first);
        }
    }

    if (next_offset != -1) {
        if (!stricmp(cmd_name.c_str(), "level"))
            DcAutoCompleteLevel(next_offset);
        else if (!stricmp(cmd_name.c_str(), "kick") || !stricmp(cmd_name.c_str(), "ban"))
            DcAutoCompletePlayer(next_offset);
        else if (!stricmp(cmd_name.c_str(), "rcon"))
            DcAutoCompleteCommand(next_offset);
        else if (!stricmp(cmd_name.c_str(), "help"))
            DcAutoCompleteCommand(next_offset);
        else if (matching_cmds.size() != 1)
            return;
        else
            DcShowCmdHelp(matching_cmds[0]);
    }
    else if (matching_cmds.size() > 1) {
        for (auto* cmd : matching_cmds) {
            if (cmd->help)
                rf::ConsolePrintf("%s - %s", cmd->name, cmd->help);
            else
                rf::ConsolePrintf("%s", cmd->name);
        }
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
    else if (matching_cmds.size() == 1)
        DcAutoCompletePutComponent(offset, matching_cmds[0]->name, true);
}

FunHook<void()> DcAutoCompleteInput_hook{
    0x0050A620,
    []() {
        // autocomplete on offset 0
        DcAutoCompleteCommand(0);
    },
};

void ConsoleAutoCompleteApplyPatch()
{
    // Better console autocomplete
    DcAutoCompleteInput_hook.Install();
}

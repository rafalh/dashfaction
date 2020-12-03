#include "console.h"
#include "../main.h"
#include "../misc/vpackfile.h"
#include "../rf/player.h"
#include <patch_common/FunHook.h>
#include <cstring>

void console_show_cmd_help(rf::ConsoleCommand* cmd)
{
    rf::console_run = 0;
    rf::console_help = 1;
    rf::console_status = 0;
    auto handler = reinterpret_cast<void(__thiscall*)(rf::ConsoleCommand*)>(cmd->func);
    handler(cmd);
}

int console_auto_complete_get_component(int offset, std::string& result)
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

void console_auto_complete_put_component(int offset, const std::string& component, bool finished)
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
void console_auto_complete_print_suggestions(T& suggestions, F mapping_fun)
{
    for (auto& item : suggestions) {
        rf::console_printf("%s\n", mapping_fun(item));
    }
}

void console_auto_complete_update_common_prefix(std::string& common_prefix, const std::string& value, bool& first,
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

void console_auto_complete_level(int offset)
{
    std::string level_name;
    console_auto_complete_get_component(offset, level_name);
    if (level_name.size() < 3)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<std::string> matches;
    vpackfile_find_matching_files(StringMatcher().prefix(level_name).suffix(".rfl"), [&](const char* name) {
        auto ext = strrchr(name, '.');
        auto name_len = ext ? ext - name : strlen(name);
        std::string name_without_ext(name, name_len);
        matches.push_back(name_without_ext);
        console_auto_complete_update_common_prefix(common_prefix, name_without_ext, first);
    });

    if (matches.size() == 1)
        console_auto_complete_put_component(offset, matches[0], true);
    else if (!matches.empty()) {
        console_auto_complete_print_suggestions(matches, [](std::string& name) { return name.c_str(); });
        console_auto_complete_put_component(offset, common_prefix, false);
    }
}

void console_auto_complete_player(int offset)
{
    std::string player_name;
    console_auto_complete_get_component(offset, player_name);
    if (player_name.size() < 1)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<rf::Player*> matching_players;
    find_player(StringMatcher().prefix(player_name), [&](rf::Player* player) {
        matching_players.push_back(player);
        console_auto_complete_update_common_prefix(common_prefix, player->name.c_str(), first);
    });

    if (matching_players.size() == 1)
        console_auto_complete_put_component(offset, matching_players[0]->name.c_str(), true);
    else if (!matching_players.empty()) {
        console_auto_complete_print_suggestions(matching_players, [](rf::Player* player) {
            // Print player names
            return player->name.c_str();
        });
        console_auto_complete_put_component(offset, common_prefix, false);
    }
}

void console_auto_complete_command(int offset)
{
    std::string cmd_name;
    int next_offset = console_auto_complete_get_component(offset, cmd_name);
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
            console_auto_complete_update_common_prefix(common_prefix, cmd->name, first);
        }
    }

    if (next_offset != -1) {
        if (!stricmp(cmd_name.c_str(), "level"))
            console_auto_complete_level(next_offset);
        else if (!stricmp(cmd_name.c_str(), "kick") || !stricmp(cmd_name.c_str(), "ban"))
            console_auto_complete_player(next_offset);
        else if (!stricmp(cmd_name.c_str(), "rcon"))
            console_auto_complete_command(next_offset);
        else if (!stricmp(cmd_name.c_str(), "help"))
            console_auto_complete_command(next_offset);
        else if (matching_cmds.size() != 1)
            return;
        else
            console_show_cmd_help(matching_cmds[0]);
    }
    else if (matching_cmds.size() > 1) {
        for (auto* cmd : matching_cmds) {
            if (cmd->help)
                rf::console_printf("%s - %s", cmd->name, cmd->help);
            else
                rf::console_printf("%s", cmd->name);
        }
        console_auto_complete_put_component(offset, common_prefix, false);
    }
    else if (matching_cmds.size() == 1)
        console_auto_complete_put_component(offset, matching_cmds[0]->name, true);
}

FunHook<void()> console_auto_complete_input_hook{
    0x0050A620,
    []() {
        // autocomplete on offset 0
        console_auto_complete_command(0);
    },
};

void console_auto_complete_apply_patches()
{
    // Better console autocomplete
    console_auto_complete_input_hook.install();
}

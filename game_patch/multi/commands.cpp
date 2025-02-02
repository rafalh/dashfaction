#include "../os/console.h"
#include "server_internal.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/level.h"
#include "../rf/player/player.h"
#include "../rf/crt.h"
#include "server.h"
#include "multi.h"
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>
#include <vector>
#include <format>

std::vector<int> g_players_to_kick;

void extend_round_time(int minutes)
{
    rf::level.time -= minutes * 60.0f;
}

void restart_current_level()
{
    rf::multi_change_level(rf::level.filename.c_str());
}

void load_next_level()
{
    rf::multi_change_level(nullptr);
}

void load_prev_level()
{
    rf::netgame.current_level_index--;
    if (rf::netgame.current_level_index < 0) {
        rf::netgame.current_level_index = rf::netgame.levels.size() - 1;
    }
    if (g_prev_level.empty()) {
        // this is the first level running - use previous level from rotation
        rf::multi_change_level(rf::netgame.levels[rf::netgame.current_level_index]);
    }
    else {
        rf::multi_change_level(g_prev_level.c_str());
    }
}

void load_rand_level()
{
    rf::multi_change_level(get_rand_level_filename());
}

bool validate_is_server()
{
    if (!rf::is_server) {
        rf::console::output("Command can be only executed on server", nullptr);
        return false;
    }
    return true;
}

bool validate_not_limbo()
{
    if (rf::gameseq_get_state() != rf::GS_GAMEPLAY) {
        rf::console::output("Command can not be used between rounds", nullptr);
        return false;
    }
    return true;
}

ConsoleCommand2 map_ext_cmd{
    "map_ext",
    [](std::optional<int> minutes_opt) {
        if (validate_is_server() && validate_not_limbo()) {
            int minutes = minutes_opt.value_or(5);
            extend_round_time(minutes);
            std::string msg = std::format("\xA6 Round extended by {} minutes", minutes);
            rf::multi_chat_say(msg.c_str(), false);
        }
    },
    "Extend round time",
    "map_ext [minutes]",
};

ConsoleCommand2 map_rest_cmd{
    "map_rest",
    []() {
        if (validate_is_server() && validate_not_limbo()) {
            rf::multi_chat_say("\xA6 Restarting current level", false);
            restart_current_level();
        }
    },
    "Restart current level",
};

ConsoleCommand2 map_next_cmd{
    "map_next",
    []() {
        if (validate_is_server() && validate_not_limbo()) {
            rf::multi_chat_say("\xA6 Loading next level", false);
            load_next_level();
        }
    },
    "Load next level",
};

ConsoleCommand2 map_rand_cmd{
    "map_rand",
    []() {
        if (validate_is_server() && validate_not_limbo()) {
            rf::multi_chat_say("\xA6 Loading random level from rotation", false);
            load_rand_level();
        }
    },
    "Load random level from rotation",
};

ConsoleCommand2 map_prev_cmd{
    "map_prev",
    []() {
        if (validate_is_server() && validate_not_limbo()) {
            rf::multi_chat_say("\xA6 Loading previous level", false);
            load_prev_level();
        }
    },
    "Load previous level",
};

void kick_player_delayed(rf::Player* player)
{
    g_players_to_kick.push_back(player->net_data->player_id);
}

CallHook<void(rf::Player*)> multi_kick_player_hook{0x0047B9BD, kick_player_delayed};

void process_delayed_kicks()
{
    // Process kicks outside of packet processing loop to avoid crash when a player is suddenly destroyed (00479299)
    for (int player_id : g_players_to_kick) {
        rf::Player* player = rf::multi_find_player_by_id(player_id);
        if (player) {
            rf::multi_kick_player(player);
        }
    }
    g_players_to_kick.clear();
}

void ban_cmd_handler_hook()
{
    if (rf::is_multi && rf::is_server) {
        if (rf::console::run) {
            rf::console::get_arg(rf::console::ARG_STR, true);
            rf::Player* player = find_best_matching_player(rf::console::str_arg);

            if (player) {
                if (player != rf::local_player) {
                    rf::console::printf(rf::strings::banning_player, player->name.c_str());
                    rf::multi_ban_ip(player->net_data->addr);
                    kick_player_delayed(player);
                }
                else
                    rf::console::print("You cannot ban yourself!");
            }
        }

        if (rf::console::help) {
            rf::console::output(rf::strings::usage, nullptr);
            rf::console::print("     ban <{}>", rf::strings::player_name);
        }
    }
}

void kick_cmd_handler_hook()
{
    if (rf::is_multi && rf::is_server) {
        if (rf::console::run) {
            rf::console::get_arg(rf::console::ARG_STR, true);
            rf::Player* player = find_best_matching_player(rf::console::str_arg);

            if (player) {
                if (player != rf::local_player) {
                    rf::console::printf(rf::strings::kicking_player, player->name.c_str());
                    kick_player_delayed(player);
                }
                else
                    rf::console::print("You cannot kick yourself!");
            }
        }

        if (rf::console::help) {
            rf::console::output(rf::strings::usage, nullptr);
            rf::console::print("     kick <{}>", rf::strings::player_name);
        }
    }
}

ConsoleCommand2 unban_last_cmd{
    "unban_last",
    []() {
        if (rf::is_multi && rf::is_server) {
            auto opt = multi_ban_unban_last();
            if (opt) {
                rf::console::print("{} has been unbanned!", opt.value());
            }
        }
    },
    "Unbans last banned player",
};

void init_server_commands()
{
    map_ext_cmd.register_cmd();
    map_rest_cmd.register_cmd();
    map_next_cmd.register_cmd();
    map_rand_cmd.register_cmd();
    map_prev_cmd.register_cmd();

    AsmWriter(0x0047B6F0).jmp(ban_cmd_handler_hook);
    AsmWriter(0x0047B580).jmp(kick_cmd_handler_hook);

    unban_last_cmd.register_cmd();

    multi_kick_player_hook.install();
}

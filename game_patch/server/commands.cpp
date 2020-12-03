#include "../console/console.h"
#include "server_internal.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/geometry.h"

void extend_round_time(int minutes)
{
    auto& level_time = addr_as_ref<float>(0x006460F0);
    level_time -= minutes * 60.0f;
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

bool validate_is_server()
{
    if (!rf::is_server) {
        rf::console_output("Command can be only executed on server", nullptr);
        return false;
    }
    return true;
}

bool validate_not_limbo()
{
    if (rf::gameseq_get_state() != rf::GS_GAMEPLAY) {
        rf::console_output("Command can not be used between rounds", nullptr);
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
            std::string msg = string_format("\xA6 Round extended by %d minutes", minutes);
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

void init_server_commands()
{
    map_ext_cmd.register_cmd();
    map_rest_cmd.register_cmd();
    map_next_cmd.register_cmd();
    map_prev_cmd.register_cmd();
}

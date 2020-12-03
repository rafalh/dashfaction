#include "../console/console.h"
#include "server_internal.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/geometry.h"

void ExtendRoundTime(int minutes)
{
    auto& level_time = addr_as_ref<float>(0x006460F0);
    level_time -= minutes * 60.0f;
}

void RestartCurrentLevel()
{
    rf::multi_change_level(rf::level.filename.c_str());
}

void LoadNextLevel()
{
    rf::multi_change_level(nullptr);
}

void LoadPrevLevel()
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

bool ValidateIsServer()
{
    if (!rf::is_server) {
        rf::console_output("Command can be only executed on server", nullptr);
        return false;
    }
    return true;
}

bool ValidateNotLimbo()
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
        if (ValidateIsServer() && ValidateNotLimbo()) {
            int minutes = minutes_opt.value_or(5);
            ExtendRoundTime(minutes);
            std::string msg = StringFormat("\xA6 Round extended by %d minutes", minutes);
            rf::multi_chat_say(msg.c_str(), false);
        }
    },
    "Extend round time",
    "map_ext [minutes]",
};

ConsoleCommand2 map_rest_cmd{
    "map_rest",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::multi_chat_say("\xA6 Restarting current level", false);
            RestartCurrentLevel();
        }
    },
    "Restart current level",
};

ConsoleCommand2 map_next_cmd{
    "map_next",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::multi_chat_say("\xA6 Loading next level", false);
            LoadNextLevel();
        }
    },
    "Load next level",
};

ConsoleCommand2 map_prev_cmd{
    "map_prev",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::multi_chat_say("\xA6 Loading previous level", false);
            LoadPrevLevel();
        }
    },
    "Load previous level",
};

void InitServerCommands()
{
    map_ext_cmd.Register();
    map_rest_cmd.Register();
    map_next_cmd.Register();
    map_prev_cmd.Register();
}

#include "../console/console.h"
#include "server_internal.h"
#include "../rf/network.h"
#include "../rf/gameseq.h"

void ExtendRoundTime(int minutes)
{
    auto& level_time = AddrAsRef<float>(0x006460F0);
    level_time -= minutes * 60.0f;
}

void RestartCurrentLevel()
{
    rf::MultiChangeLevel(rf::level_filename.CStr());
}

void LoadNextLevel()
{
    rf::MultiChangeLevel(nullptr);
}

void LoadPrevLevel()
{
    rf::netgame.current_level_index--;
    if (rf::netgame.current_level_index < 0) {
        rf::netgame.current_level_index = rf::netgame.levels.Size() - 1;
    }
    if (g_prev_level.empty()) {
        // this is the first level running - use previous level from rotation
        rf::MultiChangeLevel(rf::netgame.levels.Get(rf::netgame.current_level_index));
    }
    else {
        rf::MultiChangeLevel(g_prev_level.c_str());
    }
}

bool ValidateIsServer()
{
    if (!rf::is_server) {
        rf::ConsoleOutput("Command can be only executed on server", nullptr);
        return false;
    }
    return true;
}

bool ValidateNotLimbo()
{
    if (rf::GameSeqGetState() != rf::GS_GAMEPLAY) {
        rf::ConsoleOutput("Command can not be used between rounds", nullptr);
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
            rf::ChatSay(msg.c_str(), false);
        }
    },
    "Extend round time",
    "map_ext [minutes]",
};

ConsoleCommand2 map_rest_cmd{
    "map_rest",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::ChatSay("\xA6 Restarting current level", false);
            RestartCurrentLevel();
        }
    },
    "Restart current level",
};

ConsoleCommand2 map_next_cmd{
    "map_next",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::ChatSay("\xA6 Loading next level", false);
            LoadNextLevel();
        }
    },
    "Load next level",
};

ConsoleCommand2 map_prev_cmd{
    "map_prev",
    []() {
        if (ValidateIsServer() && ValidateNotLimbo()) {
            rf::ChatSay("\xA6 Loading previous level", false);
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
